/*
 * HawkWM Composistor Copyright (C) 2026  First Person
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
 */

#include <transpiler.h>
#include <composistor.h>
#include <wpe/wpe.h>
#include <wpe/fdo.h>
#include <wpe/fdo-egl.h>

void renderWindow(DisplayVariant display, WindowVariant window) {
    wpe_loader_init("libWPEBackend-fdo-1.0.so");

}