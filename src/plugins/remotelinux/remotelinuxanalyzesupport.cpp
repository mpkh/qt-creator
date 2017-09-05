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

#include "remotelinuxanalyzesupport.h"

#include "remotelinuxrunconfiguration.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runnables.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <ssh/sshconnection.h>

#include <qmldebug/qmloutputparser.h>
#include <qmldebug/qmldebugcommandlinearguments.h>

#include <QPointer>

using namespace QSsh;
using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {

// RemoteLinuxQmlProfilerSupport

RemoteLinuxQmlProfilerSupport::RemoteLinuxQmlProfilerSupport(RunControl *runControl)
    : SimpleTargetRunner(runControl)
{
    setDisplayName("RemoteLinuxQmlProfilerSupport");

    m_portsGatherer = new PortsGatherer(runControl);
    addStartDependency(m_portsGatherer);

    // The ports gatherer can safely be stopped once the process is running, even though it has to
    // be started before.
    addStopDependency(m_portsGatherer);

    m_profiler = runControl->createWorker(runControl->runMode());
    m_profiler->addStartDependency(this);
    addStopDependency(m_profiler);
}

void RemoteLinuxQmlProfilerSupport::start()
{
    Port qmlPort = m_portsGatherer->findPort();

    QUrl serverUrl;
    serverUrl.setHost(device()->sshParameters().host);
    serverUrl.setPort(qmlPort.number());
    m_profiler->recordData("QmlServerUrl", serverUrl);

    QString args = QmlDebug::qmlDebugTcpArguments(QmlDebug::QmlProfilerServices, qmlPort);
    auto r = runnable().as<StandardRunnable>();
    if (!r.commandLineArguments.isEmpty())
        r.commandLineArguments.append(' ');
    r.commandLineArguments += args;

    setRunnable(r);

    SimpleTargetRunner::start();
}

} // namespace RemoteLinux
