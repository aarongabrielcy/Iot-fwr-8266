#include "system_service.h"
#include "app_events_ids.h"
#include "app_state.h"
#include "esp_log.h"
#include "scheduler.h"
#include "cfg.h"
#include "wifi_mgr.h"
#include "cfg.h"

static const char *TAG = "SYSTEM_SERVICE";

void system_service_handle(const app_cfg_t *cfg, int32_t event_id, void *event_data) {
    (void)event_data;
    (void)cfg;

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

        case WIFI_RECONNECTION_CFG:
            ESP_LOGI(TAG, "WiFi reconnection configured");
            start_reconnection_timer();
            break;

        case WIFI_RECONNECTION_TRY:
            ESP_LOGI(TAG, "WiFi reconnection attempt");
            if (cfg_has_wifi_sta(cfg)) {
                wifi_mgr_try_sta_reconnect(cfg->wifi_ssid, cfg->wifi_pass);
            } else {
                ESP_LOGW(TAG, "WiFi reconnection skipped: no WiFi STA config");
            }
            break;

        case WIFI_RECONNECTION_STOP:
            stop_reconnection_timer();
            app_state_set_config_mode(false);
            app_state_set_mode(APP_MODE_STA);
    
            ESP_LOGI(TAG, "WiFi reconnection stopped");
            break;

        default:
            ESP_LOGW(TAG, "Unhandled system event: %ld", (long)event_id);
            break;
    }
}