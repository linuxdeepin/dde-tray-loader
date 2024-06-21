// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef PLUGIN_WRAPPER
#define PLUGIN_WRAPPER

#include "pluginsiteminterface_v2.h"
#include "dock-constants.h"

#include <QPointer>
#include <QPluginLoader>
#include <QJsonObject>
#include <QJsonDocument>
#include <QGSettings>

#include <DGuiApplicationHelper>

using namespace Dock;

const static PluginFlags UNADAPTED_PLUGIN_FLAGS = PluginFlag::Type_Unadapted | PluginFlag::Attribute_Normal;

class PluginWrapper : public QObject
{
    Q_OBJECT
public:
    explicit PluginWrapper(QObject *parent, PluginsItemInterface *plugin
        , QPluginLoader *pluginLoader);

    PluginWrapper() = delete;

    inline PluginFlags flags() const { return m_flags; }
    inline bool canDrag() const { return m_flags & PluginFlag::Attribute_CanDrag; }
    inline bool canInsert() const { return m_flags & PluginFlag::Attribute_CanInsert; }
    inline bool canSetting() const { return m_flags & PluginFlag::Attribute_CanSetting; }
    inline bool isAdaptedPlugin()  { return (m_flags & PluginFlag::Type_Unadapted) == 0; }
    inline bool isQuickPlugin() { return m_flags & PluginFlag::Type_Quick; }
    inline PluginsItemInterface* plugin() const { return m_plugin; }
    inline QPluginLoader* pluginLoader() const {return m_pluginLoader.data(); }
    inline QString pluginName() const { return m_plugin->pluginName(); }
    inline QStringList items() const { return m_items; }
    inline bool isLoaded() const { return !m_items.isEmpty(); }
    inline bool isLoaded(const QString &itemKey) const { return m_items.contains(itemKey); }
    inline QString apiVersion() const { return metaData().value("api").toString(); }
    inline QString dependsDaemonDbusService() const { return metaData().value("depends-daemon-dbus-service").toString(); }
    bool isEnabled() const;
    bool isQuickPluginDocked() const;
    void addItemKey(const QString &itemKey);
    void removeItem(const QString &itemKey);
    QJsonObject toJson(bool concise) const;
    QJsonObject metaData() const;
    Dock::ItemType itemType() const;
    // 有一些插件强依赖后端服务，需要等后端服务启动后再初始化，这里用一个标志位做个标记
    inline bool isInitialized() const { return m_initialized; };
    void markInited() { m_initialized = true; }

    // 通过 message 方法通讯
    bool supportFlag() const;
    void updateOverflowState(int state) const;
    void setAppletMinHeight(int minHeight) const;
    bool wantToBeLoaded() const;
    void setAppletContainer(int container) const;
    void updateDockPanelSize(const QSize &dockSize) const;
    QMap<QString, QVariant> pluginPropertyMap() const;

    // 在插件的右键菜单中，追加选项
    static QString addPluginContextMenu(const QString &menuJson, const QJsonObject &item);
    static bool showUndock(const QString &menuJson);

    static Dock::ThemeType dockThemeType() { return Dtk::Gui::DGuiApplicationHelper::instance()->themeType() == Dtk::Gui::DGuiApplicationHelper::LightType ? Dock::ThemeType_Light : Dock::ThemeType_Dark; }
    static PluginFlags flags(PluginsItemInterface* plugin, QPluginLoader* pluginLoader);
    static PluginsItemInterfaceV2* toV2(PluginsItemInterface* plugin) { return dynamic_cast<PluginsItemInterfaceV2*>(plugin); }

Q_SIGNALS:
    void pluginInfoChanged();
    void enableStateChanged(bool );

private:
    PluginFlags getFlags() const;

private:
    PluginsItemInterface* m_plugin;
    PluginsItemInterfaceV2* m_pluginV2;
    const QGSettings *m_moduleGsettings;
    QPointer<QPluginLoader> m_pluginLoader;
    PluginFlags m_flags;
    QStringList m_items;
    bool m_initialized;
};

#endif