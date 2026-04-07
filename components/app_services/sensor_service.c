#include "sensor_service.h"
#include "app_events_ids.h"
#include "esp_log.h"

static const char *TAG = "SENSOR_SERVICE";

void sensor_service_handle(int32_t event_id, void *event_data)
{
    (void)event_data;

    switch (event_id) {
        case APP_SENSOR_RESERVED:
            /*
             * Por ahora no hay inputs ni polling real.
             * Este evento queda reservado por si luego quieres
             * expandir IO.
             */
            ESP_LOGI(TAG, "APP_SENSOR_READ_IO received (no-op)");
            break;

        default:
            ESP_LOGW(TAG, "Unhandled sensor event: %ld", (long)event_id);
            break;
    }
}