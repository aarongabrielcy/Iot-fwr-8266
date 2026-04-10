#include "io_drv.h"
#include "board_pins.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

#define TAG "IO_DRV"

#define IO_DRV_OUTPUT_COUNT 1

static bool s_io_initialized = false;
static bool s_output_1_state = false;

static bool io_drv_valid_output(uint8_t output_id)
{
    return (output_id == 1U);
}

esp_err_t io_drv_init(void)
{
    if (s_io_initialized) {
        ESP_LOGW(TAG, "io_drv already initialized");
        return ESP_OK;
    }

    gpio_config_t output_gpio = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT_OD,
        .pin_bit_mask = (1ULL << OUTPUT_1),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE
    };

    esp_err_t err = gpio_config(&output_gpio);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "IO DRV CONFIG OUTPUT PIN FAILED: %s", esp_err_to_name(err));
        return err;
    }

    /*
     * Tu hardware actual parece activo en bajo:
     * en tu código viejo hacías gpio_set_level(OUTPUT_1, !state)
     *
     * Entonces:
     * value=false  => pin en alto  => salida desactivada
     * value=true   => pin en bajo  => salida activada
     */
    err = gpio_set_level(OUTPUT_1, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set initial output level: %s", esp_err_to_name(err));
        return err;
    }

    s_output_1_state = false;
    s_io_initialized = true;

    ESP_LOGI(TAG, "IO driver initialized");
    return ESP_OK;
}

esp_err_t io_drv_set_output(uint8_t output_id, bool value)
{
    if (!s_io_initialized) {
        ESP_LOGE(TAG, "io_drv_set_output called before init");
        return ESP_ERR_INVALID_STATE;
    }

    if (!io_drv_valid_output(output_id)) {
        ESP_LOGW(TAG, "Invalid output_id=%u", (unsigned)output_id);
        return ESP_ERR_INVALID_ARG;
    }

    int physical_level = value ? 0 : 1;

    esp_err_t err = gpio_set_level(OUTPUT_1, !physical_level);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "gpio_set_level failed: %s", esp_err_to_name(err));
        return err;
    }

    s_output_1_state = value;

    ESP_LOGI(TAG,
             "Output %u -> logical=%d physical_level=%d",
             (unsigned)output_id,
             (int)value,
             physical_level);

    return ESP_OK;
}

esp_err_t io_drv_get_output(uint8_t output_id, bool *value)
{
    if (!s_io_initialized) {
        ESP_LOGE(TAG, "io_drv_get_output called before init");
        return ESP_ERR_INVALID_STATE;
    }

    if (value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!io_drv_valid_output(output_id)) {
        ESP_LOGW(TAG, "Invalid output_id=%u", (unsigned)output_id);
        return ESP_ERR_INVALID_ARG;
    }

    *value = s_output_1_state;
    return ESP_OK;
}

esp_err_t io_drv_get_output_level(uint8_t output_id, int *level)
{
    if (!s_io_initialized) {
        ESP_LOGE(TAG, "io_drv_get_output_level called before init");
        return ESP_ERR_INVALID_STATE;
    }

    if (level == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!io_drv_valid_output(output_id)) {
        ESP_LOGW(TAG, "Invalid output_id=%u", (unsigned)output_id);
        return ESP_ERR_INVALID_ARG;
    }

    *level = gpio_get_level(OUTPUT_1);
    return ESP_OK;
}