// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dqwaylandplatform.h"
#include "plugin.h"

#include <DPlatformTheme>

DGUI_USE_NAMESPACE
using namespace Plugin;

DQWaylandPlatformInterface::DQWaylandPlatformInterface(DPlatformTheme *platformTheme)
    : DPlatformInterface(platformTheme)
{
    QObject::connect(PlatformInterfaceProxy::instance(), &PlatformInterfaceProxy::fontNameChanged, platformTheme, &DPlatformTheme::fontNameChanged);
    QObject::connect(PlatformInterfaceProxy::instance(), &PlatformInterfaceProxy::fontPointSizeChanged, platformTheme, &DPlatformTheme::fontPointSizeChanged);
    QObject::connect(PlatformInterfaceProxy::instance(), &PlatformInterfaceProxy::activeColorChanged, platformTheme, &DPlatformTheme::activeColorChanged);
    QObject::connect(PlatformInterfaceProxy::instance(), &PlatformInterfaceProxy::darkActiveColorChanged, platformTheme, &DPlatformTheme::darkActiveColorChanged);
    QObject::connect(PlatformInterfaceProxy::instance(), &PlatformInterfaceProxy::themeNameChanged, platformTheme, &DPlatformTheme::themeNameChanged);
    QObject::connect(PlatformInterfaceProxy::instance(), &PlatformInterfaceProxy::iconThemeNameChanged, platformTheme, &DPlatformTheme::iconThemeNameChanged);
}

QByteArray DQWaylandPlatformInterface::fontName() const
{
    return PlatformInterfaceProxy::instance()->fontName();
}

qreal DQWaylandPlatformInterface::fontPointSize() const
{
    return PlatformInterfaceProxy::instance()->fontPointSize();
}

QColor DQWaylandPlatformInterface::activeColor() const
{
    return PlatformInterfaceProxy::instance()->activeColor();
}

QColor DQWaylandPlatformInterface::darkActiveColor() const
{
    return PlatformInterfaceProxy::instance()->darkActiveColor();
}

QByteArray DQWaylandPlatformInterface::themeName() const
{
    return PlatformInterfaceProxy::instance()->themeName();
}

QByteArray DQWaylandPlatformInterface::iconThemeName() const
{
    return PlatformInterfaceProxy::instance()->iconThemeName();
}
