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
#include <QKeyEvent>
#include <compositor.h>
#include <window.h>

HWindow::HWindow() {
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

void HWindow::mousePressEvent(QMouseEvent *event) {
    m_compositor->handleMousePress(event->position().toPoint(), event->button());
}

void HWindow::setCompositor(HCompositor *hcmp) {
    m_compositor = hcmp;
}

void HWindow::mouseMoveEvent(QMouseEvent *event) {
    m_compositor->handleMouseMove(event->position().toPoint());
}

void HWindow::mouseReleaseEvent(QMouseEvent *event) {
    m_compositor->handleMouseRelease(event->position().toPoint(), event->button(), event->buttons());
}

void HWindow::wheelEvent(QWheelEvent *e) {
    m_compositor->handleMouseWheel(e->angleDelta());
}

void HWindow::keyPressEvent(QKeyEvent *e) {
    m_compositor->handleKeyPress(e->nativeScanCode());
}

void HWindow::keyReleaseEvent(QKeyEvent *e) {
    m_compositor->handleKeyRelease(e->nativeScanCode());
}