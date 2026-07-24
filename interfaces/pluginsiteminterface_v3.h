// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef PLUGINSITEMINTERFACE_V3_H
#define PLUGINSITEMINTERFACE_V3_H

#include "pluginsiteminterface_v2.h"

#include <QString>
#include <QWindow>

class PluginsItemInterfaceV3 : public PluginsItemInterfaceV2
{
public:
    /**
     * @brief Item key that should be exported as a card surface.
     *
     * The item key is still scoped by pluginName(), so the compositor can use
     * "pluginName::itemKey" as a stable surface id.
     */
    virtual QString cardItemKey() const { return {}; }

    /**
     * @brief Native window for the card item.
     *
     * The loader exposes this window to the dock compositor as a Wayland
     * surface. QML or QWidget based UI should be handled by plugin itself.
     * The plugin keeps ownership of the returned window.
     */
    virtual QWindow *cardWindow() const { return nullptr; }
};

QT_BEGIN_NAMESPACE

#define ModuleInterface_iid_V3 "com.deepin.dock.PluginsItemInterface_V3"

Q_DECLARE_INTERFACE(PluginsItemInterfaceV3, ModuleInterface_iid_V3)
QT_END_NAMESPACE

#endif // PLUGINSITEMINTERFACE_V3_H
