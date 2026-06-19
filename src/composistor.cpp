/*
 * HawkWM Composistor Copyright (C) 2026  First Person
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
 */

#include <algorithm>
#include <iostream>
#include <string>
#include <variant>
#include <composistor.h>
#include <optional>
#include <thread>
#include <xdg-shell-client-protocol.h>
#include <wayland-client.h>
#include <wayland-server.h>
#include <X11/Xutil.h>
#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <webkit2/webkit2.h>

extern "C" {
    #include <atspi/atspi.h>
}

static std::string jsStr(const std::string& s) {
    std::string r = "'";
    for (unsigned char c : s) {
        switch (c) {
            case '\\': r += "\\\\"; break;
            case '\'': r += "\\'"; break;
            case '\n': r += "\\n"; break;
            case '\r': r += "\\r"; break;
            case '\t': r += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\x%02x", c);
                    r += buf;
                } else {
                    r += c;
                }
        }
    }
    r += '\'';
    return r;
}

static std::string getJSCodeFromRole(AtspiAccessible* accessible, const std::string& wrapperId) {
    gchar* raw_name = atspi_accessible_get_name(accessible, nullptr);
    std::string name_str = raw_name ? raw_name : "";
    if (raw_name) g_free(raw_name);

    AtspiRole role = atspi_accessible_get_role(accessible, nullptr);

    gint ax = 0, ay = 0, aw = 100, ah = 100;
    AtspiComponent* comp = atspi_accessible_get_component_iface(accessible);
    if (comp) {
        GError* err = nullptr;
        AtspiRect* rect = atspi_component_get_extents(comp, ATSPI_COORD_TYPE_SCREEN, &err);
        if (rect) {
            ax = rect->x; ay = rect->y; aw = rect->width; ah = rect->height;
            g_free(rect);
        }
        if (err) g_error_free(err);
        g_object_unref(comp);
    }

    std::string t = name_str.empty() ? jsStr("Window") : jsStr(name_str);

    auto sanitizeId = [](const std::string& s) -> std::string {
        std::string r;
        r.reserve(s.size());
        for (unsigned char c : s) {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
                r += c;
            else
                r += '_';
        }
        return r;
    };
    std::string base = name_str.empty() ? "window" : sanitizeId(name_str);

    std::string pos = "position:fixed;top:" + std::to_string(ay) + "px;left:" + std::to_string(ax) + "px;width:" + std::to_string(aw) + "px;height:" + std::to_string(ah) + "px;";

    auto js = [&](const std::string& body) -> std::string {
        return "var w=document.getElementById('" + wrapperId + "');"
               "if(!w)return;"
               "w.id='" + base + "';"
               "w.style.cssText='" + pos + "';"
               "w.innerHTML='';"
               "var drag=document.createElement('div');"
               "drag.style.cssText='position:absolute;top:0;left:0;width:100%;height:28px;cursor:grab;"
                   "z-index:10000;background:rgba(0,0,0,0.08);pointer-events:auto;';"
               "drag.onmousedown=function(e){"
                   "var startX=e.clientX, startY=e.clientY;"
                   "var origTop=parseInt(w.style.top,10)||0;"
                   "var origLeft=parseInt(w.style.left,10)||0;"
                   "document.body.style.userSelect='none';"
                   "function onMouseMove(m){w.style.top=(origTop+m.clientY-startY)+'px';w.style.left=(origLeft+m.clientX-startX)+'px';};"
                   "function onMouseUp(){document.removeEventListener('mousemove',onMouseMove);document.removeEventListener('mouseup',onMouseUp);document.body.style.userSelect='';};"
                   "document.addEventListener('mousemove',onMouseMove);"
                   "document.addEventListener('mouseup',onMouseUp);"
                   "e.preventDefault();"
               "};"
               "w.appendChild(drag);"
               + body;
    };

    switch (role) {
        case ATSPI_ROLE_PUSH_BUTTON:
        case ATSPI_ROLE_PUSH_BUTTON_MENU:
        case ATSPI_ROLE_TOGGLE_BUTTON:
        case ATSPI_ROLE_RADIO_BUTTON:
            return js(
                "var e=document.createElement('button');"
                "e.id='" + base + "_button';"
                "e.style.cssText='position:fixed;top:0;left:0;width:100%;height:100%;"
                    "font-size:2em;cursor:pointer;"
                    "background:linear-gradient(135deg,#667eea,#764ba2);"
                    "color:#fff;border:none;border-radius:8px;font-family:sans-serif;"
                    "box-shadow:0 4px 6px rgba(0,0,0,0.1);transition:transform 0.1s;';"
                "e.textContent=" + t + ";"
                "e.onclick=function(){alert('Button clicked');};"
                "e.onmousedown=function(){this.style.transform='scale(0.98)';};"
                "e.onmouseup=function(){this.style.transform='scale(1)';};"
                "w.appendChild(e);"
            );

        case ATSPI_ROLE_CHECK_BOX:
        case ATSPI_ROLE_CHECK_MENU_ITEM:
            return js(
                "var l=document.createElement('label');"
                "l.id='" + base + "_checkbox';"
                "l.style.cssText='position:fixed;top:0;left:0;width:100%;height:100%;"
                    "display:flex;align-items:center;justify-content:center;"
                    "font-size:2em;background:#f3e5f5;color:#4a148c;font-family:sans-serif;"
                    "border:2px solid #7b1fa2;box-sizing:border-box;';"
                "var c=document.createElement('input');c.type='checkbox';"
                "c.style.cssText='width:30px;height:30px;margin-right:12px;transform:scale(1.5);';"
                "l.appendChild(c);"
                "l.appendChild(document.createTextNode(" + t + "));"
                "w.appendChild(l);"
            );

        case ATSPI_ROLE_COMBO_BOX:
            return js(
                "var e=document.createElement('select');"
                "e.id='" + base + "_combobox';"
                "e.style.cssText='position:fixed;top:0;left:0;width:100%;height:100%;"
                    "font-size:1.5em;background:#fff3e0;color:#e65100;"
                    "border:2px solid #ef6c00;font-family:sans-serif;padding:10px;';"
                "var o=document.createElement('option');o.textContent=" + t + ";e.appendChild(o);"
                "var o2=document.createElement('option');o2.textContent='Option 1';e.appendChild(o2);"
                "var o3=document.createElement('option');o3.textContent='Option 2';e.appendChild(o3);"
                "w.appendChild(e);"
            );

        case ATSPI_ROLE_ENTRY:
        case ATSPI_ROLE_PASSWORD_TEXT:
        case ATSPI_ROLE_TEXT:
            return js(
                "var e=document.createElement('input');"
                "e.id='" + base + "_textbox';"
                "e.type='text';e.placeholder=" + t + ";"
                "e.style.cssText='position:fixed;top:0;left:0;width:100%;height:100%;"
                    "font-size:2em;padding:20px;box-sizing:border-box;"
                    "border:2px solid #1565c0;background:#e3f2fd;color:#0d47a1;font-family:sans-serif;';"
                "w.appendChild(e);e.focus();"
            );

        case ATSPI_ROLE_SLIDER:
            return js(
                "w.style.display='flex';"
                "w.style.alignItems='center';"
                "w.style.justifyContent='center';"
                "var e=document.createElement('input');e.type='range';e.min='0';e.max='100';e.value='50';"
                "e.id='" + base + "_slider';"
                "e.style.cssText='width:90%;height:40px;';"
                "var d=document.createElement('div');"
                "d.style.cssText='position:fixed;top:20px;left:0;width:100%;"
                    "text-align:center;font-size:1.5em;color:#333;font-family:sans-serif;';"
                "d.textContent=" + t + "+': 50';"
                "e.oninput=function(){d.textContent=" + t + "+': '+this.value;};"
                "w.appendChild(e);w.appendChild(d);"
            );

        case ATSPI_ROLE_MENU:
        case ATSPI_ROLE_MENU_BAR:
        case ATSPI_ROLE_MENU_ITEM:
            return js(
                "var e=document.createElement('div');"
                "e.id='" + base + "_menu';"
                "e.style.cssText='position:fixed;top:0;left:0;width:100%;"
                    "padding:15px 20px;font-size:1.5em;color:#fff;"
                    "background:#455a64;font-family:sans-serif;"
                    "box-shadow:0 2px 4px rgba(0,0,0,0.3);';"
                "e.textContent=" + t + ";"
                "var s=document.createElement('div');"
                "s.style.cssText='position:fixed;top:60px;left:0;width:100%;"
                    "background:#546e7a;font-family:sans-serif;';"
                "var i1=document.createElement('div');"
                "i1.style.cssText='padding:12px 20px;color:#fff;cursor:pointer;"
                    "border-bottom:1px solid #607d8b;';"
                "i1.textContent='Item 1';s.appendChild(i1);"
                "var i2=document.createElement('div');"
                "i2.style.cssText='padding:12px 20px;color:#fff;cursor:pointer;"
                    "border-bottom:1px solid #607d8b;';"
                "i2.textContent='Item 2';s.appendChild(i2);"
                "var i3=document.createElement('div');"
                "i3.style.cssText='padding:12px 20px;color:#fff;cursor:pointer;';"
                "i3.textContent='Item 3';s.appendChild(i3);"
                "w.appendChild(e);w.appendChild(s);"
            );

        case ATSPI_ROLE_PAGE_TAB:
        case ATSPI_ROLE_PAGE_TAB_LIST:
            return js(
                "var c=document.createElement('div');"
                "c.id='" + base + "_tab';"
                "c.style.cssText='position:fixed;top:0;left:0;width:100%;"
                    "display:flex;background:#f5f5f5;"
                    "border-bottom:2px solid #1976d2;font-family:sans-serif;';"
                "var t1=document.createElement('div');"
                "t1.style.cssText='padding:15px 25px;cursor:pointer;"
                    "background:#1976d2;color:#fff;font-weight:bold;';"
                "t1.textContent=" + t + ";c.appendChild(t1);"
                "var t2=document.createElement('div');"
                "t2.style.cssText='padding:15px 25px;cursor:pointer;color:#333;';"
                "t2.textContent='Tab 2';c.appendChild(t2);"
                "var p=document.createElement('div');"
                "p.style.cssText='position:fixed;top:52px;left:0;width:100%;"
                    "height:calc(100vh - 52px);padding:20px;box-sizing:border-box;"
                    "font-size:1.2em;color:#555;font-family:sans-serif;';"
                "p.textContent='Content for: '+(" + t + ");"
                "w.appendChild(c);w.appendChild(p);"
            );

        case ATSPI_ROLE_LIST:
        case ATSPI_ROLE_LIST_ITEM:
            return js(
                "var h=document.createElement('div');"
                "h.id='" + base + "_list';"
                "h.style.cssText='padding:15px 20px;font-size:1.5em;font-weight:bold;"
                    "background:#e8eaf6;color:#283593;font-family:sans-serif;"
                    "border-bottom:2px solid #3949ab;';"
                "h.textContent=" + t + ";w.appendChild(h);"
                "var l=document.createElement('ul');"
                "l.style.cssText='margin:0;padding:0;list-style:none;font-family:sans-serif;';"
                "(function(){var items=['Item 1','Item 2','Item 3'];"
                "for(var i=0;i<items.length;i++){"
                "var li=document.createElement('li');"
                "li.style.cssText='padding:15px 20px;border-bottom:1px solid #e0e0e0;"
                    "cursor:pointer;color:#333;font-size:1.1em;';"
                "li.textContent=items[i];"
                "li.onmouseover=function(){this.style.background='#e3f2fd';};"
                "li.onmouseout=function(){this.style.background='transparent';};"
                "l.appendChild(li);}})();"
                "w.appendChild(l);"
            );

        case ATSPI_ROLE_LABEL:
            return js(
                "w.style.display='flex';"
                "w.style.alignItems='center';"
                "w.style.justifyContent='center';"
                "var e=document.createElement('div');"
                "e.id='" + base + "_label';"
                "e.style.cssText='font-size:2.5em;color:#333;font-family:sans-serif;"
                    "text-align:center;padding:20px;';"
                "e.textContent=" + t + ";"
                "w.appendChild(e);"
            );

        default: {
            gchar* role_name = atspi_role_get_name(role);
            std::string label = role_name ? role_name : "unknown";
            if (role_name)
                g_free(role_name);
            return js(
                "w.style.display='flex';"
                "w.style.alignItems='center';"
                "w.style.justifyContent='center';"
                "var e=document.createElement('div');"
                "e.id='" + base + "_" + sanitizeId(label) + "';"
                "e.style.cssText='font-size:1.8em;color:#666;font-family:sans-serif;"
                    "text-align:center;padding:20px;';"
                "e.textContent=" + jsStr(label) + "+'('+" + t + "+')';"
                "w.appendChild(e);"
            );
        }
    }
}

static int onXSessionError(Display *display, XErrorEvent *error) {
    return 0;
}

GdkFilterReturn HComposistor::onX11EventFilter(GdkXEvent *xevent, GdkEvent *event, gpointer data) {
    auto *self = static_cast<HComposistor*>(data);
    auto *xev = static_cast<XEvent*>(xevent);

    if (xev->type == self->m_damage_event_base + XDamageNotify) {
        auto *de = reinterpret_cast<XDamageNotifyEvent*>(xev);
        if (de->damage == self->m_current_damage && self->currentWindow != 0
                && !self->m_current_wrapper_id.empty()) {
            XWindowAttributes attrs;
            if (XGetWindowAttributes(std::get<Display*>(self->display), self->currentWindow, &attrs)
                    && attrs.width > 0 && attrs.height > 0) {
                self->captureAndUpdateWebView(self->m_current_wrapper_id, self->currentWindow,
                                              attrs.width, attrs.height);
            }
        }
        return GDK_FILTER_CONTINUE;
    }

    return GDK_FILTER_CONTINUE;
}

HComposistor::HComposistor(DisplayProtocol protocol) : m_surface(nullptr), m_frame_callback(nullptr), gwnd(nullptr), webView(nullptr), contentManager(nullptr), currentWindow(0), currentPid(0), gscreen(nullptr) {
    switch (protocol) {
        case DisplayProtocol::Wayland:
            display = wl_display_connect(nullptr);
            if (std::get<wl_display*>(display) == nullptr) {
                if (wl_display_get_error(std::get<wl_display*>(display)) == EPROTO)
                    std::exit(interpretProtocolError());
                else
                    std::cout << "Unable To Connect With Display." << std::endl;
                std::exit(WL_DISPLAY_ERROR);
            }
            break;

        case DisplayProtocol::Xorg:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            g_type_init();
#pragma GCC diagnostic pop

            gdk_set_allowed_backends("x11");

            if (!gtk_init_check(nullptr, nullptr)) {
                std::cout << "Unable To Connect With Display." << std::endl;
                std::exit(WL_DISPLAY_ERROR);
            }
            display = gdk_x11_get_default_xdisplay();

            if (atspi_init()) {
                std::cerr << "Failed To Initialize AT-SPI." << std::endl;
                std::exit(1474);
            }

            m_current_damage = None;
            {
                int comp_event_base, comp_error_base;
                m_composite_available = XCompositeQueryExtension(std::get<Display*>(display),
                    &comp_event_base, &comp_error_base);
                XDamageQueryExtension(std::get<Display*>(display),
                    &m_damage_event_base, &m_damage_error_base);
            }

            gwnd = gtk_window_new(GTK_WINDOW_TOPLEVEL);
            gtk_window_set_default_size(GTK_WINDOW(gwnd), 1280, 720);
            g_signal_connect(gwnd, "destroy", G_CALLBACK(gtk_main_quit), NULL);
            g_signal_connect(gwnd, "key-press-event", G_CALLBACK(HComposistor::onKeyPress), this);

            contentManager = webkit_user_content_manager_new();
            webkit_user_content_manager_register_script_message_handler(contentManager, "hawk");
            g_signal_connect(contentManager,
                             "script-message-received::hawk",
                             G_CALLBACK(HComposistor::onScriptMessageReceived),
                             this);
            webView = GTK_WIDGET(webkit_web_view_new_with_user_content_manager(contentManager));
            gtk_container_add(GTK_CONTAINER(gwnd), webView);

            gtk_widget_show_all(gwnd);
            gtk_widget_realize(gwnd);

            GdkWindow *gdk_window = gtk_widget_get_window(gwnd);
            if (gdk_window == nullptr) {
                std::cerr << "Failed to initialize X11 webview window." << std::endl;
                std::exit(1);
            }

            root = GDK_WINDOW_XID(gdk_window);
            gscreen = gdk_display_get_default_screen(gdk_display_get_default());

            XSelectInput(std::get<Display*>(display), root, SubstructureNotifyMask | StructureNotifyMask | ExposureMask);
            updateFrame();
            XSetErrorHandler(onXSessionError);
            XSetIOErrorHandler(onXSessionExit);
            gtk_widget_show_all(gwnd);
            gdk_window_add_filter(NULL, onX11EventFilter, this);
            //std::thread worker(&HComposistor::loop, this);
            //worker.detach();
            break;

        /* default:
            std::cout << "No Supported Display Protocol Found.";
            std::exit(750); */
    }
}

const struct xdg_surface_listener HComposistor::xdg_surface_listener = {
    .configure = onWlWindowCreated,
};

const struct wl_callback_listener HComposistor::frame_listener = {
    .done = onFrameDone,
};

void HComposistor::onWlWindowCreated(void *data, struct xdg_surface *xdg_surface, uint32_t serial) {
}

void HComposistor::onFrameDone(void *data, struct wl_callback *callback, uint32_t time) {
    HComposistor *self = static_cast<HComposistor*>(data);
    if (!self)
        return;

    if (self->m_frame_callback == callback)
        self->m_frame_callback = nullptr;

    self->updateFrame();
}

gboolean HComposistor::onKeyPress(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    switch (event->keyval) {
        case GDK_KEY_Escape:
            std::exit(0);
        default:
            break;
    }
    return FALSE;
}

void HComposistor::onScriptMessageReceived(WebKitUserContentManager *manager, WebKitJavascriptResult *result, gpointer user_data) {
    HComposistor *self = static_cast<HComposistor*>(user_data);
    if (!self || !result)
        return;

    JSCValue *value = webkit_javascript_result_get_js_value(result);
    if (!value)
        return;

    gchar *text = jsc_value_to_string(value);
    if (!text)
        return;

    std::string message(text);
    g_free(text);
    self->handleScriptMessage(message);
}

void HComposistor::onJavaScriptEvaluated(GObject *source_object, GAsyncResult *res, gpointer user_data) {
    WebKitWebView *web_view = WEBKIT_WEB_VIEW(source_object);
    GError *error = nullptr;
    JSCValue *result = webkit_web_view_evaluate_javascript_finish(web_view, res, &error);

    if (error) {
        std::cout << "[JavaScript Error] " << error->message << std::endl;
        g_error_free(error);
    }

    if (result)
        g_object_unref(result);
}

void HComposistor::handleScriptMessage(const std::string &message) {
    const std::string prefix = "window-update:";
    if (message.rfind(prefix, 0) != 0)
        return;

    int x = 0, y = 0;
    unsigned int width = 0, height = 0;
    unsigned long pid = 0;
    std::string payload = message.substr(prefix.size());
    if (sscanf(payload.c_str(), "%d,%d,%u,%u,%lu", &x, &y, &width, &height, &pid) != 5)
        return;

    if (currentPid != static_cast<gint>(pid) || currentWindow == 0)
        return;

    XMoveResizeWindow(std::get<Display*>(display), currentWindow, x, y, width, height);
    XFlush(std::get<Display*>(display));
}

/*
 * captureAndUpdateWebView not getting callback
 */

void HComposistor::captureAndUpdateWebView(const std::string& wid, Window window,
                                           unsigned int width, unsigned int height) {
    if (!m_composite_available || width == 0 || height == 0)
        return;

    Display *disp = std::get<Display*>(display);

    Pixmap pixmap = XCompositeNameWindowPixmap(disp, window);
    if (!pixmap)
        return;

    XImage *image = XGetImage(disp, pixmap, 0, 0, width, height, AllPlanes, ZPixmap);
    XFreePixmap(disp, pixmap);
    if (!image)
        return;

    GdkPixbuf *pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, width, height);
    if (!pixbuf) {
        XDestroyImage(image);
        return;
    }

    auto trailingZeros = [](unsigned long mask) -> int {
        if (!mask) return 0;
        int n = 0;
        while (!(mask & 1)) { mask >>= 1; n++; }
        return n;
    };

    int rshift = trailingZeros(image->red_mask);
    int gshift = trailingZeros(image->green_mask);
    int bshift = trailingZeros(image->blue_mask);

    guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);
    int dest_stride = gdk_pixbuf_get_rowstride(pixbuf);

    for (unsigned int y = 0; y < height; y++) {
        for (unsigned int x = 0; x < width; x++) {
            unsigned long pixel = XGetPixel(image, x, y);
            pixels[y * dest_stride + x * 3 + 0] = (pixel >> rshift) & 0xFF;
            pixels[y * dest_stride + x * 3 + 1] = (pixel >> gshift) & 0xFF;
            pixels[y * dest_stride + x * 3 + 2] = (pixel >> bshift) & 0xFF;
        }
    }

    XDestroyImage(image);

    gchar *png_buffer = nullptr;
    gsize png_size = 0;
    gdk_pixbuf_save_to_buffer(pixbuf, &png_buffer, &png_size, "png", nullptr, nullptr);
    g_object_unref(pixbuf);

    if (!png_buffer || png_size == 0)
        return;

    gchar *b64 = g_base64_encode(reinterpret_cast<const guchar*>(png_buffer), png_size);
    g_free(png_buffer);

    if (!b64)
        return;

    std::string js = "var w=document.getElementById('" + wid + "');"
                     "if(!w)return;"
                     "var c=document.getElementById('" + wid + "_content');"
                     "if(!c){"
                     "  c=document.createElement('img');"
                     "  c.id='" + wid + "_content';"
                     "  c.style.cssText='position:absolute;top:0;left:0;width:100%;height:100%;"
                     "    object-fit:fill;z-index:0;pointer-events:none;';"
                     "  w.appendChild(c);"
                     "}"
                     "c.src='data:image/png;base64," + std::string(b64) + "';";

    webkit_web_view_evaluate_javascript(
        WEBKIT_WEB_VIEW(webView), js.c_str(),
        -1, nullptr, nullptr, nullptr,
        (GAsyncReadyCallback)onJavaScriptEvaluated, nullptr
    );

    g_free(b64);
}

uint32_t HComposistor::interpretProtocolError() {
    const wl_interface* interface;
    uint32_t id;

    uint32_t err = wl_display_get_protocol_error(std::get<wl_display*>(display), &interface, &id);
    if (err == WL_DISPLAY_ERROR_NO_MEMORY)
        std::cout << "The Compositor Ran Out Memory." << std::endl;
    else if (err == WL_DISPLAY_ERROR_INVALID_OBJECT)
        std::cout << "The Requested Object Is Invalid." << std::endl;
    else if (err == WL_DISPLAY_ERROR_INVALID_METHOD)
        std::cout << "The Requested Method Is Invalid." << std::endl;
#ifdef DEBUG_MODE
    else if (err == WL_DISPLAY_ERROR_IMPLEMENTATION)
        throw std::runtime_error("Something Went Wrong In Composistor");
#endif
    return err;
}

int HComposistor::onXSessionExit(Display *display) {
    std::cout << "Session Exitting." << std::endl;
    std::exit(0);
    return 0;
}

void HComposistor::loop() {
    if (std::holds_alternative<Display*>(display)) {
        XEvent event;
        while (1) {
            bool UIDone = false;
            if (g_main_context_iteration(g_main_context_default(), false))
                UIDone = true;

            XNextEvent(std::get<Display*>(display), &event);
            switch (event.type) {
                case CreateNotify:
                    onXorgWindowCreated(&event.xcreatewindow.window);
                    break;

                case DestroyNotify:
                    onXorgWindowDestoried(&event.xdestroywindow.window);
                    break;

                case ResizeRequest: {
                    XResizeRequestEvent *wev = &event.xresizerequest;
                    onXorgWindowResized(wev->window, wev->height, wev->width);
                    break;
                }

                case Expose:
                    if (event.xexpose.window == root)
                        updateFrame();
                    break;

                case ConfigureNotify:
                    onXorgWindowMoved(event.xconfigure.window, event.xconfigure.x, event.xconfigure.y);
                    break;

                /* default:
                    break; */
            }

            if (UIDone)
                g_usleep(2000); // Prevent From Spin Lock Stuck
        }
    } else if (std::holds_alternative<wl_display*>(display)) {
        while (1) {
            wl_display_dispatch(std::get<wl_display*>(display));
        }
    }
}

void HComposistor::onXorgWindowCreated(Window* window) {
    if (*window != root);
        reloadWindow(window);
}

void HComposistor::onXorgWindowDestoried(Window* window) {
    destory(window);
}

void HComposistor::onXorgWindowResized(Window window, int height, int width) {
    XResizeWindow(std::get<Display*>(display), window, width, height);
}

void HComposistor::onXorgWindowMoved(Window window, int x, int y) {
    if (window == root) return;
    XMoveWindow(std::get<Display*>(display), window, x, y);
}

void HComposistor::setSurface(struct wl_surface *surface) {
    m_surface = surface;
}

void HComposistor::updateFrame() {
    if (std::holds_alternative<Display*>(display)) {
        if (!webView)
            return;

        const std::string js =
            "if (!document.getElementById('hawk-root')) {"
            "  var root = document.createElement('div');"
            "  root.id='hawk-root';"
            "  root.style.cssText='position:fixed;top:0;left:0;right:0;bottom:0;"
            "background:radial-gradient(circle at top left, rgba(63,81,181,0.35), transparent 30%),"
            "linear-gradient(180deg, #10101a 0%, #18182e 50%, #0a0a14 100%);"
            "overflow:hidden;';"
            "  document.body.appendChild(root);"
            "} else {"
            "  document.getElementById('hawk-root').style.background = 'radial-gradient(circle at top left, rgba(63,81,181,0.35), transparent 30%), linear-gradient(180deg, #10101a 0%, #18182e 50%, #0a0a14 100%)';"
            "}"
            "document.body.style.margin='0';"
            "document.body.style.overflow='hidden';";

        webkit_web_view_evaluate_javascript(
            WEBKIT_WEB_VIEW(webView),
            js.c_str(),
            -1,
            NULL,
            NULL,
            NULL,
            (GAsyncReadyCallback)onJavaScriptEvaluated,
            NULL
        );
        return;
    }
    else if (std::holds_alternative<wl_display*>(display) && m_surface) {
        if (m_frame_callback) {
            wl_callback_destroy(m_frame_callback);
            m_frame_callback = nullptr;
        }

        m_frame_callback = wl_surface_frame(m_surface);
        wl_callback_add_listener(m_frame_callback, &frame_listener, this);
        wl_surface_commit(m_surface);
    }
}

void HComposistor::reloadWindow(WindowVariant window) {
    if (std::holds_alternative<Window*>(window) && std::holds_alternative<Display*>(display)) {
        Window *xwindow = std::get<Window*>(window);
        if (xwindow && *xwindow) {
            XEvent xev;
            xev.type = ClientMessage;
            xev.xclient.window = *xwindow;
            xev.xclient.message_type = XInternAtom(std::get<Display*>(display), "WM_CHANGE_STATE", False);
            xev.xclient.format = 32;
            xev.xclient.data.l[0] = IconicState; // Force it to iconic (minimized) state

            // Send the message to the root window
            XSendEvent(
                std::get<Display*>(display),
                root,
                False,
                SubstructureRedirectMask | SubstructureNotifyMask,
                &xev
            );

            // If Hawk WM Ignores To Unmap The Window
            XUnmapWindow(std::get<Display*>(display), *xwindow);

            XMapWindow(std::get<Display*>(display), *xwindow);

            Atom actual_type;
            int aformat;
            unsigned long nitems, bytes_after;
            unsigned char* prop = nullptr;
            unsigned long pid = 0;

            Atom patom = XInternAtom(std::get<Display*>(display), "_NET_WM_PID", True);
            if (patom != None && XGetWindowProperty(
                    std::get<Display*>(display), *xwindow, patom,
                    0, 1, False, XA_CARDINAL,
                    &actual_type, &aformat,
                    &nitems, &bytes_after, &prop) == Success && prop) {
                pid = *((unsigned long*)prop);
                XFree(prop);
            }

            if (pid == 0)
                std::cerr << "Unable To Get PID Of Window" << std::endl;

            gint target_pid = static_cast<gint>(pid);
            std::string wid = "wnd_" + std::to_string(pid);

            currentWindow = *xwindow;
            currentPid = target_pid;

            int win_x = 0, win_y = 0;
            unsigned int win_width = 0, win_height = 0;
            bool isResizable = true;
            XWindowAttributes attrs;
            if (XGetWindowAttributes(std::get<Display*>(display), *xwindow, &attrs)) {
                win_width = attrs.width;
                win_height = attrs.height;
                Window child;
                XTranslateCoordinates(
                    std::get<Display*>(display), *xwindow,
                    DefaultRootWindow(std::get<Display*>(display)),
                    0, 0, &win_x, &win_y, &child
                );

                XSizeHints sizeHints;
                long supplied_return = 0;
                if (XGetWMNormalHints(std::get<Display*>(display), *xwindow, &sizeHints, &supplied_return)) {
                    if ((sizeHints.flags & (PMinSize | PMaxSize)) == (PMinSize | PMaxSize)
                            && sizeHints.min_width == sizeHints.max_width
                            && sizeHints.min_height == sizeHints.max_height) {
                        isResizable = false;
                    }
                }
            }

            if (m_composite_available && *xwindow) {
                if (m_current_damage != None) {
                    XDamageDestroy(std::get<Display*>(display), m_current_damage);
                    m_current_damage = None;
                }
                XCompositeRedirectWindow(std::get<Display*>(display), *xwindow,
                                          CompositeRedirectAutomatic);
                m_current_damage = XDamageCreate(std::get<Display*>(display), *xwindow,
                                                  XDamageReportRawRectangles);
                m_current_wrapper_id = wid;
            }

            std::string wrapperStyle = std::string("position:fixed;")
                                       + "top:" + std::to_string(win_y) + "px;"
                                       + "left:" + std::to_string(win_x) + "px;"
                                       + "width:" + std::to_string(win_width) + "px;"
                                       + "height:" + std::to_string(win_height) + "px;";
            if (isResizable)
                wrapperStyle += "resize:both;overflow:scroll;";
            else
                wrapperStyle += "resize:none;overflow:scroll;";

            webkit_web_view_evaluate_javascript(
                WEBKIT_WEB_VIEW(webView),
                ("document.body.style.cssText='margin:0;background:#fff;';"
                 "var e=document.getElementById('" + wid + "');if(e)e.remove();"
                 "var w=document.createElement('div');"
                 "w.id='" + wid + "';"
                 "w.style.cssText='" + wrapperStyle + "';"
                 "var c=document.createElement('img');"
                 "c.id='" + wid + "_content';"
                 "c.style.cssText='position:absolute;top:0;left:0;width:100%;height:100%;"
                     "object-fit:fill;z-index:0;pointer-events:none;';"
                 "w.appendChild(c);"
                 "var reportWindow=function(){"
                     "var left=parseInt(w.style.left,10)||0;"
                     "var top=parseInt(w.style.top,10)||0;"
                     "var width=parseInt(w.style.width,10)||0;"
                     "var height=parseInt(w.style.height,10)||0;"
                     "if(window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.hawk)"
                         "window.webkit.messageHandlers.hawk.postMessage('window-update:' + left + ',' + top + ',' + width + ',' + height + '," + std::to_string(pid) + "');"
                 "};"
                 "w.addEventListener('mouseup',reportWindow);"
                 "document.body.appendChild(w);").c_str(),
                -1, NULL, NULL, NULL, (GAsyncReadyCallback)onJavaScriptEvaluated, NULL
            );
            AtspiAccessible* desktop = atspi_get_desktop(0);
            if (desktop) {
                gint app_count = atspi_accessible_get_child_count(desktop, nullptr);
                for (gint i = 0; i < app_count; ++i) {
                    AtspiAccessible* app = atspi_accessible_get_child_at_index(desktop, i, nullptr);
                    if (app) {
                        if (atspi_accessible_get_process_id(app, nullptr) == target_pid) {
                            webkit_web_view_evaluate_javascript(
                                WEBKIT_WEB_VIEW(webView),
                                getJSCodeFromRole(app, wid).c_str(),
                                -1,
                                NULL,
                                NULL,
                                NULL,
                                (GAsyncReadyCallback)onJavaScriptEvaluated,
                                NULL
                            );
                            g_object_unref(app);
                            break;
                        }
                        g_object_unref(app);
                    }
                }
                g_object_unref(desktop);
            }

            // The Bug Around Here
            if (m_composite_available && *xwindow && win_width > 0 && win_height > 0)
                captureAndUpdateWebView(wid, *xwindow, win_width, win_height);

            XFlush(std::get<Display*>(display));
        }
    }
}

void HComposistor::destory(WindowVariant window) {
    std::string title;
    if (std::holds_alternative<Window*>(window)) {
        Window *xwindow = std::get<Window*>(window);
        if (xwindow && *xwindow) {
            char* wm_name = nullptr;
            if (XFetchName(std::get<Display*>(display), *xwindow, &wm_name) && wm_name) {
                for (char* p = wm_name; *p; ++p) {
                    unsigned char c = *p;
                    title += ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) ? c : '_';
                }
                XFree(wm_name);
            }
            if (m_composite_available && *xwindow == currentWindow) {
                if (m_current_damage != None) {
                    XDamageDestroy(std::get<Display*>(display), m_current_damage);
                    m_current_damage = None;
                }
                XCompositeUnredirectWindow(std::get<Display*>(display), *xwindow,
                                            CompositeRedirectAutomatic);
                currentWindow = 0;
                currentPid = 0;
                if (!m_current_wrapper_id.empty()) {
                    m_current_wrapper_id.clear();
                }
            }
            XDestroyWindow(std::get<Display*>(display), *xwindow);
        }
    } else {
        struct xdg_surface *xdg_surface = std::get<struct xdg_surface*>(window);
        if (xdg_surface) {
            xdg_surface_destroy(xdg_surface);
        }
    }

    if (!title.empty()) {
        webkit_web_view_evaluate_javascript(
            WEBKIT_WEB_VIEW(webView),
            ("var e=document.getElementById('" + title + "');if(e)e.remove();").c_str(),
            -1, NULL, NULL, NULL, (GAsyncReadyCallback)onJavaScriptEvaluated, NULL
        );
    }
}

HComposistor::~HComposistor() {
    if (std::holds_alternative<wl_display*>(display)) {
        wl_display *disp = std::get<wl_display*>(display);
        if (disp) {
            wl_display_disconnect(disp);
        }
    }
}
