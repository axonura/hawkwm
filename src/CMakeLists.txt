add_executable(hawkwm
        main.cpp
        composistor.cpp
        ../include/composistor.h
)
add_dependencies(hawkwm xdg-shell-generated)

target_link_libraries(hawkwm PRIVATE
        PkgConfig::WaylandClient
        PkgConfig::X11
        PkgConfig::Xi
        PkgConfig::Xrandr
        PkgConfig::XCOMPOSITE
        PkgConfig::XDAMAGE
        Libinput::Libinput
        PkgConfig::WEBKIT
        PkgConfig::GTK3
        PkgConfig::GTK3_X11
        PkgConfig::ATSPI
)

if(DEFINED WPE_FOUND AND DEFINED WPE_FDO_FOUND AND WPE_FOUND AND WPE_FDO_FOUND)
    target_link_libraries(hawkwm PRIVATE
        PkgConfig::WPE
        PkgConfig::WPE_FDO
    )
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(hawkwm PRIVATE DEBUG_MODE)
endif()
