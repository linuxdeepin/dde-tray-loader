// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DQWAYLANDPLATFORM_H
#define DQWAYLANDPLATFORM_H

#include "Private/dplatforminterface_p.h"

class DQWaylandPlatformInterface : public DTK_GUI_NAMESPACE::DPlatformInterface
{
public:
    explicit DQWaylandPlatformInterface(DTK_GUI_NAMESPACE::DPlatformTheme *platformTheme);

    QByteArray fontName() const override;
    qreal fontPointSize() const override;
    QColor activeColor() const override;
    QColor darkActiveColor() const override;
    QByteArray themeName() const override;
    QByteArray iconThemeName() const override;
};

#endif
