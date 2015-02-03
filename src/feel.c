#include <pebble.h>
#define KEY_TEMPERATURE 0
#define KEY_DEWPOINT 1

static Window *window;
static BitmapLayer *background_layer;
static GBitmap *background_bitmap;
static TextLayer *temperature_layer;
static GFont *temperature_font;
static TextLayer *dewpoint_layer;
static GFont *dewpoint_font;

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
  // load background
  background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
  background_layer = bitmap_layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  bitmap_layer_set_bitmap(background_layer, background_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(background_layer));
  // temperature-layer
  temperature_layer = text_layer_create(GRect(0, 30, bounds.size.w, bounds.size.h));
  text_layer_set_background_color(temperature_layer, GColorBlack);
  text_layer_set_text_color(temperature_layer, GColorClear);
  text_layer_set_text_alignment(temperature_layer, GTextAlignmentCenter);
  temperature_font = fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD);
  text_layer_set_font(temperature_layer, temperature_font);
  layer_add_child(window_layer, text_layer_get_layer(temperature_layer));
  // dew-point layer
  dewpoint_layer = text_layer_create(GRect(0, 75, bounds.size.w, bounds.size.h));
  text_layer_set_background_color(dewpoint_layer, GColorBlack);
  text_layer_set_text_color(dewpoint_layer, GColorClear);
  text_layer_set_text_alignment(dewpoint_layer, GTextAlignmentCenter);
  dewpoint_font = fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT);
  text_layer_set_font(dewpoint_layer, dewpoint_font);
  layer_add_child(window_layer, text_layer_get_layer(dewpoint_layer));
  update_time();
}

static void window_unload(Window *window) {
  text_layer_destroy(dewpoint_layer);
  text_layer_destroy(temperature_layer);
  gbitmap_destroy(background_bitmap);
  bitmap_layer_destroy(background_layer);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  static char temperature_buffer[8];
  static char dewpoint_buffer[8];
  static char temperature_layer_buffer[8];
  static char dewpoint_layer_buffer[8];
  Tuple *t = dict_read_first(iterator);
  while (t != NULL) {
    switch (t->key) {
      case KEY_TEMPERATURE:
        snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)t->value->int32);
        break;
      case KEY_DEWPOINT:
        snprintf(dewpoint_buffer, sizeof(dewpoint_buffer), "%dC", (int)t->value->int32);
        break;
      default:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognised!", (int)t->key);
        break;
    }
    t = dict_read_next(iterator);
  }
  // construct full string, display string
  snprintf(temperature_layer_buffer, sizeof(temperature_layer_buffer), "%s", temperature_buffer);
  text_layer_set_text(temperature_layer, temperature_layer_buffer);
  snprintf(dewpoint_layer_buffer, sizeof(dewpoint_layer_buffer), "%s", dewpoint_buffer);
  text_layer_set_text(dewpoint_layer, dewpoint_layer_buffer);
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