/*
 * Hawk Window Object  Copyright (C) 2026  First Person
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
 */

#ifndef WINDOW_H
#define WINDOW_H
#include <QOpenGLWindow>
#include <QOpenGLTextureBlitter>
#include "compositor.h"

class HCompositor;

class HWindow : public QOpenGLWindow {
    Q_OBJECT
public:
    HWindow();
    void setCompositor(HCompositor *hcmp);

signals:
    void glReady();

protected:
    void initializeGL() override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *) override;

    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;

private:
    QOpenGLTextureBlitter m_textureBlitter;
    HCompositor *m_compositor = nullptr;
};

#endif // WINDOW_H