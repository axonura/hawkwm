/*
 * Hawk Window Manager  Copyright (C) 2026  First Person
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
 */

#include <QApplication>
#include <QQmlApplicationEngine>
#include <compositor.h>
#include <window.h>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    HWindow root;
    root.resize(800,600);
    HCompositor compositor(&root);
    root.show();

    return app.exec();
}