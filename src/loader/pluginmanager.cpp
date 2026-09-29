// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pluginmanager.h"
#include "pluginsiteminterface_v3.h"
#include "pluginsiteminterface_v2.h"
#include "widgetplugin.h"

#include <QFileInfo>
#include <QPluginLoader>
#include <QElapsedTimer>
#include <QTimer>

PluginManager::PluginManager(QObject *parent)
    : QObject(parent)
{
}

void PluginManager::setPluginPaths(const QStringList &paths)
{
    m_pluginPaths = paths;
}

void PluginManager::loadPlugins()
{
    for (const QString &path : m_pluginPaths) {
        QFileInfo pathInfo(path);

        if (!pathInfo.exists()) {
            qWarning() << "Path does not exist:" << path;
            continue;
        }

        if (pathInfo.isDir()) {
            loadPluginsFromDir(path);
        } else if (pathInfo.isFile()) {
            m_pendingPluginPaths.append(path);
        } else {
            qWarning() << "Unsupported path type:" << path;
        }
    }

    QTimer::singleShot(0, this, &PluginManager::loadNextPlugin);
}

void PluginManager::loadNextPlugin()
{
    if (m_pendingPluginPaths.isEmpty()) {
        Q_EMIT loadingFinished(!m_loadedPlugins.isEmpty());
        return;
    }

    // Give already initialized plugins a chance to paint and dispatch Wayland
    // events before loading the next one. All widget work stays on this thread.
    Q_EMIT pluginLoadStarted();
    loadPlugin(m_pendingPluginPaths.takeFirst());
    Q_EMIT pluginLoadFinished();
    QTimer::singleShot(0, this, &PluginManager::loadNextPlugin);
}

QVector<PluginsItemInterface *> PluginManager::loadedPlugins() const
{
    return m_loadedPlugins;
}

void PluginManager::loadPlugin(const QString &pluginFilePath)
{
    QElapsedTimer timer;
    timer.start();
    QStringList blacklistedPluginPaths;
    // TODO: use dconfig for this purpose.
    if (qgetenv("XDG_SESSION_TYPE") == "wayland") {
        blacklistedPluginPaths.append(QStringList{
            "libshot-start-record-plugin.so",
            "libdeepin-screen-recorder-plugin.so",
            "libeye-comfort-mode.so",
        });
    }
    for (const QString &path : blacklistedPluginPaths) {
        if (pluginFilePath.endsWith(path)) {
            qDebug() << "Skipping blacklisted plugin:" << pluginFilePath;
            return;
        }
    }

    auto *pluginLoader = new QPluginLoader(pluginFilePath, this);
    const QJsonObject &meta = pluginLoader->metaData().value("MetaData").toObject();
    const QString &pluginApi = meta.value("api").toString();
    Q_UNUSED(pluginApi);

    if (!pluginLoader->load()) {
        qWarning() << "Failed to load plugin:" << pluginFilePath << pluginLoader->errorString();
        pluginLoader->deleteLater();
        return;
    }

    QObject *instance = pluginLoader->instance();
    PluginsItemInterface *interface = qobject_cast<PluginsItemInterfaceV3 *>(instance);
    if (!interface) {
        interface = qobject_cast<PluginsItemInterfaceV2 *>(instance);
    }
    if (!interface) {
        interface = qobject_cast<PluginsItemInterface *>(instance);
    }

    if (!interface) {
        qWarning() << "Failed to get valid interface from plugin:" << pluginFilePath;
        pluginLoader->unload();
        pluginLoader->deleteLater();
        return;
    }

    m_loadedPlugins.append(interface);
    qDebug() << "Loaded plugin:" << interface->pluginName();

    (void)new dock::WidgetPlugin(interface, this);
    qInfo() << "Initialized plugin:" << interface->pluginName() << "in" << timer.elapsed() << "ms";
}

void PluginManager::loadPluginsFromDir(const QString &dirPath)
{
    QDir pluginDir(dirPath);
    if (!pluginDir.exists()) {
        qWarning() << "Directory does not exist:" << dirPath;
        return;
    }

    QFileInfoList entries = pluginDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &entry : entries) {
        if (entry.isDir()) {
            loadPluginsFromDir(entry.absoluteFilePath());
        } else {
            if (entry.suffix() != "so") continue;
            m_pendingPluginPaths.append(entry.absoluteFilePath());
        }
    }
}
