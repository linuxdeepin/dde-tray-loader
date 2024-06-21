// Copyright (C) 2022 ~ 2022 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef ABSTRACTPLUGINCONTROLLER_H
#define ABSTRACTPLUGINCONTROLLER_H

#include "pluginproxyinterface.h"
#include "pluginsiteminterface_v2.h"
#include "plugin-wrapper.h"

#include <QList>
#include <QMap>
#include <QDBusConnectionInterface>

class PluginsItemInterface;

// TODO 和 QuickPluginController 、 DockPluginController 整合一下
class AbstractPluginController : public QObject, public PluginProxyInterface
{
    Q_OBJECT

public:
    AbstractPluginController() = default;
    explicit AbstractPluginController(PluginProxyInterface *proxyInter, QObject *parent = Q_NULLPTR);
    ~AbstractPluginController();

    void setProxyInter(PluginProxyInterface *proxyInter) { m_proxyInter = proxyInter; }
    void addPluginLoader(QPluginLoader* pluginLoader);
    PluginWrapper *pluginWrapper(const QString &itemKey) const;
    QList<PluginWrapper *> pluginWrappers();
    QMap<QString, PluginWrapper*> pluginNameWrappers();

public:
    // Implements PluginProxyInterface
    virtual void itemAdded(PluginsItemInterface * const itemInter, const QString &itemKey) override;
    virtual void itemUpdate(PluginsItemInterface * const itemInter, const QString &itemKey) override;
    virtual void itemRemoved(PluginsItemInterface * const itemInter, const QString &itemKey) override;
    virtual void requestWindowAutoHide(PluginsItemInterface * const itemInter, const QString &itemKey, const bool autoHide) override;
    virtual void requestRefreshWindowVisible(PluginsItemInterface * const itemInter, const QString &itemKey) override;
    virtual void requestSetAppletVisible(PluginsItemInterface * const itemInter, const QString &itemKey, const bool visible) override;
    virtual void saveValue(PluginsItemInterface *const itemInter, const QString &key, const QVariant &value) override;
    virtual const QVariant getValue(PluginsItemInterface *const itemInter, const QString &key, const QVariant& fallback = QVariant()) override;
    virtual void removeValue(PluginsItemInterface * const itemInter, const QStringList &keyList) override;

Q_SIGNALS:
    void pluginInserted(const QString &itemKey);
    void pluginRemoved(const QString &itemKey);
    void requestAppletVisible(PluginsItemInterface *, const QString &, bool);

protected:
    bool pluginCanDock(PluginsItemInterface *plugin) const;

protected Q_SLOTS:
    void displayModeChanged();
    void positionChanged();

private:
    // QMap<插件指针,插件 loader 指针(主要是为了获取 元数据和元对象)>
    QMap<PluginsItemInterface*, QPluginLoader*> m_pluginLoaders;
    // QMap<itemKey, item 相关数据>
    QMap<QString, QPointer<PluginWrapper>> m_itemsMap;
    PluginProxyInterface *m_proxyInter;
};

#endif // DOCKPLUGINCONTROLLER_H
