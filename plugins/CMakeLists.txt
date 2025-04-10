# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: CC0-1.0


# FIXME: copy from dde-network-core need to fix dde-network-core and remove those code
find_package(PkgConfig REQUIRED)

pkg_check_modules(Gio REQUIRED gio-2.0)
include_directories(${Gio_INCLUDE_DIRS})

pkg_search_module(GLIB REQUIRED glib-2.0)
include_directories(${GLIB_INCLUDE_DIRS})
link_directories(${GLIB_LIBRARY_DIRS})

find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} COMPONENTS Core Widgets DBus Network LinguistTools REQUIRED)

add_subdirectory(application-tray)
add_subdirectory(dde-dock)
add_subdirectory(libdbusmenuqt)
add_subdirectory(dde-network-display-ui)
