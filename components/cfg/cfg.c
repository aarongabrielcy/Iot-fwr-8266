#include "cfg.h"

#include <string.h>
#include <stdio.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "CFG";
static const char *NVS_NS  = "app";
static const char *NVS_KEY = "cfg_blob";

static app_cfg_t s_cfg_cache;
static bool s_cfg_cache_valid = false;

static void cfg_set_defaults(app_cfg_t *cfg);
static void cfg_sync_cache(const app_cfg_t *cfg);

bool cfg_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS issue, erasing NVS partition...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init failed: %s", esp_err_to_name(err));
        return false;
    }

    return true;
}

static void cfg_sync_cache(const app_cfg_t *cfg)
{
    if (!cfg) {
        return;
    }

    s_cfg_cache = *cfg;
    s_cfg_cache_valid = true;
}

static void cfg_set_defaults(app_cfg_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));

    /* Identidad */
    snprintf(cfg->device_id, sizeof(cfg->device_id), "FCB46773B338");

    /* WiFi STA */
    snprintf(cfg->wifi_ssid, sizeof(cfg->wifi_ssid), "INFINITUM471D");
    snprintf(cfg->wifi_pass, sizeof(cfg->wifi_pass), "12345678");

    /* MQTT */
    snprintf(cfg->mqtt_host, sizeof(cfg->mqtt_host), "192.168.1.10");
    cfg->mqtt_port = 1883;
    snprintf(cfg->mqtt_user, sizeof(cfg->mqtt_user), "demo");
    snprintf(cfg->mqtt_pass, sizeof(cfg->mqtt_pass), "1234");

    /* DHCP */
    snprintf(cfg->ip_dhcp, sizeof(cfg->ip_dhcp), "0.0.0.0");

    /* Web login */
    snprintf(cfg->web_user, sizeof(cfg->web_user), "%s", cfg->device_id);
    snprintf(cfg->web_pass, sizeof(cfg->web_pass), "12345678");

    /* MAC */
    snprintf(cfg->wifi_mac, sizeof(cfg->wifi_mac), "FCB46773B338");

    /* Sensores / límites */
    cfg->temperature_cfg.max_temp = 50;
    cfg->temperature_cfg.min_temp = 0;
    cfg->temperature_cfg.max_hum  = 80;
    cfg->temperature_cfg.min_hum  = 20;

    cfg->sensor_selector = ULTRASONIC_SENSOR;
    cfg->telemetry_interval_s = 30;

    snprintf(cfg->cfg_version, sizeof(cfg->cfg_version), "%s", CFG_BLOB_VERSION);

    /* Tiempo / odómetro */
    cfg->datetime = 0;
    cfg->orometer_ = 0;
}

bool cfg_load(app_cfg_t *out)
{
    if (!out) {
        return false;
    }

    memset(out, 0, sizeof(*out));

    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_open(read) failed: %s", esp_err_to_name(err));
        return false;
    }

    size_t required = 0;
    err = nvs_get_blob(nvs, NVS_KEY, NULL, &required);

    if (err != ESP_OK || required != sizeof(app_cfg_t)) {
        ESP_LOGW(TAG,
                 "No cfg blob or size mismatch (err=%s size=%u expected=%u). Writing defaults.",
                 esp_err_to_name(err),
                 (unsigned)required,
                 (unsigned)sizeof(app_cfg_t));

        cfg_set_defaults(out);

        err = nvs_set_blob(nvs, NVS_KEY, out, sizeof(*out));
        if (err == ESP_OK) {
            err = nvs_commit(nvs);
        }

        nvs_close(nvs);

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to write default cfg blob: %s", esp_err_to_name(err));
            return false;
        }

        cfg_sync_cache(out);
        return true;
    }

    err = nvs_get_blob(nvs, NVS_KEY, out, &required);
    nvs_close(nvs);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "nvs_get_blob failed: %s", esp_err_to_name(err));
        return false;
    }

    /* versioning mínimo */
    if (strstr(out->cfg_version, CFG_BLOB_VERSION) == NULL) {
        strlcpy(out->cfg_version, CFG_BLOB_VERSION, sizeof(out->cfg_version));
    }

    cfg_sync_cache(out);
    return true;
}

bool cfg_save(const app_cfg_t *cfg)
{
    if (!cfg) {
        return false;
    }

    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open(write) failed: %s", esp_err_to_name(err));
        return false;
    }

    err = nvs_set_blob(nvs, NVS_KEY, cfg, sizeof(*cfg));
    if (err == ESP_OK) {
        err = nvs_commit(nvs);
    }

    nvs_close(nvs);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "cfg_save failed: %s", esp_err_to_name(err));
        return false;
    }

    cfg_sync_cache(cfg);
    return true;
}

void cfg_ensure_device_id(app_cfg_t *cfg)
{
    if (!cfg) return;
    if (cfg->device_id[0] != '\0') return;

    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    snprintf(cfg->device_id,
             sizeof(cfg->device_id),
             "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void cfg_ensure_web_credentials(app_cfg_t *cfg)
{
    if (!cfg) return;

    if (cfg->web_user[0] == '\0') {
        cfg_ensure_device_id(cfg);
        strlcpy(cfg->web_user, cfg->device_id, sizeof(cfg->web_user));
    }

    if (cfg->web_pass[0] == '\0') {
        strlcpy(cfg->web_pass, "12345678", sizeof(cfg->web_pass));
    }
}

void cfg_ensure_telemetry_interval(app_cfg_t *cfg)
{
    if (!cfg) return;

    if (cfg->telemetry_interval_s == 0) {
        cfg->telemetry_interval_s = 10;
    }
}

bool cfg_has_wifi_sta(const app_cfg_t *cfg)
{
    return (cfg && cfg->wifi_ssid[0] != '\0');
}

bool cfg_has_mqtt(const app_cfg_t *cfg)
{
    return (cfg && cfg->mqtt_host[0] != '\0' && cfg->mqtt_port > 0);
}

bool cfg_set_ip_dhcp_and_save(app_cfg_t *cfg, const char *ip_str)
{
    if (!cfg || !ip_str || ip_str[0] == '\0') {
        return false;
    }

    if (strncmp(cfg->ip_dhcp, ip_str, sizeof(cfg->ip_dhcp)) == 0) {
        return true;
    }

    strlcpy(cfg->ip_dhcp, ip_str, sizeof(cfg->ip_dhcp));
    return cfg_save(cfg);
}

bool cfg_set_bt_and_wifi_mac(app_cfg_t *cfg)
{
    if (!cfg) {
        return false;
    }

    uint8_t mac_wifi[6] = {0};
    esp_read_mac(mac_wifi, ESP_MAC_WIFI_STA);

    snprintf(cfg->wifi_mac,
             sizeof(cfg->wifi_mac),
             "%02X:%02X:%02X:%02X:%02X:%02X",
             mac_wifi[0], mac_wifi[1], mac_wifi[2],
             mac_wifi[3], mac_wifi[4], mac_wifi[5]);

    return true;
}

void cfg_print_boot(const app_cfg_t *cfg, bool show_password)
{
    if (!cfg) return;

    ESP_LOGI(TAG, "===== NVS CONFIG (boot) =====");
    ESP_LOGI(TAG, "device_id: %s", cfg->device_id[0] ? cfg->device_id : "(empty)");

    ESP_LOGI(TAG, "wifi_ssid: %s", cfg->wifi_ssid[0] ? cfg->wifi_ssid : "(empty)");
    if (show_password) {
        ESP_LOGI(TAG, "wifi_pass: %s", cfg->wifi_pass[0] ? cfg->wifi_pass : "(empty)");
    } else {
        ESP_LOGI(TAG, "wifi_pass: %s", cfg->wifi_pass[0] ? "********" : "(empty)");
    }

    ESP_LOGI(TAG, "ip_dhcp(last): %s", cfg->ip_dhcp[0] ? cfg->ip_dhcp : "(empty)");

    ESP_LOGI(TAG, "mqtt_host: %s", cfg->mqtt_host[0] ? cfg->mqtt_host : "(empty)");
    ESP_LOGI(TAG, "mqtt_port: %d", cfg->mqtt_port);
    ESP_LOGI(TAG, "mqtt_user: %s", cfg->mqtt_user[0] ? cfg->mqtt_user : "(empty)");

    ESP_LOGI(TAG, "web_user: %s", cfg->web_user[0] ? cfg->web_user : "(empty)");
    ESP_LOGI(TAG, "web_pass: %s", cfg->web_pass[0] ? cfg->web_pass : "(empty)");

    ESP_LOGI(TAG, "sensor_selected: %d", cfg->sensor_selector);
    ESP_LOGI(TAG, "temp_range: %d - %d",
             cfg->temperature_cfg.min_temp,
             cfg->temperature_cfg.max_temp);
    ESP_LOGI(TAG, "hum_range: %d - %d",
             cfg->temperature_cfg.min_hum,
             cfg->temperature_cfg.max_hum);
    ESP_LOGI(TAG, "telemetry_interval_s: %u", (unsigned)cfg->telemetry_interval_s);
    ESP_LOGI(TAG, "cfg_version: %s", cfg->cfg_version[0] ? cfg->cfg_version : "(empty)");
    ESP_LOGI(TAG, "wifi_mac: %s", cfg->wifi_mac[0] ? cfg->wifi_mac : "(empty)");
    ESP_LOGI(TAG, "=============================");
}

/* =========================
 * Cache / getters nuevos
 * ========================= */

const app_cfg_t *cfg_get_active(void)
{
    return s_cfg_cache_valid ? &s_cfg_cache : NULL;
}

bool cfg_get_active_copy(app_cfg_t *out)
{
    if (!out || !s_cfg_cache_valid) {
        return false;
    }

    *out = s_cfg_cache;
    return true;
}

bool cfg_set_active(const app_cfg_t *cfg)
{
    if (!cfg) {
        return false;
    }

    cfg_sync_cache(cfg);
    return true;
}

bool cfg_save_active(void)
{
    if (!s_cfg_cache_valid) {
        return false;
    }

    return cfg_save(&s_cfg_cache);
}

const char *cfg_get_device_id(void)
{
    return s_cfg_cache_valid ? s_cfg_cache.device_id : NULL;
}

const char *cfg_get_wifi_ssid(void)
{
    return s_cfg_cache_valid ? s_cfg_cache.wifi_ssid : NULL;
}

const char *cfg_get_wifi_pass(void)
{
    return s_cfg_cache_valid ? s_cfg_cache.wifi_pass : NULL;
}

const char *cfg_get_mqtt_host(void)
{
    return s_cfg_cache_valid ? s_cfg_cache.mqtt_host : NULL;
}

int cfg_get_mqtt_port(void)
{
    return s_cfg_cache_valid ? s_cfg_cache.mqtt_port : 0;
}

const char *cfg_get_mqtt_user(void)
{
    return s_cfg_cache_valid ? s_cfg_cache.mqtt_user : NULL;
}

const char *cfg_get_mqtt_pass(void)
{
    return s_cfg_cache_valid ? s_cfg_cache.mqtt_pass : NULL;
}

const char *cfg_get_ip_dhcp(void)
{
    return s_cfg_cache_valid ? s_cfg_cache.ip_dhcp : NULL;
}

const char *cfg_get_web_user(void)
{
    return s_cfg_cache_valid ? s_cfg_cache.web_user : NULL;
}

const char *cfg_get_web_pass(void)
{
    return s_cfg_cache_valid ? s_cfg_cache.web_pass : NULL;
}

const char *cfg_get_wifi_mac(void)
{
    return s_cfg_cache_valid ? s_cfg_cache.wifi_mac : NULL;
}

uint32_t cfg_get_telemetry_interval_s(void)
{
    return s_cfg_cache_valid ? s_cfg_cache.telemetry_interval_s : 0;
}

sensor_selector_t cfg_get_sensor_selector(void)
{
    return s_cfg_cache_valid ? s_cfg_cache.sensor_selector : ULTRASONIC_SENSOR;
}