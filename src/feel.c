#include <pebble.h>
#define KEY_TEMPERATURE 0
#define KEY_HUMIDITY 1

static Window *window;
static TextLayer *t_layer;
static GFont *t_font;
static TextLayer *dp_layer;
static GFont *dp_font;

static void update_time() {
  time_t tmp = time(NULL);
  struct tm *tick_time = localtime(&tmp);
  static char buffer[] = "00:00";
  // write current time to buffer
  if (clock_is_24h_style() == true) {
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } // 24h time
  else {
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  } // 12h time
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  if (tick_time->tm_min % 30 == 0) {
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter,0,0); // add key-value pair
    app_message_outbox_send();
  }
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  // temperature-layer
  t_layer = text_layer_create(GRect(0, 30, bounds.size.w, bounds.size.h));
  text_layer_set_background_color(t_layer, GColorClear);
  text_layer_set_text_color(t_layer, GColorBlack);
  text_layer_set_text_alignment(t_layer, GTextAlignmentCenter);
  t_font = fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT);
  text_layer_set_font(t_layer,t_font);
  layer_add_child(window_layer,text_layer_get_layer(t_layer));
  dp_layer = text_layer_create(GRect(0, 75, bounds.size.w, bounds.size.h));
  // dew-point layer
  text_layer_set_background_color(dp_layer, GColorClear);
  text_layer_set_text_color(dp_layer, GColorBlack);
  text_layer_set_text_alignment(dp_layer, GTextAlignmentCenter);
  dp_font = fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT);
  text_layer_set_font(dp_layer,dp_font);
  layer_add_child(window_layer,text_layer_get_layer(dp_layer));
  update_time();
}

static void window_unload(Window *window) {
  text_layer_destroy(dp_layer);
  text_layer_destroy(t_layer);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  static char temperature_buffer[8];
  static char humidity_buffer[8];
  static char t_layer_buffer[8];
  static char dp_layer_buffer[8];
  Tuple *t = dict_read_first(iterator);
  while(t != NULL) {
    switch(t->key) {
      case KEY_TEMPERATURE:
        snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)t->value->int32);
        break;
      case KEY_HUMIDITY:
        snprintf(humidity_buffer, sizeof(humidity_buffer), "%d%%", (int)t->value->int32);
        break;
      default:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognised!", (int)t->key);
        break;
    }
    t = dict_read_next(iterator);
  }
  // construct full string, display string
  snprintf(t_layer_buffer, sizeof(t_layer_buffer), "%s", temperature_buffer);
  text_layer_set_text(t_layer, t_layer_buffer);
  snprintf(dp_layer_buffer, sizeof(dp_layer_buffer), "%s", humidity_buffer);
  text_layer_set_text(dp_layer, dp_layer_buffer);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  window_stack_push(window, true);
  // register timer
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  // registered callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  // open appmessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initialising, pushed window: %p", window);
  app_event_loop();
  deinit();
}