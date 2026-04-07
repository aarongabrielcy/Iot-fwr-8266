#include "i2c_drv.h"
#include "board_pins.h"

#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"

#define TAG "I2C_DRV"
static bool i2c_driver_installed = false;

esp_err_t i2c_drv_init(i2c_drv_cfg_t *cfg);

esp_err_t i2c_drv_init(i2c_drv_cfg_t *cfg) {
    esp_err_t err;

    if (!i2c_driver_installed) {
        i2c_config_t conf = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = I2C_MASTER_SDA_PIN,
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_io_num = I2C_MASTER_SCL_PIN,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
            .clk_stretch_tick = 300, // Valor recomendado para ESP8266
        };

        err = i2c_param_config(I2C_NUM_0, &conf);
        if (err != ESP_OK) return err;

        err = i2c_driver_install(I2C_NUM_0, conf.mode);
        if (err != ESP_OK) {
            ESP_LOGI(TAG, "MASTER BUS INIT FAILED: %s", esp_err_to_name(err));
            return ESP_FAIL;
        }
        i2c_driver_installed = true;
    }

    // El ESP8266 no usa 'cfg->handle' para dispositivos individuales. 
    // La dirección se pasa en cada transacción.
    
    // Simulación de "Probe" para mantener tu lógica de detección
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (cfg->device_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    err = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "¡Sensor encontrado en 0x%02x!", cfg->device_address);
    } else {
        ESP_LOGE(TAG, "Sensor NO encontrado en 0x%02x. Error: %s", cfg->device_address, esp_err_to_name(err));
    }

    return ESP_OK;
}

bool send_data_i2c(const uint8_t *data, size_t len, i2c_drv_cfg_t *cfg){
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (cfg->device_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, (uint8_t*)data, len, true);
    i2c_master_stop(cmd);
    
    esp_err_t err = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    if (err != ESP_OK) {
        ESP_LOGI(TAG, "I2C SEND DATA FAILED: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool read_data_i2c(uint8_t *data, size_t len, i2c_drv_cfg_t *cfg){
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (cfg->device_address << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    esp_err_t err = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    if (err != ESP_OK) {
        ESP_LOGI(TAG, "I2C READ DATA FAILED: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}