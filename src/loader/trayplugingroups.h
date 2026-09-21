// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QStringList>

namespace loader {

// Well-known plugin group names. They match the keys of the
// org.deepin.dde.dock.plugin.groups DConfig schema (owned by dde-tray-loader),
// which assigns each plugin .so to the process group it should be loaded in.
inline constexpr auto SelfMaintenanceGroup = "selfMaintenanceTrayPlugins";
inline constexpr auto SubprojectGroup = "subprojectTrayPlugins";
inline constexpr auto CrashProneGroup = "crashProneTrayPlugins";
inline constexpr auto OtherGroup = "otherTrayPlugins";

// Whether <groupName> is one of the well-known group names. Used to reject
// typos in manual invocations early instead of silently loading nothing.
bool isValidGroup(const QString &groupName);

// Scan the standard dde-dock plugin directories and return the absolute
// paths of all plugins that belong to the given group according to the
// DConfig grouping configuration. Plugins not listed in any configured
// group fall into OtherGroup.
//
// When <ok> is non-null, it is set to false if the grouping config could
// not be loaded (so callers can distinguish a genuine empty group from a
// config failure), and to true otherwise.
QStringList pluginPathsForGroup(const QString &groupName, bool *ok = nullptr);

}
