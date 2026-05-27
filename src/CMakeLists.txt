qt_add_executable(hawkwm
    main.cpp
    compositor.cpp
    window.cpp
    appmenu.cpp
    ../include/compositor.h
    ../include/window.h
    ../include/appmenu.h
)

target_link_libraries(hawkwm PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
    Qt::Qml
    Qt::WaylandCompositor
)