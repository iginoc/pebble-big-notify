#include <pebble.h>

static Window *s_window;

static TextLayer *s_time_layer;

static PropertyAnimation *s_scroll_anim;
#define SCROLL_PX_PER_SEC 30   // velocità: più basso = più lento
static GFont s_big_font;
static Layer *s_msg_layer;
static char s_first[64];
static char s_rest[64];
static int16_t s_first_h;
static void prv_msg_update(Layer *layer, GContext *ctx);

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_big_font = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);

  // Titolo centrato
  s_time_layer = text_layer_create(
      GRect(0, (bounds.size.h - 34) / 2, bounds.size.w, 34));
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text(s_time_layer, "Wathsapp");
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  // Messaggio: nascosto finché non arriva qualcosa
  s_msg_layer = layer_create(GRect(0, 20, bounds.size.w, bounds.size.h - 20));
  layer_set_update_proc(s_msg_layer, prv_msg_update);
  layer_set_hidden(s_msg_layer, true);
  layer_add_child(window_layer, s_msg_layer);
  

}

static void prv_window_unload(Window *window) {
  if (s_scroll_anim) {
    animation_unschedule((Animation *)s_scroll_anim);
    s_scroll_anim = NULL;
    text_layer_destroy(s_time_layer);
  }
    layer_destroy(s_msg_layer);
  text_layer_destroy(s_time_layer);
} 

static void prv_msg_update(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);

  graphics_context_set_text_color(ctx, PBL_IF_COLOR_ELSE(GColorRed, GColorBlack));
  graphics_draw_text(ctx, s_first, s_big_font,
                     GRect(0, 0, b.size.w, s_first_h),
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, s_rest, s_big_font,
                     GRect(0, s_first_h, b.size.w, b.size.h - s_first_h),
                     GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

static void prv_start_scroll(void) {
  GRect bounds = layer_get_bounds(window_get_root_layer(s_window));
  GRect box = GRect(0, 0, bounds.size.w, 2000);

  s_first_h = graphics_text_layout_get_content_size(
      s_first, s_big_font, box,
      GTextOverflowModeWordWrap, GTextAlignmentCenter).h;
  int16_t rest_h = s_rest[0] ? graphics_text_layout_get_content_size(
      s_rest, s_big_font, box,
      GTextOverflowModeWordWrap, GTextAlignmentCenter).h : 0;
  int16_t th = s_first_h + rest_h + 10;

  if (s_scroll_anim) {
    animation_unschedule((Animation *)s_scroll_anim);
    s_scroll_anim = NULL;
  }

  GRect from = GRect(0, bounds.size.h, bounds.size.w, th);
  GRect to   = GRect(0, -th,           bounds.size.w, th);
  layer_set_frame(s_msg_layer, from);

  s_scroll_anim = property_animation_create_layer_frame(s_msg_layer, &from, &to);
  Animation *anim = property_animation_get_animation(s_scroll_anim);
  animation_set_duration(anim, (bounds.size.h + th) * 1000 / SCROLL_PX_PER_SEC);
  animation_set_curve(anim, AnimationCurveLinear);
  animation_set_play_count(anim, ANIMATION_PLAY_COUNT_INFINITE);
  animation_schedule(anim);
}


static void prv_inbox_received(DictionaryIterator *iter, void *context) {
  Tuple *t = dict_find(iter, 0);
  if (!t || t->type != TUPLE_CSTRING || !s_msg_layer || !s_time_layer) return;

  const char *msg = t->value->cstring;
  const char *nl = strchr(msg, '\n');
  if (nl) {
    size_t n = nl - msg;
    if (n >= sizeof(s_first)) n = sizeof(s_first) - 1;
    memcpy(s_first, msg, n);
    s_first[n] = '\0';
    snprintf(s_rest, sizeof(s_rest), "%s", nl + 1);
  } else {
    snprintf(s_first, sizeof(s_first), "%s", msg);
    s_rest[0] = '\0';
  }

  layer_set_hidden(text_layer_get_layer(s_time_layer), true);
  layer_set_hidden(s_msg_layer, false);
  prv_start_scroll();
}

static void prv_tap_handler(AccelAxisType axis, int32_t direction) {
  if (s_msg_layer && !layer_get_hidden(s_msg_layer)) {
    window_stack_pop_all(true);
  }
}

static void prv_init(void) {
  s_window = window_create();
  accel_tap_service_subscribe(prv_tap_handler);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;  
  app_message_register_inbox_received(prv_inbox_received);
  app_message_open(128, 32);
  window_stack_push(s_window, animated);
   //vibes_short_pulse(); // vibrazione corta
  //vibes_long_pulse(); //per una vibrazione lunga
  vibes_double_pulse(); //per due impulsi brevi
}

static void prv_deinit(void) {
  window_destroy(s_window);
  accel_tap_service_unsubscribe();
}

int main(void) {
  prv_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

  app_event_loop();
  prv_deinit();
}
