#include "app_boot.h"

#include "cfg.h"
#include "app_init.h"
#include "app_handlers.h"
#include "io_sensor.h"
#include "mqtt_mgr.h"
#include "scheduler.h"
#include "serial_monitor.h"
#include "app_runtime.h"

#include "esp_log.h"
#include "esp_err.h"

static const char *TAG = "APP_BOOT";

static app_cfg_t g_cfg;

esp_err_t app_boot_start(void)
{
    /*
     * 1) Validación / arranque de NVS
     * 2) Carga de cfg
     *
     * Esto se conserva ANTES de cualquier otro proceso,
     * tal como pediste.
     */
    if (!cfg_init()) {
        ESP_LOGE(TAG, "Error en el inicio de NVS");
        return ESP_FAIL;
    }

    if (!cfg_load(&g_cfg)) {
        ESP_LOGE(TAG, "Error en la carga del blob");
        return ESP_FAIL;
    }

    if (!cfg_set_bt_and_wifi_mac(&g_cfg)) {
        ESP_LOGE(TAG, "Error setting WiFi/Bluetooth MAC");
        return ESP_FAIL;
    }

    cfg_ensure_device_id(&g_cfg);
    cfg_ensure_web_credentials(&g_cfg);
    cfg_ensure_telemetry_interval(&g_cfg);

    /*
     * Muy importante:
     * cfg_load() llena el cache interno, pero aquí ya mutamos g_cfg
     * con ensure_* y MAC. Entonces hay que resincar el cache activo.
     */
    if (!cfg_set_active(&g_cfg)) {
        ESP_LOGE(TAG, "Failed to sync active cfg cache");
        return ESP_FAIL;
    }

    cfg_print_boot(&g_cfg, true);

    /*
     * A partir de aquí ya se pueden inicializar procesos.
     */
    app_core_init();
    app_handlers_init();

    if (!io_sensor_init()) {
        ESP_LOGE(TAG, "io_sensor_init failed");
        return ESP_FAIL;
    }

    if (!mqtt_mgr_init()) {
        ESP_LOGE(TAG, "mqtt_mgr_init failed");
        return ESP_FAIL;
    }

    if (!scheduler_init()) {
        ESP_LOGE(TAG, "scheduler_init failed");
        return ESP_FAIL;
    }

    serial_monitor_init();

    app_runtime_start(&g_cfg);

    return ESP_OK;
}