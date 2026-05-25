qt_add_executable(hawkwm
    main.cpp
    compositor.cpp
    window.cpp
    ../include/compositor.h
    ../include/window.h
)

target_link_libraries(hawkwm PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
    Qt::Qml
    Qt::WaylandCompositor
)