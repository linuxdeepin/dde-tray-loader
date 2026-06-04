# SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

set(PLUGIN_NAME "application-tray")

project(${PLUGIN_NAME})

find_package(PkgConfig REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED Core Gui Widgets DBus WaylandClient)
if(Qt${QT_VERSION_MAJOR}_VERSION VERSION_GREATER_EQUAL 6.10)
  find_package(Qt${QT_VERSION_MAJOR} COMPONENTS GuiPrivate WaylandClientPrivate REQUIRED)
endif()
find_package(Dtk${DTK_VERSION_MAJOR} REQUIRED Core Gui Widget Tools)
find_package(KF6WindowSystem 6.6 REQUIRED) # for x11 tray selection owner

pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11 xcb xcb-image xcb-damage xcb-composite xcb-xfixes xcb-util xcb-shape xtst xcb-xtest xcb-res xcb-ewmh)
pkg_check_modules(WaylandClient REQUIRED IMPORTED_TARGET wayland-client)

set(TRAY_SOURCES
    api/types/dbusimagelist.cpp api/types/dbusimagelist.h
    api/types/dbustooltip.cpp api/types/dbustooltip.h
    api/types/traylist.h

    abstracttrayprotocol.h
    ddeindicatortrayprotocol.cpp ddeindicatortrayprotocol.h
    sniprotocolhandler.cpp sniprotocolhandler.h
    trayplugin.cpp trayplugin.h
    traywidget.cpp traywidget.h
    util.cpp util.h
    xembedprotocolhandler.cpp xembedprotocolhandler.h

    c_ptr.h
    traymanager1.cpp traymanager1.h
    fdoselectionmanager.cpp fdoselectionmanager.h

    ${CMAKE_SOURCE_DIR}/src/tray-wayland-integration/xdgactivationclient.h
    ${CMAKE_SOURCE_DIR}/src/tray-wayland-integration/xdgactivationclient.cpp
)

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/api/dbus/org.deepin.dde.TrayManager1.xml
    PROPERTIES  INCLUDE api/types/traylist.h
                CLASSNAME TrayManager
)

qt_add_dbus_adaptor(TRAY_SOURCES
    api/dbus/org.deepin.dde.TrayManager1.xml traymanager1.h TrayManager1
)

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/api/dbus/org.kde.StatusNotifierItem.xml
    PROPERTIES  INCLUDE api/types/dbusimagelist.h
                INCLUDE api/types/dbustooltip.h
                CLASSNAME StatusNotifierItem
)

qt_add_dbus_interfaces(
    DBUS_INTERFACES
    ${CMAKE_CURRENT_SOURCE_DIR}/api/dbus/org.deepin.dde.TrayManager1.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/api/dbus/org.kde.StatusNotifierWatcher.xml
)

dtk_add_dbus_interfaces(
    DBUS_INTERFACES
    ${CMAKE_CURRENT_SOURCE_DIR}/api/dbus/org.kde.StatusNotifierItem.xml
)

add_library(${PLUGIN_NAME} SHARED
    ${DBUS_INTERFACES}
    ${TRAY_SOURCES}
    application-tray.qrc
)

qt_generate_wayland_protocol_client_sources(${PLUGIN_NAME}
NO_INCLUDE_CORE_ONLY
FILES
    ${WAYLAND_PROTOCOLS_DATADIR}/staging/xdg-activation/xdg-activation-v1.xml
)

target_compile_definitions(${PLUGIN_NAME} PRIVATE "QT_PLUGIN")

target_link_libraries(${PLUGIN_NAME}
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::CorePrivate
        Qt${QT_VERSION_MAJOR}::WidgetsPrivate
        Qt${QT_VERSION_MAJOR}::GuiPrivate
        Qt${QT_VERSION_MAJOR}::WaylandClient
        Qt${QT_VERSION_MAJOR}::WaylandClientPrivate
        Wayland::Client
        Dtk${DTK_VERSION_MAJOR}::Core
        Dtk${DTK_VERSION_MAJOR}::Gui
        Dtk${DTK_VERSION_MAJOR}::Widget
        KF6::WindowSystem
        PkgConfig::X11
        PkgConfig::WaylandClient
        dbusmenuqt
        dockpluginmanager-interface
)

target_include_directories(${PLUGIN_NAME} PUBLIC
    "${CMAKE_SOURCE_DIR}/interfaces/"
    "${CMAKE_SOURCE_DIR}/src/tray-wayland-integration/"
    "${CMAKE_CURRENT_SOURCE_DIR}/../libdbusmenuqt/"
    "${CMAKE_SOURCE_DIR}/plugins/application-tray/api/"
)
set_target_properties(${PLUGIN_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins/dde-dock)
# 设置执行 make install 时哪个目标应该被 install 到哪个位置
install(TARGETS ${PLUGIN_NAME} LIBRARY DESTINATION lib/dde-dock/plugins)