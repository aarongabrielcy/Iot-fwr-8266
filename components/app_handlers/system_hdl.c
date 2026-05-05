#include "system_hdl.h"
#include "app_event_loop.h"
#include "app_events_ids.h"
#include "system_service.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "cfg.h"

static const char *TAG = "SYSTEM_HDL";

static void system_event_handler(void *handler_arg,
                                 esp_event_base_t event_base,
                                 int32_t event_id,
                                 void *event_data) {
    if (handler_arg == NULL) {
        ESP_LOGW(TAG, "handler_arg is NULL");
        return;
    }
    app_cfg_t *cfg = (app_cfg_t *)handler_arg;
    (void)event_base;
    (void)event_data;

    system_service_handle(cfg, event_id, event_data);
}

void system_hdl_init(const app_cfg_t *cfg) {

    if (cfg == NULL) {
        ESP_LOGE(TAG, "cfg is NULL");
        return;
    }

    esp_err_t err = esp_event_handler_register_with(
        app_event_loop_get(),
        APP_SYSTEM_EVENTS,
        ESP_EVENT_ANY_ID,
        system_event_handler,
        (void *)cfg
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register system handler: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "System handler initialized");
}