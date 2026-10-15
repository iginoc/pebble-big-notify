#include <pebble.h>

static Window *s_window = NULL;
static PropertyAnimation *s_scroll_anim = NULL;
static TextLayer *s_header_layer = NULL;
static Layer *s_msg_layer = NULL;

#define HEADER_H 30
#define SCROLL_PX_PER_SEC 25

static GFont s_big_font;
static char s_header_text[64] = "In attesa...";
static char s_rest[128] = "";
static int16_t s_content_h = 0;

static void prv_cancel_anim(void) {
  if (s_scroll_anim) {
    animation_unschedule((Animation *)s_scroll_anim);
    // Non chiamare property_animation_destroy: unschedule basta
    s_scroll_anim = NULL;
  }
}

static void prv_msg_update(Layer *layer, GContext *ctx) {
  if (!s_rest[0]) return;
  GRect b = layer_get_bounds(layer);
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, s_rest, s_big_font,
                     GRect(4, 0, b.size.w - 8, b.size.h),
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

static void prv_start_scroll(void) {
  if (!s_window || !s_msg_layer) return;

  prv_cancel_anim();

  GRect win = layer_get_bounds(window_get_root_layer(s_window));
  int view_h = win.size.h - HEADER_H;

  GRect box = GRect(0, 0, win.size.w - 8, 2000);
  s_content_h = s_rest[0] ? graphics_text_layout_get_content_size(
      s_rest, s_big_font, box,
      GTextOverflowModeWordWrap, GTextAlignmentCenter).h + 20 : 50;

  GRect from = GRect(0, win.size.h, win.size.w, s_content_h);
  GRect to   = GRect(0, HEADER_H - s_content_h, win.size.w, s_content_h);

  layer_set_frame(s_msg_layer, from);
  layer_set_hidden(s_msg_layer, false);
  layer_mark_dirty(s_msg_layer);

  s_scroll_anim = property_animation_create_layer_frame(s_msg_layer, &from, &to);
  if (!s_scroll_anim) return;

  Animation *anim = property_animation_get_animation(s_scroll_anim);
  animation_set_duration(anim, (uint32_t)((view_h + s_content_h) * 1000 / SCROLL_PX_PER_SEC));
  animation_set_curve(anim, AnimationCurveLinear);
  animation_set_play_count(anim, ANIMATION_PLAY_COUNT_INFINITE);
  animation_schedule(anim);
}

static void clean_first_line(char *out, size_t out_size, const char *in) {
  size_t j = 0;
  int inside = 0;
  for (size_t i = 0; in[i] && j < out_size - 1; i++) {
    if (in[i] == '(') { inside = 1; continue; }
    if (in[i] == ')') { inside = 0; continue; }
    if (!inside) out[j++] = in[i];
  }
  out[j] = '\0';
  while (out[0] == ' ') memmove(out, out + 1, strlen(out));
  if (out[0] == '\0') snprintf(out, out_size, "Messaggio");
}

static void update_header_style(const char *msg) {
  if (!s_header_layer) return;
  if (strncmp(msg, "(W)", 3) == 0) {
    text_layer_set_background_color(s_header_layer, GColorGreen);
    text_layer_set_text_color(s_header_layer, GColorBlack);
  } else if (strncmp(msg, "(SMS)", 5) == 0) {
    text_layer_set_background_color(s_header_layer, GColorBlue);
    text_layer_set_text_color(s_header_layer, GColorWhite);
  } else {
    text_layer_set_background_color(s_header_layer, GColorRed);
    text_layer_set_text_color(s_header_layer, GColorWhite);
  }
}

static void prv_inbox_received(DictionaryIterator *iter, void *context) {
  Tuple *t = dict_read_first(iter);
  const char *msg = NULL;

  while (t) {
    if (t->type == TUPLE_CSTRING) {
      msg = t->value->cstring;
      break;
    }
    t = dict_read_next(iter);
  }
  if (!msg) return;

  const char *nl = strchr(msg, '\n');
  char raw_first[64];

  if (nl) {
    size_t n = (size_t)(nl - msg);
    if (n >= sizeof(raw_first)) n = sizeof(raw_first) - 1;
    memcpy(raw_first, msg, n);
    raw_first[n] = '\0';
    snprintf(s_rest, sizeof(s_rest), "%s", nl + 1);
  } else {
    snprintf(raw_first, sizeof(raw_first), "%s", msg);
    s_rest[0] = '\0';
  }

  clean_first_line(s_header_text, sizeof(s_header_text), raw_first);
  if (s_header_layer) {
    text_layer_set_text(s_header_layer, s_header_text);
    update_header_style(msg);
  }

  prv_start_scroll();
  vibes_double_pulse();
}

static void prv_tap_handler(AccelAxisType axis, int32_t direction) {
  window_stack_pop_all(true);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_big_font = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);

  s_msg_layer = layer_create(bounds);
  layer_set_update_proc(s_msg_layer, prv_msg_update);
  layer_set_hidden(s_msg_layer, true);
  layer_add_child(window_layer, s_msg_layer);

  // Header per ultimo → sempre sopra
  s_header_layer = text_layer_create(GRect(0, 0, bounds.size.w, HEADER_H));
  text_layer_set_background_color(s_header_layer, GColorRed);
  text_layer_set_text_color(s_header_layer, GColorWhite);
  text_layer_set_font(s_header_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_header_layer, GTextAlignmentCenter);
  text_layer_set_text(s_header_layer, s_header_text);
  layer_add_child(window_layer, text_layer_get_layer(s_header_layer));
}

static void prv_window_unload(Window *window) {
  prv_cancel_anim();

  if (s_header_layer) {
    text_layer_destroy(s_header_layer);
    s_header_layer = NULL;
  }
  if (s_msg_layer) {
    layer_destroy(s_msg_layer);
    s_msg_layer = NULL;
  }
}

static void prv_init(void) {
  s_window = window_create();
  accel_tap_service_subscribe(prv_tap_handler);

  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });

  app_message_register_inbox_received(prv_inbox_received);
  app_message_open(128, 32);

  window_stack_push(s_window, true);
}

static void prv_deinit(void) {
  accel_tap_service_unsubscribe();
  prv_cancel_anim();
  if (s_window) {
    window_destroy(s_window);
    s_window = NULL;
  }
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}