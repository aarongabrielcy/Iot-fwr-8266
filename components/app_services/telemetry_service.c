#include "telemetry_service.h"
#include "app_state.h"
#include "app_events_ids.h"
#include "mqtt_mgr.h"
#include "telemetry_builder.h"
#include "esp_log.h"

static const char *TAG = "TELEMETRY_SERVICE";

static bool telemetry_build_payload(int32_t report_event_id,
                                    void *event_data,
                                    char *payload,
                                    size_t payload_len)
{
    (void)event_data;

    if (payload == NULL || payload_len == 0) {
        return false;
    }

    payload[0] = '\0';

    switch (report_event_id) {
        case APP_RPT_TRACKING:
            return telemetry_builder_build_tracking(payload, payload_len);

        case APP_RPT_BOOT:
            return telemetry_builder_build_boot(payload, payload_len);

        case APP_RPT_OUTPUT_STATUS:
            return telemetry_builder_build_output_status(payload, payload_len);

        case APP_RPT_INPUT_STATUS:
            return telemetry_builder_build_input_status(payload, payload_len);

        case APP_RPT_SENSOR_STATUS:
            return telemetry_builder_build_sensor_status(payload, payload_len);

        case APP_RPT_ALERT:
            return telemetry_builder_build_alert(payload, payload_len);

        case APP_RPT_CT_ALERT:
            return telemetry_builder_build_ct_alert(payload, payload_len);

        default:
            ESP_LOGW(TAG, "Unsupported report event for telemetry: %ld", (long)report_event_id);
            return false;
    }
}

bool telemetry_service_publish_report(int32_t report_event_id, void *event_data)
{
    char payload[512];

    if (!app_state_get_wifi_connected()) {
        ESP_LOGW(TAG, "Skipping publish: WiFi disconnected");
        return false;
    }

    if (!app_state_get_mqtt_connected()) {
        ESP_LOGW(TAG, "Skipping publish: MQTT disconnected");
        return false;
    }

    if (!telemetry_build_payload(report_event_id, event_data, payload, sizeof(payload))) {
        ESP_LOGW(TAG, "Failed to build telemetry payload for event %ld", (long)report_event_id);
        return false;
    }

    if (payload[0] == '\0') {
        ESP_LOGW(TAG, "Skipping publish: empty payload");
        return false;
    }

    if (!mqtt_mgr_publish_telemetry(payload)) {
        ESP_LOGW(TAG, "MQTT publish failed for event %ld", (long)report_event_id);
        return false;
    }

    ESP_LOGI(TAG, "Telemetry published for event %ld: %s", (long)report_event_id, payload);
    return true;
}