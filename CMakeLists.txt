cmake_minimum_required(VERSION 3.16...3.25)
project(hawkwm VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(XDG_SHELL_XML "/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml")
set(XDG_SHELL_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/xdg-shell-client-protocol.h")
set(XDG_SHELL_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/include/xdg-shell-protocol.c")

find_program(WAYLAND_SCANNER_EXE wayland-scanner)
if(NOT WAYLAND_SCANNER_EXE)
    message(FATAL_ERROR "wayland-scanner not found")
endif()

add_custom_command(
        OUTPUT "${XDG_SHELL_HEADER}"
        COMMAND ${WAYLAND_SCANNER_EXE} client-header "${XDG_SHELL_XML}" "${XDG_SHELL_HEADER}"
        DEPENDS "${XDG_SHELL_XML}"
        COMMENT "Generating Wayland XDG Shell client header..."
)

add_custom_command(
        OUTPUT "${XDG_SHELL_SOURCE}"
        COMMAND ${WAYLAND_SCANNER_EXE} private-code "${XDG_SHELL_XML}" "${XDG_SHELL_SOURCE}"
        DEPENDS "${XDG_SHELL_XML}"
        COMMENT "Generating Wayland XDG Shell private code source..."
)

add_custom_target(xdg-shell-generated
        DEPENDS "${XDG_SHELL_HEADER}" "${XDG_SHELL_SOURCE}")

# Load PkgConfig
find_package(PkgConfig REQUIRED)

# Find Wayland via pkg-config
pkg_check_modules(WaylandClient REQUIRED IMPORTED_TARGET wayland-client)
pkg_check_modules(WaylandServer REQUIRED IMPORTED_TARGET wayland-server)

# Find X11 components via pkg-config (Bypasses the broken FindX11 module)
pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)
pkg_check_modules(Xi REQUIRED IMPORTED_TARGET xi)
pkg_check_modules(Xrandr REQUIRED IMPORTED_TARGET xrandr)
pkg_check_modules(XCOMPOSITE REQUIRED IMPORTED_TARGET xcomposite)
pkg_check_modules(XDAMAGE REQUIRED IMPORTED_TARGET xdamage)

pkg_check_modules(WPE IMPORTED_TARGET wpe-1.0)
pkg_check_modules(WPE_FDO IMPORTED_TARGET wpe-fdo-1.0)

# Find remaining standard CMake packages
find_package(Libinput REQUIRED)
pkg_check_modules(WEBKIT REQUIRED IMPORTED_TARGET webkit2gtk-4.1)
pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)
pkg_check_modules(GTK3_X11 REQUIRED IMPORTED_TARGET gdk-x11-3.0)
pkg_check_modules(ATSPI REQUIRED IMPORTED_TARGET atspi-2)

include_directories(include)
add_subdirectory(src)