// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "trayplugingroups.h"

#include <DConfig>

#include <QDir>
#include <QLoggingCategory>
#include <QScopedPointer>

Q_LOGGING_CATEGORY(loaderLog, "org.deepin.dde.tray-loader")

namespace loader {

namespace {

const QStringList &pluginDirs()
{
    // Keep in sync with the directories previously scanned by dde-shell
    // (panels/dock/loadtrayplugins.h).
    static const QStringList dirs = {
        "/usr/lib/dde-dock/plugins/",
        "/usr/lib/dde-dock/plugins/quick-trays/",
        "/usr/lib/dde-dock/plugins/system-trays/"
    };
    return dirs;
}

QStringList allPluginPaths()
{
    QStringList dirs;
    const auto debugPaths = qEnvironmentVariable("TRAY_DEBUG_PLUGIN_PATH");
    if (!debugPaths.isEmpty())
        dirs << debugPaths.split(QDir::listSeparator());

    if (dirs.isEmpty())
        dirs << pluginDirs();

    QStringList pluginPaths;
    for (const auto &pluginDir : std::as_const(dirs)) {
        QDir dir(pluginDir);
        if (!dir.exists()) {
            qCWarning(loaderLog) << "The plugin directory does not exist:" << pluginDir;
            continue;
        }

        const auto pluginFileInfos = dir.entryInfoList({"*.so"}, QDir::Files);
        for (const auto &pluginInfo : pluginFileInfos) {
            pluginPaths.append(pluginInfo.absoluteFilePath());
        }
    }

    return pluginPaths;
}

} // namespace

bool isValidGroup(const QString &groupName)
{
    return groupName == SelfMaintenanceGroup
        || groupName == SubprojectGroup
        || groupName == CrashProneGroup
        || groupName == OtherGroup;
}

QStringList pluginPathsForGroup(const QString &groupName, bool *ok)
{
    // The grouping schema is owned by dde-tray-loader itself (appid
    // org.deepin.dde.tray-loader, config org.deepin.dde.dock.plugin.groups).
    // It supersedes the grouping keys that previously lived in dde-shell's
    // org.deepin.ds.dock.tray schema.
    QScopedPointer<Dtk::Core::DConfig> dConfig(Dtk::Core::DConfig::create(
            "org.deepin.dde.tray-loader", "org.deepin.dde.dock.plugin.groups", QString()));
    if (!dConfig || !dConfig->isValid()) {
        qCWarning(loaderLog) << "Failed to load group config for" << groupName;
        if (ok) *ok = false;
        return {};
    }
    if (ok) *ok = true;

    const QStringList crashPronePlugins = dConfig->value(CrashProneGroup).toStringList();
    const QStringList selfMaintenancePlugins = dConfig->value(SelfMaintenanceGroup).toStringList();
    const QStringList subprojectPlugins = dConfig->value(SubprojectGroup).toStringList();

    QStringList groupPluginPaths;
    for (const auto &filePath : allPluginPaths()) {
        const QString pluginName = filePath.section("/", -1);
        // Each plugin belongs to exactly one group, with crash isolation taking priority.
        const char *assignedGroup = OtherGroup;
        if (crashPronePlugins.contains(pluginName))
            assignedGroup = CrashProneGroup;
        else if (selfMaintenancePlugins.contains(pluginName))
            assignedGroup = SelfMaintenanceGroup;
        else if (subprojectPlugins.contains(pluginName))
            assignedGroup = SubprojectGroup;

        if (groupName == assignedGroup)
            groupPluginPaths.append(filePath);
    }

    return groupPluginPaths;
}

}
