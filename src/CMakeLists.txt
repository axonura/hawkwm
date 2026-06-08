set(HAWKWM_SOURCES
        main.cpp
        composistor.cpp
        ../include/composistor.h)

if(TARGET PkgConfig::WPE AND TARGET PkgConfig::WPE_FDO)
    list(APPEND HAWKWM_SOURCES transpiler.cpp)
else()
    message(WARNING "WPE or WPE_FDO not found; transpiler.cpp will not be built.")
endif()

add_executable(hawkwm ${HAWKWM_SOURCES})
add_dependencies(hawkwm xdg-shell-generated)

target_link_libraries(hawkwm PRIVATE
        PkgConfig::WaylandClient
        PkgConfig::X11
        PkgConfig::Xi
        PkgConfig::Xrandr
        Libinput::Libinput
        PkgConfig::WEBKIT
        PkgConfig::GTK3
        PkgConfig::GTK3_X11
        PkgConfig::ATSPI
)

if(TARGET PkgConfig::WPE AND TARGET PkgConfig::WPE_FDO)
    target_link_libraries(hawkwm PRIVATE
        PkgConfig::WPE
        PkgConfig::WPE_FDO
    )
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(hawkwm PRIVATE DEBUG_MODE)
endif()
