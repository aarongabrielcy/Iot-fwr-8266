#include "app_event_loop.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "APP_EVENT_LOOP";
static esp_event_loop_handle_t s_app_event_loop = NULL;

void app_event_loop_init(void) {
    if (s_app_event_loop != NULL) {
        ESP_LOGW(TAG, "App event loop already initialized");
        return;
    }

    esp_event_loop_args_t loop_args = {
        .queue_size = 16,
        .task_name = "app_evt_loop",
        .task_priority = 5,
        .task_stack_size = 4096,
        .task_core_id = 0
    };

    esp_err_t err = esp_event_loop_create(&loop_args, &s_app_event_loop);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create app event loop: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "App event loop initialized");
}

esp_event_loop_handle_t app_event_loop_get(void) {
    return s_app_event_loop;
}