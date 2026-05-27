/*
 * Hawk Wayland Compositor  Copyright (C) 2026  First Person
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
 */

#include <QtWaylandCompositor/QWaylandCompositor>
#include <QtWaylandCompositor/QWaylandSurface>
#include <QtWaylandCompositor/QWaylandView>
#include <QtWaylandCompositor/QWaylandOutput>
#include <QtWaylandCompositor/QWaylandIviApplication>
#include <QtWaylandCompositor/QWaylandIviSurface>
#include <QtWaylandCompositor/QWaylandSeat>
#include <QtCore/QPointer>
#include <QtCore/QRandomGenerator>
#include <QWaylandXdgShell>
#include <QWaylandQuickItem>
#include <QWaylandPointer>
#include <compositor.h>
#include <window.h>

QOpenGLTexture *HView::getTexture() {
    if(advance())
        m_texture = currentBuffer().toOpenGLTexture();
    else
        return nullptr;
    return m_texture;
}

QPoint HView::mapToLocal(const QPoint &globalPos) const {
    return globalPos - globalPosition();
}

// Normally, an IVI based compositor would have a design where each window has
// a defined position, based on the id. In this example, we just assign a random position.
void HView::initPosition(const QSize &screenSize, const QSize &surfaceSize) {
    if(m_positionSet)
        return;
    QRandomGenerator rand{iviId()};
    int xrange = qMax(qMax(screenSize.width(), surfaceSize.width()), 1);
    int yrange = qMax(screenSize.height() - surfaceSize.height(), 1);
    setGlobalPosition(QPoint(rand.bounded(xrange), rand.bounded(yrange)));
}

HCompositor::HCompositor(HWindow *window, QObject *parent) : QWaylandCompositor(parent), m_window(window) {
    window->setCompositor(this);
    connect(window, &HWindow::glReady, this, [this] { create(); });

    m_xdgShell = new QWaylandXdgShell(this);
    connect(m_xdgShell, &QWaylandXdgShell::toplevelCreated, this, &HCompositor::onTopLevelCreated);
}

HCompositor::~HCompositor() {
}

void HCompositor::onTopLevelCreated(QWaylandXdgToplevel *toplevel, QWaylandXdgSurface *xdgSurface) {
    QWaylandSurface *csurface = xdgSurface->surface();
    HWindow *window = new HWindow();
    window->setTopLevel(toplevel);
    window->handleXdgToplevelCreated(toplevel, xdgSurface);

    HView *view = new HView(this);
    view->setSurface(csurface);
    view->setGlobalPosition(QPoint(100, 100)); // I Will Be Implement Dynamic Posistioning
    m_views.append(view);
    m_windows.append(window);

    // Find Window Parent Then Connect With The Parent On Server
    for (HWindow *win : m_windows) {
        if (toplevel == win->getTopLevel()) {
            win->handleXdgToplevelCreated(toplevel, xdgSurface);
            break;
        }
    }

    connect(toplevel, &QObject::destroyed, this, [=]() {
        m_views.removeOne(view);
        m_windows.removeOne(window);
        view->deleteLater();
        delete window;
    });

    connect(toplevel, &QWaylandXdgToplevel::activatedChanged, this,
        [=]() { if (toplevel->activated()) raise(view); });
}

void HCompositor::create() {
    QWaylandOutput *output = new QWaylandOutput(this, m_window);
    QWaylandOutputMode mode(m_window->size(), 60000);
    output->addMode(mode, true);
    QWaylandCompositor::create();
    output->setCurrentMode(mode);

    m_iviApplication = new QWaylandIviApplication(this);
    connect(m_iviApplication, &QWaylandIviApplication::iviSurfaceCreated, this, &HCompositor::onIviSurfaceCreated);
}

HView *HCompositor::viewAt(const QPoint &pos) {
    for(auto it = m_views.crbegin(); it != m_views.crend(); ++it) {
        HView *view = *it;
        if(view->globalGeometry().contains(pos))
            return view;
    }
    return nullptr;
}

void HCompositor::raise(HView *view) {
    m_views.removeAll(view);
    m_views.append(view);
    defaultSeat()->setKeyboardFocus(view->surface());
    triggerRender();
}

static inline QPoint mapToView(const HView *view, const QPoint &position)
{
    return view ? view->mapToLocal(position) : position;
}

void HCompositor::handleMousePress(const QPoint &pos, Qt::MouseButton btn) {
    if(!m_mouseView) {
        if((m_mouseView = viewAt(pos)))
            raise(m_mouseView);
    }
    auto *seat = defaultSeat();
    seat->sendMouseMoveEvent(m_mouseView, mapToView(m_mouseView, pos));
    seat->sendMousePressEvent(btn);
}

void HCompositor::handleMouseRelease(const QPoint &pos, Qt::MouseButton btn, Qt::MouseButtons btns) {
    auto *seat = defaultSeat();
    seat->sendMouseMoveEvent(m_mouseView, mapToView(m_mouseView, pos));
    seat->sendMouseReleaseEvent(btn);

    if(btns == Qt::NoButton) {
        HView *newView = viewAt(pos);
        if(newView != m_mouseView)
            seat->sendMouseMoveEvent(newView, mapToView(newView, pos));
        m_mouseView = nullptr;
    }
}

void HCompositor::handleMouseMove(const QPoint &pos) {
    HView *view = m_mouseView ? m_mouseView.data() : viewAt(pos);
    defaultSeat()->sendMouseMoveEvent(view, mapToView(view, pos));
}

void HCompositor::handleMouseWheel(const QPoint &angleDelta) {
    // TODO: fix this to send a single event, when diagonal scrolling is supported
    if(angleDelta.x() != 0)
        defaultSeat()->sendMouseWheelEvent(Qt::Horizontal, angleDelta.x());
    if(angleDelta.y() != 0)
        defaultSeat()->sendMouseWheelEvent(Qt::Vertical, angleDelta.y());
}

void HCompositor::handleKeyPress(quint32 nScanCode) {
    defaultSeat()->sendKeyPressEvent(nScanCode);
}

void HCompositor::handleKeyRelease(quint32 nScanCode) {
    defaultSeat()->sendKeyReleaseEvent(nScanCode);
}

void HCompositor::onIviSurfaceCreated(QWaylandIviSurface *iviSurface) {
    HView *view = new HView(); 
    view->setSurface(iviSurface->surface());
    view->setOutput(outputFor(m_window));

    m_views << view;
    connect(view, &QWaylandView::surfaceDestroyed, this, &HCompositor::viewSurfaceDestroyed);
    connect(iviSurface->surface(), &QWaylandSurface::redraw, this, &HCompositor::triggerRender);
}

void HCompositor::viewSurfaceDestroyed() {
    HView *view = qobject_cast<HView*>(sender());
    m_views.removeAll(view);
    delete view;
    triggerRender();
}

void HCompositor::triggerRender() {
    m_window->requestUpdate();
}

void HCompositor::startRender() {
    QWaylandOutput *out = defaultOutput();
    if(out)
        out->frameStarted();
}

void HCompositor::endRender() {
    QWaylandOutput *out = defaultOutput();
    if(out)
        out->sendFrameCallbacks();
}