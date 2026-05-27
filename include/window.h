/*
 * Hawk Window Object  Copyright (C) 2026  First Person
 * This progmnmnram comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
 */

#ifndef WINDOW_H
#define WINDOW_H
#include <QOpenGLWindow>
#include <QOpenGLTextureBlitter>
#include "compositor.h"

class HCompositor;
class HAppMenu;
class QOpenGLTexture;

struct ManagedWindow {
    QWaylandView *view;
    QRect geometry; // Position (x, y) and size (width, height) on screen
};

class HWindow : public QOpenGLWindow {
    Q_OBJECT
public:
    HWindow();
    void setCompositor(HCompositor *hcmp);
    void handleXdgToplevelCreated(QWaylandXdgToplevel *toplevel, QWaylandXdgSurface *surface);

    void setTopLevel(QWaylandXdgToplevel *topLevel) { m_topLevel = topLevel; }
    QWaylandXdgToplevel *getTopLevel() {
        return m_topLevel;
    }
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

    QWaylandView* findViewAt(const QPointF &pos);
    QPointF getViewOffset(QWaylandView *view);
#ifdef HAWKWM_DEBUG
    void toggleAppMenu();
    QPointF menuRenderPos() const;
#endif

private:
    QOpenGLTextureBlitter m_textureBlitter;
    HCompositor *m_compositor = nullptr;
    QWaylandSeat *m_seat = nullptr;
    QList<ManagedWindow> m_windowList;
    QWaylandXdgToplevel *m_topLevel = nullptr; // Identify The Window On Server
#ifdef HAWKWM_DEBUG
    HAppMenu *m_appMenu = nullptr;
    QOpenGLTexture *m_menuTexture = nullptr;
    bool m_menuVisible = false;
    bool m_menuDirty = false;
#endif
};

#endif // WINDOW_H