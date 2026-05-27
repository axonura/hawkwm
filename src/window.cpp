/*
 * Hawk Window Object  Copyright (C) 2026  First Person
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
 */

#include <QOpenGLWindow>
#include <QOpenGLTextureBlitter>
#include <QOpenGLTexture>
#include <QOpenGLFunctions>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QWaylandCompositor>
#include <QKeyEvent>
#include <compositor.h>
#include <window.h>

#ifdef HAWKWM_DEBUG
#include <appmenu.h>
#endif

HWindow::HWindow() {
    qDebug("Window Is Initialized");
    if (m_seat == nullptr)
        m_seat = new QWaylandSeat(m_compositor, QWaylandSeat::Pointer | QWaylandSeat::Keyboard | QWaylandSeat::Touch);
}

void HWindow::initializeGL() {
    m_textureBlitter.create();
    emit glReady();
}

void HWindow::paintGL() {
    m_compositor->startRender();

    QOpenGLFunctions *functions = context()->functions();
    functions->glClearColor(.4f, .7f, .1f, .5f);
    functions->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GLenum currTarget = GL_TEXTURE_2D;
    m_textureBlitter.bind(currTarget);
    functions->glEnable(GL_BLEND);
    functions->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const auto views = m_compositor->views();
    for(HView *view : views) {
        auto texture = view->getTexture();
        if(!texture)
            continue;
        if(texture->target() != currTarget) {
            currTarget = texture->target();
            m_textureBlitter.bind(currTarget);
        }

        GLuint textureId = texture->textureId();
        QWaylandSurface *surface = view->surface();
        if(surface && surface->hasContent()) {
            QSize s = surface->destinationSize();
            QPointF pos = view->globalPosition();
            QRectF surfaceGeometry(pos, s);
            QOpenGLTextureBlitter::Origin surfactOrigin =
                view->currentBuffer().origin() == QWaylandSurface::OriginTopLeft
                ? QOpenGLTextureBlitter::OriginTopLeft : QOpenGLTextureBlitter::OriginBottomLeft;
            
            QMatrix4x4 trgTransform = QOpenGLTextureBlitter::targetTransform(surfaceGeometry, QRect(QPoint(), size()));
            m_textureBlitter.blit(textureId, trgTransform, surfactOrigin);
        }
    }

    m_textureBlitter.release();
    m_compositor->endRender();
}

void HWindow::handleXdgToplevelCreated(QWaylandXdgToplevel *toplevel, QWaylandXdgSurface *surface) {
    ManagedWindow win;

    win.view = new QWaylandView(surface->surface()); // Wrap the surface in a view
    win.geometry = QRect(100, 100, 800, 600);       // Position it at x=100, y=100

    m_windowList.append(win);
}

void HWindow::mousePressEvent(QMouseEvent *event) {
    m_compositor->handleMousePress(event->position().toPoint(), event->button());
}

void HWindow::setCompositor(HCompositor *hcmp) {
    m_compositor = hcmp;
}

void HWindow::mouseMoveEvent(QMouseEvent *event) {
    if (m_seat && m_seat->pointer()) {
        QWaylandView *hvrView = findViewAt(event->position());
        if (hvrView) {
            QPointF rltvClientPos = event->position() - getViewOffset(hvrView);

            m_seat->sendMouseMoveEvent(hvrView, rltvClientPos);
        }
        else {
            m_seat->setMouseFocus(nullptr);
            qDebug() << "Pointer Unfocused.";
        }
    }
    //m_compositor->handleMouseMove(event->position().toPoint());
    QWindow::mouseMoveEvent(event);
}

void HWindow::mouseReleaseEvent(QMouseEvent *event) {
    m_compositor->handleMouseRelease(event->position().toPoint(), event->button(), event->buttons());
}

void HWindow::wheelEvent(QWheelEvent *e) {
    m_compositor->handleMouseWheel(e->angleDelta());
}

void HWindow::keyPressEvent(QKeyEvent *e) {
#ifdef HAWKWM_DEBUG
    if (e->key() == Qt::Key_F12) {
        toggleAppMenu();
        return;
    }
#endif
    m_compositor->handleKeyPress(e->nativeScanCode());
}

#ifdef HAWKWM_DEBUG
void HWindow::toggleAppMenu() {
    if (!m_appMenu) {
        m_appMenu = new HAppMenu(nullptr);
        m_appMenu->setAttribute(Qt::WA_DeleteOnClose, true);
        connect(m_appMenu, &QDialog::finished, this, [this]() {
            m_appMenu = nullptr;
        });
    }
    if (m_appMenu->isVisible()) {
        m_appMenu->close();
    } else {
        m_appMenu->show();
        m_appMenu->raise();
        m_appMenu->activateWindow();
        m_appMenu->focusSearch();
    }
}
#endif

void HWindow::keyReleaseEvent(QKeyEvent *e) {
    m_compositor->handleKeyRelease(e->nativeScanCode());
}

QWaylandView* HWindow::findViewAt(const QPointF &pos) {
    for (int i = m_windowList.size() - 1; i >= 0; --i) {
        if (m_windowList[i].geometry.contains(pos.toPoint())) {
            return m_windowList[i].view;
        }
    }
    return nullptr; // No window found under the mouse pointer
}

QPointF HWindow::getViewOffset(QWaylandView *view) {
    for (const auto &managedWin : m_windowList) {
        if (managedWin.view == view) {
            return managedWin.geometry.topLeft();
        }
    }
    return QPointF(0, 0);
}