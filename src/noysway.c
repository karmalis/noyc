#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <wayland-util.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>

#include "wayland/xdg-shell-protocol.h"

#include "sharedmem.h"
#include "iperlin.h"

#define DEFAULT_NOISE_OCTAVES 8;
#define DEFAULT_NOISE_PER 0.75;
#define DEFAULT_NOISE_BASE_FREQ 0.00095;
#define DEFAULT_NOISE_BASE_AMP 0.5;

// Pixel noise state
struct noise_state {
    int octaves;
    double per;
    double bfreq;
    double bamp;
};

// Overwrites
void generate_noise(int width, int height, double depth, struct noise_state* noise, uint32_t* pixels) {
 for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t index = (size_t)(y*width + x);
            double n_v = octave_iperlin_at(
                (double) x, (double) y, depth,
                noise->octaves,
                noise->per,
                noise->bfreq,
                noise->bamp);
            uint8_t val = (uint8_t)((n_v * 0.5 + 0.5) * 255.0);
            uint8_t construct[] = {val, val, val, 255};
            uint32_t pixel = *((uint32_t*)&construct);
            pixels[index] = pixel;
        }
    }

}

// Our applciation state
struct app_state {
    // Wayland
    struct wl_display* display;
    struct wl_registry* registry;
    struct wl_compositor* compositor;
    struct wl_surface* surface;

    // Shared memory
    struct wl_shm* shm;

    //XDG structures
    struct xdg_wm_base* xdg_wm_base;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;

    // runtime params
    int closed;
    int regenerate;
    int width;
    int height;
    double depth;

    uint32_t elapsed;
    uint32_t last_frame;

    struct noise_state noise;

    // pixels in 0xRRGGBBAA format
    uint32_t* pixels;
};


// Buffer cleanup
static void wl_buffer_release(void* data, struct wl_buffer* buffer) {
    wl_buffer_destroy(buffer);
}

static const struct wl_buffer_listener buffer_listener = {
.release = wl_buffer_release
};

/// Drawing will require utilising and copying to a shared memory
// area
static struct wl_buffer* draw_frame(struct app_state* app) {
    int stride = app->width * 4; // uint32_t is 4 bytes
    int size = app->height * stride;

    int fd = allocate_shm_file(size); // allocate shared memory
    uint32_t* data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        close(fd);
        return NULL;
    }

    struct wl_shm_pool* pool = wl_shm_create_pool(app->shm, fd, size);
    int index = 0; // When data is stored in a single buffer, this could be used as the index
    int offset = app->height * stride * index;
    struct wl_buffer* buffer = wl_shm_pool_create_buffer(pool, offset, app->width, app->height, stride, WL_SHM_FORMAT_XRGB8888);

    wl_shm_pool_destroy(pool);
    close(fd);

    uint32_t* pixels = (uint32_t*) &data[offset];
    memcpy(pixels, app->pixels, size);

    munmap(data, size);

    // We can now add a listener for this particular draw buffer udpates
    wl_buffer_add_listener(buffer, &buffer_listener, NULL);

    return buffer;
}

// Resizing
static void xdg_toplevel_configure(void* data, struct xdg_toplevel* xdg_toplevel, int32_t width, int32_t height,
                                   struct wl_array* states) {
    struct app_state* app = (struct app_state*) data;
    if (width == 0 || height == 0) {
        return;
    }

    app->width = (int) width;
    app->height = (int) height;
}

// Window is being closed
static void xdg_toplevel_close(void* data, struct xdg_toplevel* toplevel) {
    struct app_state* app = (struct app_state*) data;
    app->closed = 1;
}

static void xdg_toplevel_configure_bounds()

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
.configure = xdg_toplevel_configure,
.close = xdg_toplevel_close,
.configure_bounds = xdg_toplevel_configure_bounds,
};

// function that basically means xdg surface setup is ready
static void xdg_surface_configure(void* data, struct xdg_surface* surface, uint32_t serial) {
    struct app_state* app = (struct app_state*) data;
    xdg_surface_ack_configure(surface, serial);

    struct wl_buffer *buffer = draw_frame(app);
    wl_surface_attach(app->surface, buffer, 0, 0);
    wl_surface_commit(app->surface);
}


static const struct xdg_surface_listener xdg_surface_listener = {
.configure = xdg_surface_configure
};


// A pong handler
static void xdg_base_ping(void* data, struct xdg_wm_base *wm_base, uint32_t serial) {
    xdg_wm_base_pong(wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
.ping = xdg_base_ping
};

// Updating frames
static const struct wl_callback_listener wl_surface_frame_listener;

static void wl_surface_frame_done(void* data, struct wl_callback *cb, uint32_t time) {
    wl_callback_destroy(cb);

    struct app_state* app = (struct app_state*) data;
    cb = wl_surface_frame(app->surface);
    wl_callback_add_listener(cb, &wl_surface_frame_listener, app);

    if (app->last_frame != 0) {
        uint32_t elapsed = time - app->last_frame;
        app->elapsed += elapsed;

        if (app->elapsed > 100) {
            app->depth += 1.0;
            generate_noise(app->width, app->height, app->depth, &app->noise, app->pixels);
            app->elapsed = 0;
        }
    }

    // Submit a frame for this event
    struct wl_buffer* buffer = draw_frame(app);
    wl_surface_attach(app->surface, buffer, 0, 0);
    wl_surface_damage_buffer(app->surface, 0, 0, INT32_MAX, INT32_MAX);
    wl_surface_commit(app->surface);

    // This is where delta time can be set
    app->last_frame = time;
}

static const struct wl_callback_listener wl_surface_frame_listener = {
.done = wl_surface_frame_done,
};

static void registry_handle_global(void *data, struct wl_registry* registry,
                       uint32_t name, const char* interface, uint32_t version) {
    // ...fetch registry entries
    struct app_state* app = (struct app_state*) data;
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        app->compositor = wl_registry_bind(
            registry, name, &wl_compositor_interface, 6);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        app->shm = wl_registry_bind(
            registry, name, &wl_shm_interface, 2);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        app->shm = wl_registry_bind(
            registry, name, &wl_shm_interface, 2);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        app->xdg_wm_base = wl_registry_bind(
            registry, name, &xdg_wm_base_interface, 5);
        xdg_wm_base_add_listener(app->xdg_wm_base, &xdg_wm_base_listener, app);
    }
}

static void registry_handle_global_remove(void *data, struct wl_registry *registry,
                              uint32_t name) {
    // ...handle cleanup
}

const struct wl_registry_listener registry_listener = {
.global = registry_handle_global,
.global_remove = registry_handle_global_remove
};

int main(int argc, char** argv) {

    struct app_state app = {0};
    // Initialise the display
    app.display = wl_display_connect(NULL);
    if (!app.display) {
        fprintf(stderr, "Failed to connect to wayland display.\n");
        return EXIT_FAILURE;
    }

    // Initialise the registry
    app.registry = wl_display_get_registry(app.display);
    if (!app.registry) {
        fprintf(stderr, "Failed to get wayland registry.\n");
        return EXIT_FAILURE;
    }

    // Setup or prepare the application
    wl_registry_add_listener(app.registry, &registry_listener, &app);

    // Display roundtrip should trigger some of the listeners, which
    // should allow us to proceed with more initialisation
    wl_display_roundtrip(app.display);


    // The display surface
    app.surface = wl_compositor_create_surface(app.compositor);
    if (!app.surface) {
        fprintf(stderr, "Failed to create Wayland surface\n");
        return EXIT_FAILURE;
    }

     // 8 0.75 0.00095 0.5 initial values
    struct noise_state noise = {0};
    noise.octaves = DEFAULT_NOISE_OCTAVES;
    noise.per = DEFAULT_NOISE_PER;
    noise.bfreq = DEFAULT_NOISE_BASE_FREQ;
    noise.bamp = DEFAULT_NOISE_BASE_AMP;

    app.closed = 0;
    app.width = 1024;
    app.height = 1024;
    app.depth = 0.0;
    app.elapsed = 0;
    app.last_frame = 0;
    app.noise = noise;

    app.pixels = malloc(sizeof(uint32_t) * app.width * app.height);

    generate_noise(app.width, app.height, app.depth, &app.noise, app.pixels);

    // we can now setup xdg surfaces and parts
    app.xdg_surface = xdg_wm_base_get_xdg_surface(app.xdg_wm_base, app.surface);
    xdg_surface_add_listener(app.xdg_surface, &xdg_surface_listener, &app);

    // This means toplevel surface that represents the app
    app.xdg_toplevel = xdg_surface_get_toplevel(app.xdg_surface);
    xdg_toplevel_add_listener(app.xdg_toplevel,
                              &xdg_toplevel_listener, &app);

    // Window / surface title
    xdg_toplevel_set_title(app.xdg_toplevel, "Example Application");

    // As xdg is setup, xdg_surface_configure will be called
    // from which the initial pixel can be drawn.
    // After that, the pixels can be redrawn either every frame
    // during the event loop or at another message based trigger
    wl_surface_commit(app.surface);

    struct wl_callback* cb = wl_surface_frame(app.surface);
    wl_callback_add_listener(cb, &wl_surface_frame_listener, &app);

    // Possible event loop
    while (wl_display_dispatch(app.display) && !app.closed) {
        // iterate
    }

    wl_display_disconnect(app.display);
    free(app.pixels);

    return EXIT_SUCCESS;
}
