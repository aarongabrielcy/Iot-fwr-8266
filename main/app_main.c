#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "app_boot.h"

#define TAG "APP_MAIN"

void app_main(void)
{
    esp_err_t err = app_boot_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Initialization failed, rebooting...");
        vTaskDelay(pdMS_TO_TICKS(2000));
        esp_restart();
    }

    ESP_LOGI(TAG, "Initialization done");
}