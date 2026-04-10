#include "adc_drv.h"

#include "driver/adc.h"
#include "esp_log.h"
#include "esp_err.h"

#define TAG "ADC_DRV"

static bool s_adc_initialized = false;

esp_err_t adc_drv_init(void)
{
    if (s_adc_initialized) {
        return ESP_OK;
    }

    adc_config_t adc_config = {
        .mode = ADC_READ_TOUT_MODE,
        .clk_div = 8
    };

    adc_init(&adc_config);
    s_adc_initialized = true;

    ESP_LOGI(TAG, "ADC driver initialized in TOUT mode");
    return ESP_OK;
}

esp_err_t adc_drv_read_raw(uint16_t *raw)
{
    if (raw == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_adc_initialized) {
        ESP_LOGE(TAG, "adc_drv_read_raw called before init");
        return ESP_ERR_INVALID_STATE;
    }

    uint16_t adc_value = 0;
    esp_err_t err = adc_read(&adc_value);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "adc_read failed: %s", esp_err_to_name(err));
        return err;
    }

    *raw = adc_value;
    return ESP_OK;
}

esp_err_t adc_drv_read_avg_raw(uint16_t *raw_avg, uint16_t samples)
{
    if (raw_avg == NULL || samples == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_adc_initialized) {
        ESP_LOGE(TAG, "adc_drv_read_avg_raw called before init");
        return ESP_ERR_INVALID_STATE;
    }

    uint32_t acc = 0;

    for (uint16_t i = 0; i < samples; i++) {
        uint16_t raw = 0;
        esp_err_t err = adc_drv_read_raw(&raw);
        if (err != ESP_OK) {
            return err;
        }
        acc += raw;
    }

    *raw_avg = (uint16_t)(acc / samples);
    return ESP_OK;
}