#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon.h>

#include "iperlin.h"
#include "wayland/wayland.h"
#include "wayland/sharedmem.h"
#include "wayland/xdg-shell-protocol.h"

static const int WIDTH = 1024;
static const int HEIGHT = 1024;

// Overwrites
void generate_noise(int width, int height, int octaves, double per, double bfreq, double bamp, uint32_t* pixels) {
 for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t index = (size_t)(y*width + x);
            double n_v = octave_iperlin_at((double) x, (double) y, 0.0, octaves, per, bfreq, bamp);
            uint8_t val = (uint8_t)((n_v * 0.5 + 0.5) * 255.0);
            uint8_t construct[] = {val, val, val, 255};
            uint32_t pixel = *((uint32_t*)&construct);
            pixels[index] = pixel;
        }
    }

}

static struct wl_buffer* draw_frame(struct app_state* state) {
     int stride = WIDTH * 4;
     int size = HEIGHT * stride;

    int fd = allocate_shm_file(size);
    uint32_t *pool_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (pool_data == MAP_FAILED) {
        close(fd);
        return NULL;
    }

    struct wl_shm_pool* pool = wl_shm_create_pool(state->shm, fd, size);
    int index = 0; // Index could be used for multi data blob storage, for now it's just for show
    int offset = HEIGHT * stride * index;
    struct wl_buffer* buffer = wl_shm_pool_create_buffer(pool, offset, WIDTH, HEIGHT,
                                                         stride, WL_SHM_FORMAT_XRGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);

    uint32_t* pixels = (uint32_t*) &pool_data[offset];
    memcpy(pixels, state->pixels, size);


    munmap(pool_data, size);
    wl_buffer_add_listener(buffer, &buffer_listener, NULL);
    return buffer;

}

static void xdg_surface_configure(void* data, struct xdg_surface* surface, uint32_t serial) {
    struct app_state* state = data;
    xdg_surface_ack_configure(surface, serial);

    struct wl_buffer *buffer = draw_frame(state);
    wl_surface_attach(state->surface, buffer, 0, 0);
    wl_surface_commit(state->surface);
}

static const struct xdg_surface_listener xdg_surface_listener = {
.configure = xdg_surface_configure
};

static const struct wl_callback_listener wl_surface_frame_listener;

static void wl_surface_frame_done(void* data, struct wl_callback* cb, uint32_t time) {
    wl_callback_destroy(cb);
    struct app_state* state = (struct app_state*) data;

    cb = wl_surface_frame(state->surface);
    wl_callback_add_listener(cb, &wl_surface_frame_listener, state);

    if (state->regenerate) {
        fprintf(stderr, "Regenerating\n");

          // 8 0.75 0.00095 0.5 initial values
        int octaves = 2;
        double per = 0.25;
        double bfreq = 0.00095;
        double bamp = 0.5;

        int width = 1024;
        int height = 1024;

        generate_noise(width, height, octaves, per, bfreq, bamp, state->pixels);

        struct wl_buffer* buffer = draw_frame(state);
        wl_surface_attach(state->surface, buffer, 0, 0);
        wl_surface_damage_buffer(state->surface, 0, 0, INT32_MAX, INT32_MAX);
        wl_surface_commit(state->surface);

        state->regenerate = 0;
    }
}

static const struct wl_callback_listener wl_surface_frame_listener = {
.done = wl_surface_frame_done,
};

int main(int argc, char** argv) {

    struct wl_display* display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "Failed to connect to wayland display.\n");
        return EXIT_FAILURE;
    }

    fprintf(stderr, "Connection established.\n");

    struct wl_registry* registry = wl_display_get_registry(display);
    if (!registry) {
        fprintf(stderr, "Failed to get wayland registry.\n");
        return EXIT_FAILURE;
    }

    struct app_state state = {0};

    state.display = display;
    state.registry = registry;
    state.xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    state.regenerate = 0;

     // 8 0.75 0.00095 0.5 initial values
    int octaves = 8;
    double per = 0.75;
    double bfreq = 0.00095;
    double bamp = 0.5;

    int width = 1024;
    int height = 1024;

    state.pixels = malloc(sizeof(uint32_t) * width * height);

    generate_noise(width, height, octaves, per, bfreq, bamp, state.pixels);

    wl_registry_add_listener(registry, &registry_listener, &state);
    wl_display_roundtrip(display);

    state.surface = wl_compositor_create_surface(state.compositor);

    state.xdg_surface = xdg_wm_base_get_xdg_surface(state.xdg_wm_base, state.surface);
    xdg_surface_add_listener(state.xdg_surface, &xdg_surface_listener, &state);
    state.xdg_toplevel = xdg_surface_get_toplevel(state.xdg_surface);
    xdg_toplevel_set_title(state.xdg_toplevel, "Noysway noise generator");

    wl_surface_commit(state.surface);

    struct wl_callback* cb = wl_surface_frame(state.surface);
    wl_callback_add_listener(cb, &wl_surface_frame_listener, &state);

    while (wl_display_dispatch(state.display)) {
        struct pointer_event* event = &state.pointer_event;
    }

    wl_display_disconnect(display);

    free(state.pixels);

    return EXIT_SUCCESS;
}
