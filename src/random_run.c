#include <pebble.h>
#include "random_run.h"
#include "timer.h"
#include "util.h"

static Window *window;
static Layer *window_layer;
static TextLayer *timer_text_layer;
static TextLayer *distance_text_layer;

enum unit8_t {
  KEY_COMMAND,
  KEY_DISTANCE,
  KEY_MANEUVER
};

void fill_top_rect(Layer *m, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorBlack);
  GRect fillBounds = GRect(0, 0, 148, 20);

  graphics_fill_rect(ctx, fillBounds, 0, GCornerNone);
}

void draw_top_rect() {
  GRect bounds = layer_get_frame(window_layer);
  Layer *rect_layer = layer_create(bounds);
  layer_set_update_proc(rect_layer, fill_top_rect);
  layer_add_child(window_layer, rect_layer);
}

void update_timer_display(uint32_t tick) {
  static char formatted[9];

  format_time(tick, tick >= 3600, formatted, 9);

  text_layer_set_text(timer_text_layer, formatted);
  layer_mark_dirty(text_layer_get_layer(timer_text_layer));
}

void in_received_handler(DictionaryIterator *iter, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Synced");

    Tuple *distance;

    if (dict_find(iter, KEY_COMMAND)->value->int8 == 0) {
      distance     = dict_find(iter, KEY_DISTANCE);

      APP_LOG(APP_LOG_LEVEL_DEBUG, "START %s", distance->value->cstring);
      start_timer(&update_timer_display);

      text_layer_set_text(distance_text_layer, distance->value->cstring);
      layer_mark_dirty(text_layer_get_layer(distance_text_layer));
    }

    if (dict_find(iter, KEY_COMMAND)->value->int8 == 1) {
      stop_timer();
      clear_timer();
      update_timer_display(0);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "END");
    }

    if (dict_find(iter, KEY_COMMAND)->value->int8 == 2) {
      distance  = dict_find(iter, KEY_DISTANCE);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "UPDATE %s", distance->value->cstring);

      text_layer_set_text(distance_text_layer, distance->value->cstring);
      layer_mark_dirty(text_layer_get_layer(distance_text_layer));
    }

    if(dict_find(iter, KEY_COMMAND)->value->int8 == 3) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "KEY_MANEUVER");
      vibes_short_pulse();
    }
}

void in_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error %o", reason );
}

void set_time_display(Layer *window_layer) {
  GRect bounds = layer_get_frame(window_layer);
  timer_text_layer = text_layer_create((GRect){ .origin = { 0, 20 }, .size = bounds.size });

  draw_top_rect();
  text_layer_set_text_color(timer_text_layer, GColorWhite);
  text_layer_set_text(timer_text_layer, "Start!");
  text_layer_set_background_color(timer_text_layer, GColorBlack);
  text_layer_set_font(timer_text_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
  text_layer_set_text_alignment(timer_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(timer_text_layer));
}

void set_distance_display(Layer *window_layer) {
  GRect bounds = layer_get_frame(window_layer);
  distance_text_layer = text_layer_create((GRect){ .origin = { 0, 100 }, .size = bounds.size });

  text_layer_set_text_color(distance_text_layer, GColorBlack);
  text_layer_set_text(distance_text_layer, "");
  text_layer_set_background_color(distance_text_layer, GColorWhite);
  text_layer_set_font(distance_text_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
  text_layer_set_text_alignment(distance_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(distance_text_layer));
}

void init() {
  window = window_create();
  window_stack_push(window, true);

  window_layer = window_get_root_layer(window);

  set_time_display(window_layer);
  set_distance_display(window_layer);

  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);

  const uint32_t inbound_size = app_message_inbox_size_maximum();
  const uint32_t outbound_size = 64;
  app_message_open(inbound_size, outbound_size);
}

void deinit() {
  text_layer_destroy(timer_text_layer);
  text_layer_destroy(distance_text_layer);
  stop_timer();
  window_destroy(window);
}