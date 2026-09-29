// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

class PluginsItemInterface;

class PluginManager : public QObject
{
    Q_OBJECT
public:
    explicit PluginManager(QObject *parent = nullptr);

    void setPluginPaths(const QStringList &paths);
    void loadPlugins();
    QVector<PluginsItemInterface *> loadedPlugins() const;

Q_SIGNALS:
    void pluginLoadStarted();
    void pluginLoadFinished();
    void loadingFinished(bool success);

private:
    void loadNextPlugin();
    void loadPlugin(const QString &pluginFilePath);
    void loadPluginsFromDir(const QString &dirPath);

private:
    QStringList m_pluginPaths;
    QStringList m_pendingPluginPaths;
    QVector<PluginsItemInterface *> m_loadedPlugins;
};
