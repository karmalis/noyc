#include "buffer.h"

#include <string.h>
#include <wayland-client-protocol.h>

const struct wl_registry_listener registry_listener = {
.global = registry_handle_global,
.global_remove = registry_handle_global_remove
};

const struct wl_buffer_listener wl_buffer_listener = {
.release = wl_buffer_release
};

static void
registry_handle_global(void *data, struct wl_registry* registry,
                       uint32_t name, const char* interface, uint32_t version) {
    struct app_state* state = data;
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        state->compositor = wl_registry_bind(
            registry, name, &wl_compositor_interface, 6);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        state->shm = wl_registry_bind(
            registry, name, &wl_shm_interface, 6);
    } else if (strcmp(interface, wl_shell_interface.name) == 0) {
        state->shell = wl_registry_bind(
            registry, name, &wl_shell_interface, 6);
    }
}

static void
registry_handle_global_remove(void *data, struct wl_registry *registry,
                              uint32_t name) {
    // temporary blank
}

static void wl_buffer_release(void* data, struct wl_buffer *buffer) {
    wl_buffer_destroy(buffer);
}
