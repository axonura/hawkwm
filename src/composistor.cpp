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
#include <xdg-shell-client-protocol.h>
#include <wayland-client.h>
#include <wayland-server.h>
#include <X11/Xutil.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
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
            display = XOpenDisplay(std::getenv("DISPLAY") ? std::getenv("DISPLAY") : nullptr);
            if (std::get<Display*>(display) == nullptr) {
                std::cout << "Unable To Connect With Display." << std::endl;
                std::exit(WL_DISPLAY_ERROR);
            }

            gdk_set_allowed_backends("x11");
            gtk_init(nullptr, nullptr);

            gwnd = gtk_window_new(GTK_WINDOW_TOPLEVEL);
            gtk_window_set_default_size(GTK_WINDOW(gwnd), 1280, 720);
            g_signal_connect(gwnd, "destroy", G_CALLBACK(gtk_main_quit), NULL);

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

            XSelectInput(std::get<Display*>(display), root, SubstructureNotifyMask | StructureNotifyMask | ExposureMask | KeyPressMask);
            updateFrame();
            XSetIOErrorHandler(onXSessionExit);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            g_type_init(); // Required for older GLib structures used by AT-SPI
#pragma GCC diagnostic pop

            if (atspi_init()) {
                std::cerr << "Failed To Initialize AT-SPI." << std::endl;
                std::exit(1474);
            }
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
    if (display)
        XCloseDisplay(display);
    std::cout << "Session Exitting." << std::endl;
    std::exit(0);
    return 0;
}

void HComposistor::loop() {
    if (std::holds_alternative<Display*>(display)) {
        XEvent event;
        while (1) {
            XNextEvent(std::get<Display*>(display), &event);
            switch (event.type) {
                case CreateNotify:
                    onXorgWindowCreated(&event.xcreatewindow.window);
                    break;

                case DestroyNotify:
                    onXorgWindowDestoried(&event.xdestroywindow.window);
                    break;

                case ResizeRequest: {
                    XCreateWindowEvent *wev = &event.xcreatewindow;
                    onXorgWindowResized(wev->window, wev->height, wev->width, wev->x, wev->y);
                    break;
                }

                case Expose:
                    if (event.xexpose.window == root)
                        updateFrame();
                    break;
            }
        }
    }
}

void HComposistor::onXorgWindowCreated(Window* window) {
    reloadWindow(window);
}

void HComposistor::onXorgWindowDestoried(Window* window) {
    destory(window);
}

void HComposistor::onXorgWindowResized(Window window, int height, int width, int x, int y) {
    XResizeWindow(std::get<Display*>(display), window, height, width);
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
            NULL,
            NULL
        );
        return;
    }

    if (std::holds_alternative<wl_display*>(display) && m_surface) {
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
                -1, NULL, NULL, NULL, NULL, NULL
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
                                -1,               // Length of the string (-1 if null-terminated)
                                NULL,             // World name (NULL for the main isolated world)
                                NULL,             // Source URI
                                NULL,             // GCancellable object
                                NULL,             // Callback function
                                NULL              // User data passed to callback
                            );
                            g_object_unref(app);
                            break;
                        }
                        g_object_unref(app);
                    }
                }
                g_object_unref(desktop);
            }

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
            XDestroyWindow(std::get<Display*>(display), *xwindow);
        }
    } else {
        struct xdg_surface *xdg_surface = std::get<struct xdg_surface*>(window);
        if (xdg_surface) {
            xdg_surface_destroy(xdg_surface);
        }
    }

    // Clean-up Elements Of Destroyed Window
    if (!title.empty()) {
        webkit_web_view_evaluate_javascript(
            WEBKIT_WEB_VIEW(webView),
            ("var e=document.getElementById('" + title + "');if(e)e.remove();").c_str(),
            -1, NULL, NULL, NULL, NULL, NULL
        );
    }
}

HComposistor::~HComposistor() {
    if (std::holds_alternative<Display*>(display)) {
        Display *disp = std::get<Display*>(display);
        if (disp)
            XCloseDisplay(disp);
    } else if (std::holds_alternative<wl_display*>(display)) {
        wl_display *disp = std::get<wl_display*>(display);
        if (disp) {
            wl_display_disconnect(disp);
        }
    }
}
