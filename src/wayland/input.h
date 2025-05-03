#ifndef INPUT_H_
#define INPUT_H_

#include "xkbcommon/xkbcommon.h"

#include <stdint.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

enum pointer_event_mask {
  POINTER_EVENT_ENTER = 1 << 0,
  POINTER_EVENT_LEAVE = 1 << 1,
  POINTER_EVENT_MOTION = 1 << 2,
  POINTER_EVENT_BUTTON = 1 << 3,
  POINTER_EVENT_AXIS = 1 << 4,
  POINTER_EVENT_AXIS_SOURCE = 1 << 5,
  POINTER_EVENT_AXIS_STOP = 1 << 6,
  POINTER_EVENT_AXIS_DISCRETE = 1 << 7
};

struct pointer_event {
    uint32_t event_mask;
    wl_fixed_t surface_x;
    wl_fixed_t surface_y;
    uint32_t button;
    uint32_t state;
    uint32_t time;
    uint32_t serial;
    struct {
        int valid;
        wl_fixed_t value;
        int32_t discrete;
    } axes[2];
    uint32_t axis_source;
};

extern const struct wl_pointer_listener wl_pointer_listener;
extern const struct wl_keyboard_listener wl_keyboard_listener;

#endif // INPUT_H_
