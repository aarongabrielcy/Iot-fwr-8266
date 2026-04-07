#include "report_hdl.h"
#include "app_event_loop.h"
#include "app_events_ids.h"
#include "report_service.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "REPORT_HDL";

static void report_event_handler(void *handler_arg,
                                 esp_event_base_t event_base,
                                 int32_t event_id,
                                 void *event_data) {
    (void)handler_arg;
    (void)event_base;

    report_service_handle(event_id, event_data);
}

void report_hdl_init(void) {
    esp_err_t err = esp_event_handler_register_with(
        app_event_loop_get(),
        APP_REPORT_EVENTS,
        ESP_EVENT_ANY_ID,
        report_event_handler,
        NULL
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register report handler: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Report handler initialized");
}