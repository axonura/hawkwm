/*
 * HawkWM Composistor Copyright (C) 2026  First Person
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
 */

#ifndef COMPOSISTOR_H
#define COMPOSISTOR_H

#include <variant>
#include <vector>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <wayland-client.h>
#include <wayland-server.h>
#include <xdg-shell-client-protocol.h>
#include <optional>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <webkit2/webkit2.h>

enum class DisplayProtocol {
    Wayland,
    Xorg
};

using DisplayVariant = std::variant<wl_display*, Display*>;
using WindowVariant = std::variant<struct xdg_surface*, Window*>;
class HComposistor {
private:
    DisplayVariant display;
    Window root;
    GtkWidget *gwnd, *webView;
    WebKitUserContentManager *contentManager;
    Window currentWindow;
    gint currentPid;
    struct wl_surface *m_surface;
    struct wl_callback *m_frame_callback;
    GdkScreen *gscreen;

    static const struct xdg_surface_listener xdg_surface_listener;
    static const struct wl_callback_listener frame_listener;

    uint32_t interpretProtocolError();
    static void onWlWindowCreated(void *data, struct xdg_surface *xdg_surface, uint32_t serial);
    void onXorgWindowCreated(Window* window);
    void onXorgWindowDestoried(Window* window);
    void onXorgWindowResized(Window window, int height, int width, int x, int y);
    static void onFrameDone(void *data, struct wl_callback *callback, uint32_t time);
    void reloadWindow(WindowVariant window);
    void destory(WindowVariant window);
    void setSurface(struct wl_surface *surface);
    void updateFrame();
    static int onXSessionExit(Display *display);
    static void onScriptMessageReceived(WebKitUserContentManager *manager, WebKitJavascriptResult *result, gpointer user_data);
    void handleScriptMessage(const std::string &message);
public:
    HComposistor(DisplayProtocol protocol);
    void loop();
    ~HComposistor();
};


#endif // COMPOSISTOR_H
