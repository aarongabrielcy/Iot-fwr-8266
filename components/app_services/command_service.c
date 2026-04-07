#include "command_service.h"
#include "app_events_ids.h"
#include "app_event_data.h"
#include "app_events.h"
#include "io_sensor.h"
#include "ota_fw.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "COMMAND_SERVICE";

void command_service_handle(int32_t event_id, void *event_data)
{
    switch (event_id) {
        case APP_CMD_SET_OUTPUT: {
            const app_cmd_output_t *cmd = (const app_cmd_output_t *)event_data;

            if (cmd == NULL) {
                ESP_LOGW(TAG, "APP_CMD_SET_OUTPUT received with NULL data");
                return;
            }

            ESP_LOGI(TAG, "Set output: output_id=%u value=%d",
                     (unsigned)cmd->output_id,
                     (int)cmd->value);

            if (!io_sensor_set_output(cmd->output_id, cmd->value)) {
                ESP_LOGW(TAG, "Failed to set output %u", (unsigned)cmd->output_id);
                return;
            }

            app_post_report_event(APP_RPT_OUTPUT_STATUS, NULL, 0);
            break;
        }

        case APP_CMD_REQUEST_REPORT:
            ESP_LOGI(TAG, "Request report command received");
            app_post_report_event(APP_RPT_TRACKING, NULL, 0);
            break;

        case APP_CMD_REBOOT:
            ESP_LOGI(TAG, "Reboot command received");
            esp_restart();
            break;

        case APP_CMD_START_OTA: {
            const app_cmd_ota_t *cmd = (const app_cmd_ota_t *)event_data;

            if (cmd == NULL || cmd->url[0] == '\0') {
                ESP_LOGW(TAG, "APP_CMD_START_OTA received with invalid URL");
                return;
            }

            ESP_LOGI(TAG, "Start OTA command received: %s", cmd->url);

            if (ota_update(cmd->url) != ESP_OK) {
                ESP_LOGW(TAG, "OTA update failed");
            }
            break;
        }

        default:
            ESP_LOGW(TAG, "Unhandled command event: %ld", (long)event_id);
            break;
    }
}