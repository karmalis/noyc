#ifndef BUFFER_H_
#define BUFFER_H_

#include <stdint.h>
#include <wayland-client.h>

#include "input.h"
#include "xdg-shell-protocol.h"

struct app_state {
    struct wl_compositor* compositor;
    struct wl_shm* shm;
    struct wl_display* display;
    struct wl_registry *registry;
    struct wl_seat* seat;

    struct wl_keyboard* keyboard;
    struct wl_pointer* pointer;
    struct wl_surface* surface;

    // XDG stuff
    struct xdg_wm_base* xdg_wm_base;
     struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
    // Pixels
    uint32_t* pixels;

    int regenerate;

    struct pointer_event pointer_event;
    struct xkb_state* xkb_state;
    struct xkb_context* xkb_context;
    struct xkb_keymap* xkb_keymap;
};

static void registry_handle_global(void *data, struct wl_registry* registry,
                                   uint32_t name, const char* interface, uint32_t version);

static void registry_handle_global_remove(void *data, struct wl_registry *registry,
                                          uint32_t name);

static void wl_buffer_release(void* data, struct wl_buffer *buffer);

extern const struct wl_registry_listener registry_listener;
extern const struct wl_buffer_listener buffer_listener;
#endif // BUFFER_H_
