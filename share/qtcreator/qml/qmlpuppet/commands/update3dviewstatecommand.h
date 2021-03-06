/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#pragma once

#include <QMetaType>
#include <QtCore/qsize.h>

namespace QmlDesigner {

class Update3dViewStateCommand
{
    friend QDataStream &operator>>(QDataStream &in, Update3dViewStateCommand &command);
    friend QDebug operator<<(QDebug debug, const Update3dViewStateCommand &command);

public:
    enum Type { StateChange, ActiveChange, SizeChange, Empty };

    explicit Update3dViewStateCommand(Qt::WindowStates previousStates, Qt::WindowStates currentStates);
    explicit Update3dViewStateCommand(bool active, bool hasPopup);
    explicit Update3dViewStateCommand(const QSize &size);
    Update3dViewStateCommand() = default;

    Qt::WindowStates previousStates() const;
    Qt::WindowStates currentStates() const;

    bool isActive() const;
    bool hasPopup() const;
    QSize size() const;

    Type type() const;

private:
    Qt::WindowStates m_previousStates = Qt::WindowNoState;
    Qt::WindowStates m_currentStates = Qt::WindowNoState;

    bool m_active = false;
    bool m_hasPopup = false;
    QSize m_size;

    Type m_type = Empty;
};

QDataStream &operator<<(QDataStream &out, const Update3dViewStateCommand &command);
QDataStream &operator>>(QDataStream &in, Update3dViewStateCommand &command);

QDebug operator<<(QDebug debug, const Update3dViewStateCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::Update3dViewStateCommand)
