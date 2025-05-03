#include "input.h"
#include "wayland.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon-names.h>
#include <xkbcommon/xkbcommon.h>

static void wl_pointer_enter(void *data, struct wl_pointer *pointer,
                             uint32_t serial, struct wl_surface *surface,
                             wl_fixed_t surface_x, wl_fixed_t surface_y) {
  struct app_state *state = (struct app_state *)data;

  state->pointer_event.event_mask |= POINTER_EVENT_ENTER;
  state->pointer_event.serial = serial;
  state->pointer_event.surface_x = surface_x;
  state->pointer_event.surface_y = surface_y;
}

static void wl_pointer_leave(void *data, struct wl_pointer *pointer,
                             uint32_t serial, struct wl_surface *surface) {
  struct app_state *state = (struct app_state *)data;
  state->pointer_event.event_mask |= POINTER_EVENT_LEAVE;
  state->pointer_event.serial = serial;
}

static void wl_pointer_motion(void *data, struct wl_pointer *pointer,
                              uint32_t time, wl_fixed_t surface_x,
                              wl_fixed_t surface_y) {
  struct app_state *state = (struct app_state *)data;
  state->pointer_event.event_mask |= POINTER_EVENT_MOTION;
  state->pointer_event.time = time;
  state->pointer_event.surface_x = surface_x;
  state->pointer_event.surface_y = surface_y;
}

static void wl_pointer_button(void *data, struct wl_pointer *pointer,
                              uint32_t serial, uint32_t time, uint32_t button,
                              uint32_t state) {
  struct app_state *cstate = (struct app_state *)data;
  cstate->pointer_event.event_mask |= POINTER_EVENT_BUTTON;
  cstate->pointer_event.time = time;
  cstate->pointer_event.serial = serial;
  cstate->pointer_event.button = button;
  cstate->pointer_event.state = state;
}

static void wl_pointer_axis(void *data, struct wl_pointer *pointer,
                            uint32_t time, uint32_t axis, wl_fixed_t value) {
  struct app_state *state = (struct app_state *)data;
  state->pointer_event.event_mask |= POINTER_EVENT_AXIS;
  state->pointer_event.time = time;
  state->pointer_event.axes[axis].valid = 1;
  state->pointer_event.axes[axis].value = value;
}

static void wl_pointer_axis_source(void *data, struct wl_pointer *pointer,
                                   uint32_t axis_source) {
  struct app_state *state = (struct app_state *)data;
  state->pointer_event.event_mask |= POINTER_EVENT_AXIS_SOURCE;
  state->pointer_event.axis_source = axis_source;
}

static void wl_pointer_axis_stop(void *data, struct wl_pointer *pointer,
                                 uint32_t time, uint32_t axis) {
  struct app_state *state = (struct app_state *)data;
  state->pointer_event.time = time;
  state->pointer_event.event_mask |= POINTER_EVENT_AXIS_STOP;
  state->pointer_event.axes[axis].valid = 1;
}

static void wl_pointer_axis_discrete(void *data, struct wl_pointer *pointer,
                                     uint32_t axis, int32_t discrete) {
  struct app_state *state = (struct app_state *)data;
  state->pointer_event.event_mask |= POINTER_EVENT_AXIS_DISCRETE;
  state->pointer_event.axes[axis].valid = 1;
  state->pointer_event.axes[axis].discrete = discrete;
}

static void wl_pointer_frame_debug(void *data, struct wl_pointer *pointer) {
  struct app_state *state = (struct app_state *)data;
  struct pointer_event *event = &state->pointer_event;
  fprintf(stderr, "pointer frame @ %d: ", event->time);

  if (event->event_mask & POINTER_EVENT_ENTER) {
    fprintf(stderr, "entered %f, %f ", wl_fixed_to_double(event->surface_x),
            wl_fixed_to_double(event->surface_y));
  }

  if (event->event_mask & POINTER_EVENT_LEAVE) {
    fprintf(stderr, "leave");
  }

  if (event->event_mask & POINTER_EVENT_MOTION) {
    fprintf(stderr, "motion %f, %f ", wl_fixed_to_double(event->surface_x),
            wl_fixed_to_double(event->surface_y));
  }

  if (event->event_mask & POINTER_EVENT_BUTTON) {
    char *state = event->state == WL_POINTER_BUTTON_STATE_RELEASED ? "released"
                                                                   : "pressed";
    fprintf(stderr, "button %d %s ", event->button, state);
  }

  uint32_t axis_events = POINTER_EVENT_AXIS | POINTER_EVENT_AXIS_SOURCE |
                         POINTER_EVENT_AXIS_STOP | POINTER_EVENT_AXIS_DISCRETE;
  char *axis_name[2] = {
      [WL_POINTER_AXIS_VERTICAL_SCROLL] = "vertical",
      [WL_POINTER_AXIS_HORIZONTAL_SCROLL] = "horizontal",
  };
  char *axis_source[4] = {
      [WL_POINTER_AXIS_SOURCE_WHEEL] = "wheel",
      [WL_POINTER_AXIS_SOURCE_FINGER] = "finger",
      [WL_POINTER_AXIS_SOURCE_CONTINUOUS] = "continuous",
      [WL_POINTER_AXIS_SOURCE_WHEEL_TILT] = "wheel tilt",
  };
  if (event->event_mask & axis_events) {
    for (size_t i = 0; i < 2; ++i) {
      if (!event->axes[i].valid) {
        continue;
      }
      fprintf(stderr, "%s axis ", axis_name[i]);
      if (event->event_mask & POINTER_EVENT_AXIS) {
        fprintf(stderr, "value %f ", wl_fixed_to_double(event->axes[i].value));
      }
      if (event->event_mask & POINTER_EVENT_AXIS_DISCRETE) {
        fprintf(stderr, "discrete %d ", event->axes[i].discrete);
      }
      if (event->event_mask & POINTER_EVENT_AXIS_SOURCE) {
        fprintf(stderr, "via %s ", axis_source[event->axis_source]);
      }
      if (event->event_mask & POINTER_EVENT_AXIS_STOP) {
        fprintf(stderr, "(stopped) ");
      }
    }
  }

  fprintf(stderr, "\n");
  memset(event, 0, sizeof(*event));
}

static void wl_pointer_frame_void(void *data, struct wl_pointer *pointer) {}

const struct wl_pointer_listener wl_pointer_listener = {
    .enter = wl_pointer_enter,
    .leave = wl_pointer_leave,
    .motion = wl_pointer_motion,
    .button = wl_pointer_button,
    .axis = wl_pointer_axis,
    .frame = wl_pointer_frame_void,
    .axis_source = wl_pointer_axis_source,
    .axis_stop = wl_pointer_axis_stop,
    .axis_discrete = wl_pointer_axis_discrete,
};

static void wl_keyboard_keymap(void *data, struct wl_keyboard *keyboard,
                               uint32_t format, int32_t fd, uint32_t size) {
  struct app_state *state = (struct app_state *)data;
  assert(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1);

  char *map_shm = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
  assert(map_shm != MAP_FAILED);

  struct xkb_keymap *xkb_keymap = xkb_keymap_new_from_string(
      state->xkb_context, map_shm, XKB_KEYMAP_FORMAT_TEXT_V1,
      XKB_KEYMAP_COMPILE_NO_FLAGS);
  munmap(map_shm, size);
  close(fd);

  struct xkb_state *xkb_state = xkb_state_new(xkb_keymap);
  xkb_keymap_unref(state->xkb_keymap);
  xkb_state_unref(state->xkb_state);
  state->xkb_keymap = xkb_keymap;
  state->xkb_state = xkb_state;
}

static void wl_keyboard_enter(void *data, struct wl_keyboard *keyboard,
                              uint32_t serial, struct wl_surface *surface,
                              struct wl_array *keys) {
  struct app_state *state = (struct app_state *)data;
  fprintf(stderr, "keyboard enter; keys pressed are:\n");
  uint32_t *key;
  wl_array_for_each(key, keys) {
    char buf[128];
    xkb_keysym_t sym = xkb_state_key_get_one_sym(state->xkb_state, *key + 8);
    xkb_keysym_get_name(sym, buf, sizeof(buf));
    fprintf(stderr, "sym: %-12s (%d), ", buf, sym);
    xkb_state_key_get_utf8(state->xkb_state, *key + 8, buf, sizeof(buf));
    fprintf(stderr, "utf8: '%s'\n", buf);
  }
}

static void wl_keyboard_key(void *data, struct wl_keyboard *keyboard,
                            uint32_t serial, uint32_t time, uint32_t key,
                            uint32_t state) {
  struct app_state *cstate = (struct app_state *)data;
  char buf[128];
  uint32_t keycode = key + 8;
  xkb_keysym_t sym = xkb_state_key_get_one_sym(cstate->xkb_state, keycode);
  xkb_keysym_get_name(sym, buf, sizeof(buf));
  const char *action =
      state == WL_KEYBOARD_KEY_STATE_PRESSED ? "press" : "release";
  fprintf(stderr, "key %s: sym: %-12s (%d), ", action, buf, sym);
  xkb_state_key_get_utf8(cstate->xkb_state, keycode, buf, sizeof(buf));
  fprintf(stderr, "utf8: '%s'\n", buf);

  if (sym == 32 && state == WL_KEYBOARD_KEY_STATE_RELEASED) {
      cstate->regenerate = 1;
  }
}

static void wl_keyboard_leave(void *data, struct wl_keyboard *keyboard,
                              uint32_t serial, struct wl_surface *surface) {
  fprintf(stderr, "keyboard leave\n");
}

static void wl_keyboard_modifiers(void *data, struct wl_keyboard *keyboard,
                                  uint32_t serial, uint32_t mods_depressed,
                                  uint32_t mods_latched, uint32_t mods_locked,
                                  uint32_t group) {
  struct app_state *state = (struct app_state *)data;
  xkb_state_update_mask(state->xkb_state, mods_depressed, mods_latched,
                        mods_locked, 0, 0, group);
}

static void wl_keyboard_repeat_info(void *data, struct wl_keyboard *keyboard,
                                    int32_t rate, int32_t delay) {}

const struct wl_keyboard_listener wl_keyboard_listener = {
    .keymap = wl_keyboard_keymap,
    .enter = wl_keyboard_enter,
    .leave = wl_keyboard_leave,
    .key = wl_keyboard_key,
    .modifiers = wl_keyboard_modifiers,
    .repeat_info = wl_keyboard_repeat_info,
};
