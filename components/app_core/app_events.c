#include "app_events.h"
#include "app_events_ids.h"
#include "app_event_loop.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "APP_EVENTS";

/* Define event bases */
ESP_EVENT_DEFINE_BASE(APP_SYSTEM_EVENTS);
ESP_EVENT_DEFINE_BASE(APP_REPORT_EVENTS);
ESP_EVENT_DEFINE_BASE(APP_SENSOR_EVENTS);
ESP_EVENT_DEFINE_BASE(APP_COMMAND_EVENTS);

static void app_post_event(esp_event_base_t base, int32_t event_id, const void *data, size_t len, const char *base_name) {
    esp_event_loop_handle_t loop = app_event_loop_get();
    if (loop == NULL) {
        ESP_LOGE(TAG, "Cannot post %s event %ld: app event loop is NULL", base_name, (long)event_id);
        return;
    }

    esp_err_t err = esp_event_post_to(loop, base, event_id, data, len, portMAX_DELAY);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed posting %s event %ld: %s", base_name, (long)event_id, esp_err_to_name(err));
    }
}

void app_post_system_event(int32_t event_id, const void *data, size_t len) {
    app_post_event(APP_SYSTEM_EVENTS, event_id, data, len, "SYSTEM");
}

void app_post_report_event(int32_t event_id, const void *data, size_t len) {
    app_post_event(APP_REPORT_EVENTS, event_id, data, len, "REPORT");
}

void app_post_sensor_event(int32_t event_id, const void *data, size_t len) {
    app_post_event(APP_SENSOR_EVENTS, event_id, data, len, "SENSOR");
}

void app_post_command_event(int32_t event_id, const void *data, size_t len) {
    app_post_event(APP_COMMAND_EVENTS, event_id, data, len, "COMMAND");
}