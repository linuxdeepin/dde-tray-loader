// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "plugin-wrapper.h"
#include "../util/utils.h"
#include "settingmanager.h"
#include "dock-constants.h"
#include "dconfig_helper.h"

PluginWrapper::PluginWrapper(QObject* parent, PluginsItemInterface* plugin, QPluginLoader* pluginLoader)
    : QObject(parent)
    , m_plugin(plugin)
    , m_pluginV2(toV2(plugin))
    , m_moduleGsettings(Utils::ModuleSettingsPtr(plugin->pluginName(), QByteArray(), this))
    , m_pluginLoader(pluginLoader)
    , m_flags(getFlags())
    , m_initialized(false)
{
    if (m_moduleGsettings) {
        connect(m_moduleGsettings, &QGSettings::changed, this, [this] (const QString &key) {
            if (key == "enable" && !Dock::MODULE_ENABLE_WHITE_LIST.contains(pluginName()) && m_initialized) {
                m_plugin->pluginSettingsChanged();
                Q_EMIT pluginInfoChanged();
                Q_EMIT enableStateChanged(m_moduleGsettings->get("enable").toBool());
            }
        });
    }
}

bool PluginWrapper::isQuickPluginDocked() const
{
    return SettingManager::instance()->dockedPlugins().contains(pluginName());
}

QJsonObject PluginWrapper::metaData() const
{
    if (!m_pluginLoader)
        return QJsonObject();

    return m_pluginLoader->metaData().value("MetaData").toObject();
}

Dock::ItemType PluginWrapper::itemType() const
{
    if (flags() & Type_Fixed)
        return Dock::ItemType_FixedPlugin;
    else if (flags() & Type_Tool)
        return Dock::ItemType_ToolPlugin;

    return Dock::ItemType_Plugins;
}

void PluginWrapper::addItemKey(const QString& itemKey)
{
    if (!m_items.contains(itemKey)) {
        m_items.append(itemKey);
        Q_EMIT pluginInfoChanged();
    }
}

void PluginWrapper::removeItem(const QString& itemKey)
{
    if (m_items.contains(itemKey)) {
        m_items.removeAll(itemKey);
        Q_EMIT pluginInfoChanged();
    }
}

PluginFlags PluginWrapper::flags(PluginsItemInterface* plugin, QPluginLoader* pluginLoader)
{
    auto pluginV2 = toV2(plugin);
    if (pluginV2) {
        return pluginV2->flags();
    }

    if (plugin && pluginLoader) {
        bool ok;
        auto flags = static_cast<PluginFlags>(pluginLoader->instance()->property("pluginFlags").toInt(&ok));
        if (ok) {
            return flags;
        }
    }

    return UNADAPTED_PLUGIN_FLAGS;
}

PluginFlags PluginWrapper::getFlags() const
{
    if (m_pluginV2) {
        return m_pluginV2->flags();
    }

    if (m_pluginLoader) {
        bool ok;
        auto flags = static_cast<PluginFlags>(m_pluginLoader->instance()->property("pluginFlags").toInt(&ok));
        if (ok) {
            return flags;
        }
    }

    return UNADAPTED_PLUGIN_FLAGS;
}

QJsonObject PluginWrapper::toJson(bool concise) const
{
    if (!m_plugin || !m_pluginLoader) {
        return QJsonObject();
    }

    QJsonObject obj;
    obj["pluginName"] = m_plugin->pluginName();
    if (concise) {
        obj["loaded"] = isLoaded();
        obj["pluginDisplayName"] = m_plugin->pluginDisplayName();
        obj["flags"] = static_cast<int>(flags());
        obj["apiVersion"] = apiVersion();
        obj["dependsDaemonDbusService"] = dependsDaemonDbusService();
        obj["pluginFileName"] = m_pluginLoader->fileName();
        obj["enabled"] = isEnabled();
        obj["supported"] = supportFlag();
    }

    return obj;
}

bool PluginWrapper::isEnabled() const
{
    if (Dock::MODULE_ENABLE_WHITE_LIST.contains(pluginName()))
        return true;

    return (m_moduleGsettings && m_moduleGsettings->keys().contains("enable")) ? m_moduleGsettings->get("enable").toBool() : true;
}

/**
 * @brief 获取插件当前是否被支持
 *
 * @return true 以蓝牙举例，系统有蓝牙模块，快捷面板和控制中心-个性化-任务栏-插件区域显示`蓝牙`项
 * @return false 以蓝牙举例，系统没有蓝牙模块，快捷面板和控制中心-个性化-任务栏-插件区域不限显示`蓝牙`项
 */
bool PluginWrapper::supportFlag() const
{
    if (!m_pluginV2) {
        return true;
    }

    QJsonObject obj;
    obj[Dock::MSG_TYPE] = Dock::MSG_GET_SUPPORT_FLAG;
    auto ret = m_pluginV2->message(Utils::toJson(obj));
    if (ret.isEmpty())
        return true;

    auto rootObj = Utils::getRootObj(ret);
    if (rootObj.isEmpty() || !rootObj.contains(Dock::MSG_SUPPORT_FLAG)) {
        return true;
    }

    return rootObj.value(Dock::MSG_SUPPORT_FLAG).toBool(true);
}

/**
 * @brief 通知插件当前任务栏溢出区的状态
 *
 * @param state 见constants.h中`应用溢出状态`
 */
void PluginWrapper::updateOverflowState(int state) const
{
    if (!m_pluginV2) {
        return;
    }

    QJsonObject obj;
    obj[Dock::MSG_TYPE] = Dock::MSG_UPDATE_OVERFLOW_STATE;
    obj[Dock::MSG_DATA] = state;
    m_pluginV2->message(Utils::toJson(obj));
}

/**
 * @brief 通知插件弹窗显示的最小高度
 *
 * @param minHeight 最小高度
 */
void PluginWrapper::setAppletMinHeight(int minHeight) const
{
    if (!m_pluginV2) {
        return;
    }

    QJsonObject obj;
    obj[Dock::MSG_TYPE] = Dock::MSG_SET_APPLET_MIN_HEIGHT;
    obj[Dock::MSG_DATA] = minHeight;
    m_pluginV2->message(Utils::toJson(obj));
}

bool PluginWrapper::wantToBeLoaded() const
{
    if (!m_pluginV2) {
        return true;
    }

    QJsonObject obj;
    obj[Dock::MSG_TYPE] = Dock::MSG_WHETHER_WANT_TO_BE_LOADED;
    const auto &ret = m_pluginV2->message(Utils::toJson(obj));
    if (ret.isEmpty())
        return true;
    const auto &rootObj = Utils::getRootObj(ret);
    return rootObj.value(Dock::MSG_DATA).toBool(true);
}

/**
 * @brief 告知插件弹窗即将在哪里显示（任务栏还是快捷面板中）
 *
 * @param container 0：任务栏弹窗，1：快捷面板子页面
 */
void PluginWrapper::setAppletContainer(int container) const
{
    if (!m_pluginV2) {
        return;
    }

    QJsonObject obj;
    obj[Dock::MSG_TYPE] = Dock::MSG_APPLET_CONTAINER;
    obj[Dock::MSG_DATA] = container;
    m_pluginV2->message(Utils::toJson(obj));
}

/**
 * @brief 当任务栏size发生改变时，通知插件
 *
 * @param dockSize 任务栏size
 */
void PluginWrapper::updateDockPanelSize(const QSize &dockSize) const
{
    if (!m_pluginV2) {
        return;
    }

    QJsonObject sizeData;
    sizeData["width"] = dockSize.width();
    sizeData["height"] = dockSize.height();

    QJsonObject obj;
    obj[Dock::MSG_TYPE] = Dock::MSG_DOCK_PANEL_SIZE_CHANGED;
    obj[Dock::MSG_DATA] = sizeData;
    m_pluginV2->message(Utils::toJson(obj));
}


/**
 * @brief 获取插件属性Map
 *
 * @return QMap<QString, QVariant>，属性的key和value
 */
QMap<QString, QVariant> PluginWrapper::pluginPropertyMap() const
{
    QMap<QString, QVariant> map;

    if (!m_pluginV2) {
        return map;
    }

    QJsonObject obj;
    obj[Dock::MSG_TYPE] = Dock::MSG_PLUGIN_PROPERTY;
    auto ret = m_pluginV2->message(Utils::toJson(obj));
    if (ret.isEmpty())
        return map;

    auto rootObj = Utils::getRootObj(ret);
    if (rootObj.isEmpty() || !rootObj.contains(Dock::MSG_DATA)) {
        return map;
    }

    map = rootObj[Dock::MSG_DATA].toVariant().toMap();

    return map;
}

/**
 * @brief 在插件的右键菜单中，追加选项
 *
 * @param menuJson 原始的右键菜单json列表
 * @param item 追加的选项
 */
QString PluginWrapper::addPluginContextMenu(const QString &menuJson, const QJsonObject &item)
{
    QJsonDocument jsonDocument = QJsonDocument::fromJson(menuJson.toLocal8Bit().data());
    QJsonObject jsonMenu = jsonDocument.object();
    if (!jsonDocument.isNull() && jsonMenu.contains("items")) {
        QJsonArray jsonMenuItems = jsonMenu.value("items").toArray();
        jsonMenuItems.append(item);
        jsonMenu.insert("items", jsonMenuItems);
    } else {
        jsonMenu.insert("items", QJsonArray() << item);
    }

    return QJsonDocument(jsonMenu).toJson();
}

bool PluginWrapper::showUndock(const QString &menuJson)
{
    const QJsonDocument &jsonDocument = QJsonDocument::fromJson(menuJson.toLocal8Bit().data());
    const QJsonObject &jsonMenu = jsonDocument.object();
    if (!jsonDocument.isNull() && jsonMenu.contains("showUndock")) {
        return jsonMenu.value("showUndock").toBool(true);
    }

    return true;
}
