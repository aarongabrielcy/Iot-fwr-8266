#include "ota_fw.h"
#include "esp_err.h"
//#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
//#include "esp_crt_bundle.h"

#define TAG "OTA_MANAGER"

//https://gruposisprovisa.mx/fw/1.0.1/fsdfsd.bin

esp_err_t ota_update(const char *url){
    /*ESP_LOGI(TAG, "Iniciando actualización OTA...");
    esp_http_client_config_t config = {
        .url = url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 15000,
    };
    esp_https_ota_config_t ota_config = {
        .http_config=&config,
    };
    esp_err_t err = esp_https_ota(&ota_config);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "OTA exitosa. Reiniciando...");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "Error en la actualización OTA: %d", err);
        return ESP_FAIL;
    }*/
    return ESP_OK;
}

