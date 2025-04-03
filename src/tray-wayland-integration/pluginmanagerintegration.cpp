// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pluginmanagerintegration_p.h"
#include "plugin.h"
#include "pluginsurface_p.h"

#include <qwayland-plugin-manager-v1.h>
#include <QtWaylandClient/private/qwaylandwindow_p.h>

namespace Plugin {
PluginManagerIntegration::PluginManagerIntegration()
    : QWaylandShellIntegrationTemplate<PluginManagerIntegration>(1)
{
    qInfo() << "PluginManagerIntegration22";
}

PluginManagerIntegration::~PluginManagerIntegration()
{

}

QtWaylandClient::QWaylandShellSurface *PluginManagerIntegration::createShellSurface(QtWaylandClient::QWaylandWindow *window)
{
    qInfo() << "PluginManagerIntegration2::createShellSurface";
    if (EmbedPlugin::contains(window->window())) {
        return new PluginSurface(this, window);
    }

    if (PluginPopup::contains(window->window())) {
        return new PluginPopupSurface(this, window);
    }

    if (tryCreatePopupForSubWindow(window->window())) {
        return new PluginPopupSurface(this, window);
    }

    qWarning() << "create plugin surface failed, unknown window type";
    return nullptr;
}


void PluginManagerIntegration::requestMessage(const QString &plugin_id, const QString &item_key, const QString &msg)
{
    request_message(plugin_id, item_key, msg);
}

void PluginManagerIntegration::plugin_manager_v1_position_changed(uint32_t dock_position)
{
    if (dock_position != m_dockPosition) {
        m_dockPosition = dock_position;
        Q_EMIT dockPositionChanged(m_dockPosition);
    }
}

void PluginManagerIntegration::plugin_manager_v1_color_theme_changed(uint32_t dock_color_theme)
{
    if (dock_color_theme != m_dockColorType) {
        m_dockColorType = dock_color_theme;
        Q_EMIT dockColorThemeChanged(m_dockColorType);
    }
}

void PluginManagerIntegration::plugin_manager_v1_event_message(const QString &msg)
{
    // qInfo() << "plugin receive event message" << msg;
    Q_UNUSED(msg);
    Q_EMIT eventMessage(msg);
}

void PluginManagerIntegration::plugin_manager_v1_active_color_changed(const QString &active_color, const QString &dark_active_color)
{
    PlatformInterfaceProxy::instance()->setActiveColor(QColor(active_color));
    PlatformInterfaceProxy::instance()->setDarkActiveColor(QColor(dark_active_color));
}

void PluginManagerIntegration::plugin_manager_v1_font_changed(const QString &font_name, int32_t font_point_size)
{
    PlatformInterfaceProxy::instance()->setFontName(font_name.toLocal8Bit());
    PlatformInterfaceProxy::instance()->setFontPointSize(font_point_size);
}

void PluginManagerIntegration::plugin_manager_v1_theme_changed(const QString &theme_name, const QString &icon_theme_name)
{
    PlatformInterfaceProxy::instance()->setThemeName(theme_name.toLocal8Bit());
    PlatformInterfaceProxy::instance()->setIconThemeName(icon_theme_name.toLocal8Bit());
}

bool PluginManagerIntegration::tryCreatePopupForSubWindow(QWindow *window)
{
    auto parentWindow = window->transientParent();
    if (!parentWindow)
        return false;

    if (auto plugin = EmbedPlugin::getWithoutCreating(parentWindow)) {
        auto pluginPopup = PluginPopup::get(window);
        pluginPopup->setPopupType(Plugin::PluginPopup::PopupTypeTooltip);
        pluginPopup->setPluginId(plugin->pluginId());
        pluginPopup->setItemKey(plugin->itemKey());
        auto pluginPos = plugin->pluginPos();
        pluginPopup->setX(pluginPos.x() + window->x());
        pluginPopup->setY(pluginPos.y() + window->y());
        return true;
    }

    if (auto plugin = PluginPopup::getWithoutCreating(parentWindow)) {
        auto pluginPopup = PluginPopup::get(window);
        pluginPopup->setPopupType(Plugin::PluginPopup::PopupTypeSubPopup);
        pluginPopup->setPluginId(plugin->pluginId());
        pluginPopup->setItemKey(plugin->itemKey());
        // TODO submenu window's x is changed to zero from parent menu's with when it repeated open.
        window->setX(parentWindow->width());
        auto parentPos = plugin->pluginPos();
        // TODO move to parentWindow's right position.
        pluginPopup->setX(parentPos.x() + parentWindow->width());
        pluginPopup->setY(parentPos.y() + window->y());
        return true;
    }

    return false;
}
}
