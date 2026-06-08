/*
 * HawkWM Composistor Copyright (C) 2026  First Person
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
 */

#include <iostream>
#include <cstdlib>
#include <wayland-server.h>
#include <wayland-client.h>
#include <composistor.h>

DisplayProtocol currentDisplayProtocol() {
    if (std::getenv("WAYLAND_DISPLAY") != nullptr)
        return DisplayProtocol::Wayland;

    if (const char* session_type = std::getenv("XDG_SESSION_TYPE")) {
        std::string_view type_str(session_type);
        if (type_str == "wayland") return DisplayProtocol::Wayland;
        if (type_str == "x11" || type_str == "xorg") return DisplayProtocol::Xorg;
    }

    if (std::getenv("DISPLAY") != nullptr)
        return DisplayProtocol::Xorg;

    std::cout << "No Supported Display Protocol Found.";
    std::exit(750);
}

int main(int argc, char **argv) {
    HComposistor hcomp = HComposistor(currentDisplayProtocol());
    hcomp.loop();
    return 0;
}