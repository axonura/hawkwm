/*
 * Hawk Wayland Compositor  Copyright (C) 2026  First Person
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
 */

#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include <QtWaylandCompositor/QWaylandCompositor>
#include <QtWaylandCompositor/QWaylandSurface>
#include <QtWaylandCompositor/QWaylandView>
#include <QtWaylandCompositor/QWaylandOutput>
#include <QtWaylandCompositor/QWaylandIviApplication>
#include <QtWaylandCompositor/QWaylandIviSurface>
#include <QWaylandXdgShell>
#include <QWaylandQuickItem>
#include <QtWaylandCompositor/QWaylandSeat>
#include <QtCore/QPointer>
#include "window.h"

class HWindow;
class HView;

class HView : public QWaylandView {
    Q_OBJECT
private:
    QOpenGLTexture *m_texture = nullptr;
    bool m_positionSet = false;
    QPoint m_pos;
    int m_iviId;
public:
    explicit HView(QObject *parent = nullptr) : QWaylandView(parent) {}
    QOpenGLTexture *getTexture();
    int iviId() const {
        return m_iviId;
    }

    QRect globalGeometry() {
        return QRect(globalPosition(), surface()->destinationSize());
    }
    
    void setGlobalPosition(const QPoint &globalPos) {
        m_pos = globalPos;
        m_positionSet = true;
    }

    QPoint globalPosition() const {
        return m_pos;
    }

    QPoint mapToLocal(const QPoint &globalPos) const;
    QSize size() const {
        return surface() ? surface()->destinationSize() : QSize();
    }

    void initPosition(const QSize &screenSize, const QSize &surfaceSize);
};

class HCompositor : public QWaylandCompositor {
    Q_OBJECT
private:
    HWindow *m_window = nullptr;
    QWaylandXdgShell *m_xdgShell = nullptr;
    QWaylandIviApplication *m_iviApplication = nullptr;
    QList<HView*> m_views;
    QPointer<HView> m_mouseView;
    QList<QWaylandQuickItem*> m_windows;

public:
    // Declaration only: Implementation details reside entirely in compositor.cpp
    explicit HCompositor(HWindow *window, QObject *parent = nullptr);
    ~HCompositor();

    void create();
    HView* viewAt(const QPoint &pos);
    void raise(HView *view);
    const QList<HView*> &views() const { return m_views; }
    
    // Input handling interface
    void handleMousePress(const QPoint &pos, Qt::MouseButton btn);
    void handleMouseRelease(const QPoint &pos, Qt::MouseButton btn, Qt::MouseButtons btns);
    void handleMouseMove(const QPoint &pos);
    void handleMouseWheel(const QPoint &angleDelta);
    void handleKeyPress(quint32 nScanCode);
    void handleKeyRelease(quint32 nScanCode);
    
    void startRender();
    void endRender();
private slots:
    void onIviSurfaceCreated(QWaylandIviSurface *iviSurface);
    void viewSurfaceDestroyed();
    void triggerRender();
    void onTopLevelCreated(QWaylandXdgToplevel *toplevel, QWaylandXdgSurface *xdgSurface);
};

#endif // COMPOSITOR_H