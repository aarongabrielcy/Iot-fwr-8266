#include "adc_drv.h"
#include "board_pins.h"

#include "driver/adc.h"
#include "esp_log.h"
#include "esp_err.h"

#define TAG "ADC_DRV"

static adc_config_t adc_config;
static bool unit_initialized = false;

esp_err_t adc_drv_init();

esp_err_t adc_drv_init(){
    if(!unit_initialized){
        adc_config.mode = ADC_READ_TOUT_MODE; // Leer el pin externo
        adc_config.clk_div = 8;               // Divisor de reloj estándar
    
        adc_init(&adc_config);
        unit_initialized = true;
    }
    return ESP_OK;
}

int read_adc_data(){
    uint16_t raw_adc = 0;
    if (adc_read(&raw_adc) == ESP_OK) {
        // Manipular el raw data
        return raw_adc;
    }
    return -1;
}