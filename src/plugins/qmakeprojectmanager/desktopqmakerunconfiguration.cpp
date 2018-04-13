/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "desktopqmakerunconfiguration.h"

#include "qmakeprojectmanagerconstants.h"

#include <coreplugin/variablechooser.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtoutputformatter.h>
#include <qtsupport/qtsupportconstants.h>

#include <utils/fileutils.h>
#include <utils/pathchooser.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>

using namespace ProjectExplorer;
using namespace Utils;

namespace QmakeProjectManager {
namespace Internal {

const char QMAKE_RC_PREFIX[] = "Qt4ProjectManager.Qt4RunConfiguration:";
const char PRO_FILE_KEY[] = "Qt4ProjectManager.Qt4RunConfiguration.ProFile";

//
// DesktopQmakeRunConfiguration
//

DesktopQmakeRunConfiguration::DesktopQmakeRunConfiguration(Target *target)
    : RunConfiguration(target, QMAKE_RC_PREFIX)
{
    auto envAspect = new LocalEnvironmentAspect(this, [](RunConfiguration *rc, Environment &env) {
                       static_cast<DesktopQmakeRunConfiguration *>(rc)->addToBaseEnvironment(env);
                   });
    addExtraAspect(envAspect);

    addExtraAspect(new ExecutableAspect(this));
    addExtraAspect(new ArgumentsAspect(this, "Qt4ProjectManager.Qt4RunConfiguration.CommandLineArguments"));
    addExtraAspect(new TerminalAspect(this, "Qt4ProjectManager.Qt4RunConfiguration.UseTerminal"));
    addExtraAspect(new WorkingDirectoryAspect(this, "Qt4ProjectManager.Qt4RunConfiguration.UserWorkingDirectory"));

    setOutputFormatter<QtSupport::QtOutputFormatter>();

    auto libAspect = new UseLibraryPathsAspect(this, "QmakeProjectManager.QmakeRunConfiguration.UseLibrarySearchPath");
    addExtraAspect(libAspect);
    connect(libAspect, &UseLibraryPathsAspect::changed,
            envAspect, &EnvironmentAspect::environmentChanged);

    if (HostOsInfo::isMacHost()) {
        auto dyldAspect = new UseDyldSuffixAspect(this, "QmakeProjectManager.QmakeRunConfiguration.UseDyldImageSuffix");
        addExtraAspect(dyldAspect);
        connect(dyldAspect, &UseLibraryPathsAspect::changed,
                envAspect, &EnvironmentAspect::environmentChanged);
    }

    connect(target->project(), &Project::parsingFinished,
            this, &DesktopQmakeRunConfiguration::updateTargetInformation);
}

void DesktopQmakeRunConfiguration::updateTargetInformation()
{
    setDefaultDisplayName(defaultDisplayName());
    extraAspect<LocalEnvironmentAspect>()->buildEnvironmentHasChanged();

    BuildTargetInfo bti = target()->applicationTargets().buildTargetInfo(buildKey());

    auto wda = extraAspect<WorkingDirectoryAspect>();
    wda->setDefaultWorkingDirectory(bti.workingDirectory);
    if (wda->pathChooser())
        wda->pathChooser()->setBaseFileName(target()->project()->projectDirectory());

    auto terminalAspect = extraAspect<TerminalAspect>();
    if (!terminalAspect->isUserSet())
        terminalAspect->setUseTerminal(bti.usesTerminal);

    extraAspect<ExecutableAspect>()->setExecutable(bti.targetFilePath);
}

//
// DesktopQmakeRunConfigurationWidget
//

DesktopQmakeRunConfigurationWidget::DesktopQmakeRunConfigurationWidget(RunConfiguration *rc)
    :  m_runConfiguration(rc)
{
    auto toplayout = new QFormLayout(this);

    rc->extraAspect<ExecutableAspect>()->addToMainConfigurationWidget(this, toplayout);
    rc->extraAspect<ArgumentsAspect>()->addToMainConfigurationWidget(this, toplayout);
    rc->extraAspect<WorkingDirectoryAspect>()->addToMainConfigurationWidget(this, toplayout);
    rc->extraAspect<TerminalAspect>()->addToMainConfigurationWidget(this, toplayout);
    rc->extraAspect<UseLibraryPathsAspect>()->addToMainConfigurationWidget(this, toplayout);

    if (HostOsInfo::isMacHost())
        rc->extraAspect<UseDyldSuffixAspect>()->addToMainConfigurationWidget(this, toplayout);

    Core::VariableChooser::addSupportForChildWidgets(this, rc->macroExpander());
}

QWidget *DesktopQmakeRunConfiguration::createConfigurationWidget()
{
    return wrapWidget(new DesktopQmakeRunConfigurationWidget(this));
}

Runnable DesktopQmakeRunConfiguration::runnable() const
{
    StandardRunnable r;
    r.executable = extraAspect<ExecutableAspect>()->executable().toString();
    r.commandLineArguments = extraAspect<ArgumentsAspect>()->arguments();
    r.workingDirectory = extraAspect<WorkingDirectoryAspect>()->workingDirectory().toString();
    r.environment = extraAspect<LocalEnvironmentAspect>()->environment();
    r.runMode = extraAspect<TerminalAspect>()->runMode();
    return r;
}

QVariantMap DesktopQmakeRunConfiguration::toMap() const
{
    // FIXME: For compatibility purposes in the 4.7 dev cycle only.
    const QDir projectDir = QDir(target()->project()->projectDirectory().toString());
    QVariantMap map(RunConfiguration::toMap());
    map.insert(QLatin1String(PRO_FILE_KEY), projectDir.relativeFilePath(proFilePath().toString()));
    return map;
}

bool DesktopQmakeRunConfiguration::fromMap(const QVariantMap &map)
{
    const bool res = RunConfiguration::fromMap(map);
    if (!res)
        return false;

    updateTargetInformation();
    return true;
}

void DesktopQmakeRunConfiguration::doAdditionalSetup(const RunConfigurationCreationInfo &)
{
    updateTargetInformation();
}

void DesktopQmakeRunConfiguration::addToBaseEnvironment(Environment &env) const
{
    BuildTargetInfo bti = target()->applicationTargets().buildTargetInfo(buildKey());
    if (bti.runEnvModifier)
        bti.runEnvModifier(env, extraAspect<UseLibraryPathsAspect>()->value());

    if (auto dyldAspect = extraAspect<UseDyldSuffixAspect>()) {
        if (dyldAspect->value())
            env.set(QLatin1String("DYLD_IMAGE_SUFFIX"), QLatin1String("_debug"));
    }
}

bool DesktopQmakeRunConfiguration::canRunForNode(const Node *node) const
{
    return node->filePath() == proFilePath();
}

FileName DesktopQmakeRunConfiguration::proFilePath() const
{
    return FileName::fromString(buildKey());
}

QString DesktopQmakeRunConfiguration::defaultDisplayName()
{
    FileName profile = proFilePath();
    if (!profile.isEmpty())
        return profile.toFileInfo().completeBaseName();
    return tr("Qt Run Configuration");
}

//
// DesktopQmakeRunConfigurationFactory
//

DesktopQmakeRunConfigurationFactory::DesktopQmakeRunConfigurationFactory()
{
    registerRunConfiguration<DesktopQmakeRunConfiguration>(QMAKE_RC_PREFIX);
    addSupportedProjectType(QmakeProjectManager::Constants::QMAKEPROJECT_ID);
    addSupportedTargetDeviceType(ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE);
}

} // namespace Internal
} // namespace QmakeProjectManager
