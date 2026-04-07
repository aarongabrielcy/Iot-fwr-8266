#include "system_service.h"
#include "app_events_ids.h"
#include "app_state.h"
#include "esp_log.h"

static const char *TAG = "SYSTEM_SERVICE";

void system_service_handle(int32_t event_id, void *event_data) {
    (void)event_data;

    switch (event_id) {
        case APP_SYS_WIFI_CONNECTED:
            app_state_set_wifi_connected(true);
            ESP_LOGI(TAG, "WiFi connected");
            break;

        case APP_SYS_WIFI_DISCONNECTED:
            app_state_set_wifi_connected(false);
            app_state_set_mqtt_connected(false);
            ESP_LOGI(TAG, "WiFi disconnected");
            break;

        case APP_SYS_MQTT_CONNECTED:
            app_state_set_mqtt_connected(true);
            ESP_LOGI(TAG, "MQTT connected");
            break;

        case APP_SYS_MQTT_DISCONNECTED:
            app_state_set_mqtt_connected(false);
            ESP_LOGI(TAG, "MQTT disconnected");
            break;

        case APP_SYS_TIME_SYNCED:
            app_state_set_time_synced(true);
            ESP_LOGI(TAG, "Time synced");
            break;

        default:
            ESP_LOGW(TAG, "Unhandled system event: %ld", (long)event_id);
            break;
    }
}