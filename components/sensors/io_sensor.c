#include "io_sensor.h"
#include "io_drv.h"

#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "IO_SENSOR";

bool io_sensor_init(void)
{
    esp_err_t err = io_drv_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "IO DRV INIT FAILED: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "IO sensor initialized");
    return true;
}

bool io_sensor_set_output(uint8_t output_id, bool value)
{
    esp_err_t err = io_drv_set_output(output_id, value);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "io_drv_set_output failed: %s", esp_err_to_name(err));
        return false;
    }

    return true;
}

bool io_sensor_get_output(uint8_t output_id, bool *value)
{
    esp_err_t err = io_drv_get_output(output_id, value);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "io_drv_get_output failed: %s", esp_err_to_name(err));
        return false;
    }

    return true;
}