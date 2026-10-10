#include <pebble.h>

static Window *s_window;
static TextLayer *s_text_layer;
static char s_buffer[64];
static PropertyAnimation *s_scroll_anim;
#define SCROLL_PX_PER_SEC 30   // velocità: più basso = più lento
static GFont s_big_font;
static TextLayer *s_time_layer;

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
  s_text_layer = text_layer_create(GRect(0, 20, bounds.size.w, bounds.size.h - 20));
  text_layer_set_font(s_text_layer, s_big_font);
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  layer_set_hidden(text_layer_get_layer(s_text_layer), true);
  layer_add_child(window_layer, text_layer_get_layer(s_text_layer));
}

static void prv_window_unload(Window *window) {
  if (s_scroll_anim) {
    animation_unschedule((Animation *)s_scroll_anim);
    s_scroll_anim = NULL;
    text_layer_destroy(s_time_layer);
  }
  text_layer_destroy(s_text_layer);
  text_layer_destroy(s_time_layer);
} 

static void prv_start_scroll(void) {
  Layer *root = window_get_root_layer(s_window);
  GRect bounds = layer_get_bounds(root);
  Layer *text_layer = text_layer_get_layer(s_text_layer);

  // Altezza reale del testo con il font in uso
  GSize size = graphics_text_layout_get_content_size(
      s_buffer, s_big_font,
      GRect(0, 0, bounds.size.w, 2000),
      GTextOverflowModeWordWrap, GTextAlignmentCenter);
  int16_t th = size.h + 10;

  // Ferma l'animazione precedente (se c'è)
  if (s_scroll_anim) {
    animation_unschedule((Animation *)s_scroll_anim);
    s_scroll_anim = NULL;
  }

  GRect from = GRect(0, bounds.size.h, bounds.size.w, th);
  GRect to   = GRect(0, -th,           bounds.size.w, th);
  layer_set_frame(text_layer, from);

  s_scroll_anim = property_animation_create_layer_frame(text_layer, &from, &to);
  Animation *anim = property_animation_get_animation(s_scroll_anim);
  animation_set_duration(anim, (bounds.size.h + th) * 1000 / SCROLL_PX_PER_SEC);
  animation_set_curve(anim, AnimationCurveLinear);
  animation_set_play_count(anim, ANIMATION_PLAY_COUNT_INFINITE);
  animation_schedule(anim);
}

static void prv_inbox_received(DictionaryIterator *iter, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "1 inbox ricevuto");
  Tuple *t = dict_find(iter, 0);
  if (!t || t->type != TUPLE_CSTRING) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "tupla assente o tipo errato");
    return;
  }
  if (!s_text_layer || !s_time_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "layer NULL: testo=%p titolo=%p", s_text_layer, s_time_layer);
    return;
  }
  snprintf(s_buffer, sizeof(s_buffer), "%s", t->value->cstring);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "2 buffer: %s", s_buffer);
  text_layer_set_text(s_text_layer, s_buffer);
  layer_set_hidden(text_layer_get_layer(s_time_layer), true);
  layer_set_hidden(text_layer_get_layer(s_text_layer), false);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "3 layer aggiornati");
  prv_start_scroll();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "4 scroll avviato");
}

static void prv_tap_handler(AccelAxisType axis, int32_t direction) {
  if (s_scroll_anim) {
    animation_unschedule((Animation *)s_scroll_anim);
    s_scroll_anim = NULL;
  }
  layer_set_hidden(text_layer_get_layer(s_text_layer), true);
  layer_set_hidden(text_layer_get_layer(s_time_layer), false);
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
