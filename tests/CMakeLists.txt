# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: CC0-1.0

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets DBus)

foreach(stage RANGE 1 2)
    add_library(startup-plugin-${stage} MODULE startupplugin.cpp)
    target_include_directories(startup-plugin-${stage} PRIVATE ../interfaces)
    target_compile_definitions(startup-plugin-${stage} PRIVATE TEST_PLUGIN_STAGE=${stage})
    target_link_libraries(startup-plugin-${stage} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
endforeach()

add_test(NAME tray-plugin-event-dispatch
    COMMAND trayplugin-loader -platform offscreen -p
        "$<TARGET_FILE:startup-plugin-1>;$<TARGET_FILE:startup-plugin-2>")
# The real loader initializes file logging before loading either test plugin.
# Package builders may have no writable home; keep its cache in this build.
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tray-plugin-event-dispatch-cache")
set_tests_properties(tray-plugin-event-dispatch PROPERTIES
    ENVIRONMENT "USE_DEFAULT_PLATFORMTHEME=1;QT_LOGGING_RULES=*.debug=false;DSG_DCONFIG_BACKEND_TYPE=FileBackend;WAYLAND_DISPLAY=tray-test-outer;QT_WAYLAND_SHELL_INTEGRATION=tray-test-shell;DSG_APP_ID=tray-test-app;QT_IM_MODULE=tray-test-im;QT_SCALE_FACTOR=2"
    TIMEOUT 15)
# Debian disables build RPATH, and the interface library is not installed when
# dh_auto_test runs. Always test the library from this build, not a system copy.
set_property(TEST tray-plugin-event-dispatch APPEND PROPERTY ENVIRONMENT
    "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:dockpluginmanager-interface>"
    "XDG_CACHE_HOME=${CMAKE_CURRENT_BINARY_DIR}/tray-plugin-event-dispatch-cache")

# Manual integration test: needs an isolated D-Bus session, not a desktop.
# Keep it out of package-build tests, like test_systemd_lifecycle.py.
find_package(Dtk${DTK_VERSION_MAJOR} REQUIRED COMPONENTS Core)
add_executable(power-dbus-cache-test EXCLUDE_FROM_ALL
    powerdbuscache.cpp
    ../plugins/dde-dock/power/dbus/dbuspower.cpp
    ../plugins/dde-dock/power/dbus/dbuspower.h)
target_include_directories(power-dbus-cache-test PRIVATE ../plugins/dde-dock/power/dbus)
target_link_libraries(power-dbus-cache-test PRIVATE Qt${QT_VERSION_MAJOR}::DBus Dtk${DTK_VERSION_MAJOR}::Core)
