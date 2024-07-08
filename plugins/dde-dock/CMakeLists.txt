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

    # generate qm
    execute_process(COMMAND bash "translate_generation.sh"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif ()

## qm files
file(GLOB QM_FILES "translations/*.qm")
install(FILES ${QM_FILES}
    DESTINATION share/dde-dock/translations)
