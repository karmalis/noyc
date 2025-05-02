#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <wayland-util.h>


#include "iperlin.h"
#include "wayland/buffer.h"
#include "wayland/sharedmem.h"
#include "wayland/xdg-shell-protocol.h"

static const int WIDTH = 1024;
static const int HEIGHT = 1024;

static struct wl_buffer* draw_frame(struct app_state* state) {
     int stride = WIDTH * 4;
     int size = HEIGHT * stride * 2;

    int fd = allocate_shm_file(size);
    uint8_t *pool_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
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

int main(int argc, char** argv) {

    struct wl_display* display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "Failed to connect to wayland display.\n");
    }

    fprintf(stderr, "Connection established.\n");

    struct wl_registry* registry = wl_display_get_registry(display);
    if (!registry) {
        fprintf(stderr, "Failed to get wayland registry.\n");
    }

    struct app_state state = {0};

     // 8 0.75 0.00095 0.5 initial values
    int octaves = 8;
    double per = 0.75;
    double bfreq = 0.00095;
    double bamp = 0.5;

    int width = 1024;
    int height = 1024;

    state.pixels = malloc(sizeof(uint32_t) * width * height);

     for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            size_t index = (size_t)(y*width + x);
            /*
              double n_v = (iperlin_at((double) x / 64.f, (double) y / 64.f, 0.0f) * 1.00 +
              iperlin_at((double) x / 32.f, (double) y / 32.f, 0.0f) * 0.5f +
              iperlin_at((double) x / 16.f, (double) y / 16.f, 0.0f) * 0.25f) / 1.75;
            */
            double n_v = octave_iperlin_at((double) x, (double) y, 0.0, octaves, per, bfreq, bamp);

            uint8_t val = (uint8_t)((n_v * 0.5 + 0.5) * 255.0);

            uint8_t construct[] = {255, val, val, val}; // TODO check endianessness
            uint32_t pixel = *((uint32_t*)&construct);

            state.pixels[index] = pixel;
        }
    }

    wl_registry_add_listener(registry, &registry_listener, &state);
    wl_display_roundtrip(display);

    state.surface = wl_compositor_create_surface(state.compositor);
    struct wl_shell_surface* shell_surface = wl_shell_get_shell_surface(state.shell, state.surface);
//    wl_shell_surface_set_toplevel(shell_surface);





    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_damage(surface, 0, 0, UINT32_MAX, UINT32_MAX);
    wl_surface_commit(surface);

    wl_display_disconnect(display);

    free(pixels);

    return EXIT_SUCCESS;
}
