#include <pebble.h>

// Layers
static Window *s_main_window;
static Layer *s_canvas_layer;

// Paths for the hands
static GPath *s_hour_arrow, *s_minute_arrow;

// Path definitions (triangular hands)
static const GPathInfo HOUR_HAND_PATH_INFO = {
  .num_points = 4,
  .points = (GPoint[]) {{-4, 0}, {4, 0}, {0, -30}, {-4, 0}}
};
static const GPathInfo MINUTE_HAND_PATH_INFO = {
  .num_points = 4,
  .points = (GPoint[]) {{-3, 0}, {3, 0}, {0, -50}, {-3, 0}}
};

// Update the watch hands
static void update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  
  // Set the colors
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  
  // Draw the watch face background
  graphics_fill_circle(ctx, center, 65);
  graphics_draw_circle(ctx, center, 65);
  
  // Draw the hour markers
  for (int i = 0; i < 12; i++) {
    int32_t angle = TRIG_MAX_ANGLE * i / 12;
    
    int marker_length = (i % 3 == 0) ? 10 : 5; // Longer markers for 12, 3, 6, 9
    GPoint start_point = {
      .x = (int16_t)(center.x + (65 - marker_length) * sin_lookup(angle) / TRIG_MAX_RATIO),
      .y = (int16_t)(center.y - (65 - marker_length) * cos_lookup(angle) / TRIG_MAX_RATIO)
    };
    GPoint end_point = {
      .x = (int16_t)(center.x + 65 * sin_lookup(angle) / TRIG_MAX_RATIO),
      .y = (int16_t)(center.y - 65 * cos_lookup(angle) / TRIG_MAX_RATIO)
    };
    
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_line(ctx, start_point, end_point);
  }
  
  // Get the current time
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  
  // Calculate hand angles
  int32_t hour_angle = (TRIG_MAX_ANGLE * ((t->tm_hour % 12) * 6 + (t->tm_min / 10))) / (12 * 6);
  int32_t minute_angle = TRIG_MAX_ANGLE * t->tm_min / 60;
  
  // Draw hour hand
  gpath_rotate_to(s_hour_arrow, hour_angle);
  gpath_move_to(s_hour_arrow, center);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  gpath_draw_filled(ctx, s_hour_arrow);
  gpath_draw_outline(ctx, s_hour_arrow);
  
  // Draw minute hand
  gpath_rotate_to(s_minute_arrow, minute_angle);
  gpath_move_to(s_minute_arrow, center);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  gpath_draw_filled(ctx, s_minute_arrow);
  gpath_draw_outline(ctx, s_minute_arrow);
  
  // Draw second hand (simple line)
  if (t->tm_sec != 0) {
    int32_t second_angle = TRIG_MAX_ANGLE * t->tm_sec / 60;
    GPoint second_hand = {
      .x = (int16_t)(center.x + 60 * sin_lookup(second_angle) / TRIG_MAX_RATIO),
      .y = (int16_t)(center.y - 60 * cos_lookup(second_angle) / TRIG_MAX_RATIO)
    };
    
    graphics_context_set_stroke_color(ctx, GColorRed);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_line(ctx, center, second_hand);
  }
  
  // Draw the center circle
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, 4);
}

// Handler called every minute
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_canvas_layer);
}

// Initialize the main window
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Create the canvas layer
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, update_proc);
  layer_add_child(window_layer, s_canvas_layer);
  
  // Create hand paths
  s_hour_arrow = gpath_create(&HOUR_HAND_PATH_INFO);
  s_minute_arrow = gpath_create(&MINUTE_HAND_PATH_INFO);
}

// Clean up the main window
static void main_window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
  gpath_destroy(s_hour_arrow);
  gpath_destroy(s_minute_arrow);
}

// Initialize the app
static void init() {
  // Create the main window
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

// Clean up the app
static void deinit() {
  window_destroy(s_main_window);
  tick_timer_service_unsubscribe();
}

// Main function
int main(void) {
  init();
  app_event_loop();
  deinit();
}