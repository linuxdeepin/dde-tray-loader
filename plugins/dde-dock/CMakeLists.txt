# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: CC0-1.0

add_subdirectory("airplane-mode")
add_subdirectory("bluetooth")
add_subdirectory("brightness")
add_subdirectory("datetime")
add_subdirectory("dnd-mode")
add_subdirectory("eye-comfort-mode")
add_subdirectory("keyboard-layout")
add_subdirectory("media")
add_subdirectory("onboard")
add_subdirectory("power")
add_subdirectory("shutdown")
add_subdirectory("sound")
add_subdirectory("notification")

if (NOT (${CMAKE_BUILD_TYPE} MATCHES "Debug"))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast")
endif ()

## qm files
file(GLOB DOCK_TS_FILES "translations/*.ts")
file(GLOB_RECURSE CPP_SOURCE_FILES "*.cpp" "*.h")

qt_add_lupdate(
    TS_FILES ${DOCK_TS_FILES}
    SOURCES ${CPP_SOURCE_FILES}
    OPTIONS -no-obsolete -no-ui-lines -locations none
)

qt_add_lrelease(
    TS_FILES ${DOCK_TS_FILES}
    QM_FILES_OUTPUT_VARIABLE QM_FILES
)

install(FILES ${QM_FILES}
    DESTINATION ${CMAKE_INSTALL_DATADIR}/dde-dock/translations)
