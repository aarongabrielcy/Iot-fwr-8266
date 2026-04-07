#include "report_service.h"
#include "telemetry_service.h"
#include "esp_log.h"

static const char *TAG = "REPORT_SERVICE";

void report_service_handle(int32_t event_id, void *event_data) {
    bool published = telemetry_service_publish_report(event_id, event_data);

    if (!published) {
        ESP_LOGW(TAG, "Report event %ld was not published", (long)event_id);
        return;
    }

    ESP_LOGI(TAG, "Report event %ld published", (long)event_id);
}