include_directories(../../src/loader/interfaces)
include_directories(util)
include_directories(widgets)
include_directories(dbus)

add_subdirectory("datetime")
add_subdirectory("shutdown")
add_subdirectory("power")
add_subdirectory("sound")
add_subdirectory("keyboard-layout")
add_subdirectory("bluetooth")
add_subdirectory("airplane-mode")
add_subdirectory("dnd-mode")
add_subdirectory("eye-comfort-mode")
add_subdirectory("media")
add_subdirectory("brightness")

if (NOT (${CMAKE_BUILD_TYPE} MATCHES "Debug"))
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast")

    # generate qm
    execute_process(COMMAND bash "translate_generation.sh"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif ()

## qm files
file(GLOB QM_FILES "translations/*.qm")
install(FILES ${QM_FILES}
    DESTINATION share/dde-dock/translations)
