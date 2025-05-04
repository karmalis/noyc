#include "wayland.h"
#include "xdg-shell-protocol.h"

#include <string.h>
#include <stdio.h>
#include <wayland-client-protocol.h>


const struct wl_registry_listener registry_listener = {
.global = registry_handle_global,
.global_remove = registry_handle_global_remove
};

const struct wl_buffer_listener buffer_listener = {
.release = wl_buffer_release
};

static void xdg_base_ping(void* data, struct xdg_wm_base *wm_base, uint32_t serial) {
    xdg_wm_base_pong(wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
.ping = xdg_base_ping
};

static void wl_seat_capabilities(void *data, struct wl_seat* seat, uint32_t capabilities) {
    struct app_state* state = (struct app_state*) data;

    int have_pointer = capabilities & WL_SEAT_CAPABILITY_POINTER;

    if (have_pointer && state->pointer == NULL) {
        state->pointer = wl_seat_get_pointer(state->seat);
        wl_pointer_add_listener(state->pointer,
                                &wl_pointer_listener,
                                state);
    } else if (!have_pointer && state->pointer != NULL) {
        wl_pointer_release(state->pointer);
        state->pointer = NULL;
    }

    int have_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;

    if (have_keyboard && state->keyboard == NULL) {
        state->keyboard = wl_seat_get_keyboard(state->seat);
        wl_keyboard_add_listener(state->keyboard,
                                 &wl_keyboard_listener, state);
    } else if (!have_keyboard && state->keyboard != NULL) {
        wl_keyboard_release(state->keyboard);
        state->keyboard = NULL;
    }
}



static void wl_seat_name(void* data, struct wl_seat* seat, const char* name) {
    fprintf(stderr, "seat name:%s\n", name);
}

static const struct wl_seat_listener wl_seat_listener = {
.capabilities = wl_seat_capabilities,
.name = wl_seat_name
};

static void registry_handle_global(void *data, struct wl_registry* registry,
                       uint32_t name, const char* interface, uint32_t version) {
    struct app_state* state = data;
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        state->compositor = wl_registry_bind(
            registry, name, &wl_compositor_interface, 6);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        state->shm = wl_registry_bind(
            registry, name, &wl_shm_interface, 2);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        state->xdg_wm_base = wl_registry_bind(
            registry, name, &xdg_wm_base_interface, 5);
        xdg_wm_base_add_listener(state->xdg_wm_base, &xdg_wm_base_listener, state);
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        state->seat = wl_registry_bind(
            registry, name, &wl_seat_interface, 9);
        wl_seat_add_listener(state->seat, &wl_seat_listener, state);
    }
}

static void registry_handle_global_remove(void *data, struct wl_registry *registry,
                              uint32_t name) {
    // temporary blank
}

static void wl_buffer_release(void* data, struct wl_buffer *buffer) {
    wl_buffer_destroy(buffer);
}
