// Copyright (C) 2022 ~ 2022 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "abstractplugincontroller.h"
#include "pluginsiteminterface.h"
#include "settingmanager.h"

#include <DNotifySender>
#include <DSysInfo>

#include <QDebug>
#include <QDir>
#include <QMapIterator>
#include <QPluginLoader>

#define PLUGIN_INFO_KEY "pluginInfo"
#define PLUGIN_LOADER_KEY "pluginLoader"

static const QStringList CompatiblePluginApiList {
    "1.1.1",
    "1.2",
    "1.2.1",
    "1.2.2",
    DOCK_PLUGIN_API_VERSION
};

class PluginInfo : public QObject
{
public:
    PluginInfo() : QObject(nullptr), m_loaded(false), m_visible(false) {}
    bool m_loaded;
    bool m_visible;
    QString m_itemKey;
};

AbstractPluginController::AbstractPluginController(PluginProxyInterface *proxyInter, QObject *parent)
    : QObject(parent)
    , m_proxyInter(proxyInter)
{
    qApp->installEventFilter(this);
}

AbstractPluginController::~AbstractPluginController()
{
    qDeleteAll(m_itemsMap);
    m_itemsMap.clear();
}

void AbstractPluginController::saveValue(PluginsItemInterface *const itemInter, const QString &key, const QVariant &value)
{
    m_proxyInter->saveValue(itemInter, key, value);
}

const QVariant AbstractPluginController::getValue(PluginsItemInterface *const itemInter, const QString &key, const QVariant &fallback)
{
    if (!m_proxyInter) {
        qWarning() << "Get value failed, proxy interface is null";
        return QVariant();
    }
    return m_proxyInter->getValue(itemInter, key, fallback);
}

void AbstractPluginController::removeValue(PluginsItemInterface *const itemInter, const QStringList &keyList)
{
    m_proxyInter->removeValue(itemInter, keyList);
}

void AbstractPluginController::itemAdded(PluginsItemInterface * const itemInter, const QString &itemKey)
{
    qInfo() << "Item added, key:" << itemKey;
    // 如果插件已经加载，则无需再次加载（此处保证插件出现重复调用itemAdded的情况）
    auto wrapper = m_itemsMap.value(itemInter->pluginName());
    if (!wrapper || wrapper->isLoaded(itemKey)) {
        return;
    }

    auto loader = m_pluginLoaders.value(itemInter);
    if (!loader) {
        delete wrapper;
        qWarning() << "Loader is nullptr, add item failed, item key:" << itemKey;
        return;
    }

    wrapper->addItemKey(itemKey);
    Q_EMIT pluginInserted(itemKey);
}

void AbstractPluginController::itemUpdate(PluginsItemInterface * const itemInter, const QString &itemKey)
{
    m_proxyInter->itemUpdate(itemInter, itemKey);
}

void AbstractPluginController::itemRemoved(PluginsItemInterface * const itemInter, const QString &itemKey)
{
    // 更新字段中的isLoaded字段，表示当前没有加载
    auto pluginWrapper = m_itemsMap.value(itemInter->pluginName());
    if (pluginWrapper) {
        pluginWrapper->removeItem(itemKey);
    }

    m_proxyInter->itemRemoved(itemInter, itemKey);
    Q_EMIT pluginRemoved(itemKey);
}

void AbstractPluginController::requestWindowAutoHide(PluginsItemInterface * const itemInter, const QString &itemKey, const bool autoHide)
{
    m_proxyInter->requestWindowAutoHide(itemInter, itemKey, autoHide);
}

void AbstractPluginController::requestRefreshWindowVisible(PluginsItemInterface * const itemInter, const QString &itemKey)
{
    m_proxyInter->requestRefreshWindowVisible(itemInter, itemKey);
}

// 请求页面显示或者隐藏，由插件内部来调用，例如在移除蓝牙插件后，如果已经弹出了蓝牙插件的面板，则隐藏面板
void AbstractPluginController::requestSetAppletVisible(PluginsItemInterface * const itemInter, const QString &itemKey, const bool visible)
{
    PluginsItemInterface *pluginInter = itemInter;
    Q_EMIT requestAppletVisible(pluginInter, itemKey, visible);
}

void AbstractPluginController::displayModeChanged()
{
    const Dock::DisplayMode displayMode = qApp->property(PROP_DISPLAY_MODE).value<Dock::DisplayMode>();
    for (auto inter : m_pluginLoaders.keys())
        inter->displayModeChanged(displayMode);
}

void AbstractPluginController::positionChanged()
{
    const Dock::Position position = qApp->property(PROP_POSITION).value<Dock::Position>();
    for (auto inter : m_pluginLoaders.keys())
        inter->positionChanged(position);
}

bool AbstractPluginController::pluginCanDock(PluginsItemInterface *plugin) const
{
    if (!plugin) {
        qWarning() << "Plugin interface is null";
        return false;
    }

    return SettingManager::instance()->dockedPlugins().contains(plugin->pluginName());
}

void AbstractPluginController::addPluginLoader(QPluginLoader* pluginLoader)
{
    if (!pluginLoader || !pluginLoader->instance()) {
        qWarning() << "Loader is nullptr";
        return;
    }

    auto interface = qobject_cast<PluginsItemInterface*>(pluginLoader->instance());
    if (!interface) {
        auto interfaceV2 = qobject_cast<PluginsItemInterfaceV2*>(pluginLoader->instance());
        if (interfaceV2) {
            interface = dynamic_cast<PluginsItemInterface*>(interfaceV2);
        }
    }
    if (!interface) {
        qWarning() << "Get plugin interface pointer failed`, file name:" << pluginLoader->fileName();
        return;
    }

    qInfo() << "Add plugin loader, plugin name:" << interface->pluginName();
    m_itemsMap.insert(interface->pluginName(), new PluginWrapper(this, interface, pluginLoader));
    m_pluginLoaders.insert(interface, pluginLoader);
    interface->init(this);
    qInfo() << "Init plugin finished";
}

PluginWrapper *AbstractPluginController::pluginWrapper(const QString &itemKey) const
{
    for (const auto &wrapper : m_itemsMap.values()) {
        if (wrapper->isLoaded(itemKey))
            return wrapper;
    }

    return nullptr;
}

QList<PluginWrapper*> AbstractPluginController::pluginWrappers()
{
    QList<PluginWrapper*> plugins;
    for (auto wrapper : m_itemsMap.values()) {
        if (wrapper->isLoaded()) {
            plugins.push_back(wrapper);
        }
    }

    return plugins;
}

QMap<QString, PluginWrapper*> AbstractPluginController::pluginNameWrappers()
{
    QMap<QString, PluginWrapper*> tmp;
    for (auto wrapper : m_itemsMap) {
        if (!tmp.contains(wrapper->pluginName())) {
            tmp.insert(wrapper->pluginName(), wrapper);
        }
    }

    return tmp;
}
