#include "app_runtime.h"

#include "wifi_mgr.h"
#include "cfg.h"
#include "mqtt_mgr.h"
#include "web_cfg.h"
#include "app_state.h"
#include "scheduler.h"

#include "esp_log.h"
#include "string.h"
#include <stdio.h>

#define TAG "APP_RUNTIME"

static app_cfg_t g_cfg;

static void on_wifi_connected(void);
static void on_wifi_failed(void);
static void on_wifi_got_ip(const char *ip_str);
static void start_web(void);

void app_runtime_start(const app_cfg_t *cfg)
{
    if (cfg == NULL) {
        ESP_LOGE(TAG, "app_runtime_start called with NULL cfg");
        return;
    }

    g_cfg = *cfg;

    wifi_mgr_set_on_connected(on_wifi_connected);
    wifi_mgr_set_on_failed(on_wifi_failed);
    wifi_mgr_set_on_got_ip(on_wifi_got_ip);

    if (g_cfg.wifi_ssid[0] == '\0') {
        char ap_ssid[32];
        snprintf(ap_ssid, sizeof(ap_ssid), "esp-%.8s", g_cfg.device_id);

        ESP_LOGW(TAG,
                 "No config in NVS. Starting AP config mode: ssid=%s pass=%s",
                 ap_ssid,
                 g_cfg.web_pass);

        wifi_mgr_start_apsta(ap_ssid, g_cfg.web_pass);
        start_web();

        ESP_LOGI(TAG, "Web UI (AP) -> http://192.168.4.1/");

        app_state_set_mode(APP_MODE_APSTA);
        app_state_set_config_mode(true);
        return;
    }

    ESP_LOGI(TAG, "Starting STA with saved WiFi SSID=%s", g_cfg.wifi_ssid);
    wifi_mgr_start_sta(g_cfg.wifi_ssid, g_cfg.wifi_pass);

    app_state_set_config_mode(false);
    app_state_set_mode(APP_MODE_STA);
}

static void on_wifi_connected(void)
{
    ESP_LOGI(TAG, "WiFi connected -> start MQTT + scheduler");

    if (g_cfg.mqtt_host[0] == '\0' || g_cfg.mqtt_port <= 0) {
        ESP_LOGW(TAG, "MQTT not configured yet (host/port empty). Skipping mqtt start.");
    } else {
        if (!mqtt_mgr_start()) {
            ESP_LOGW(TAG, "mqtt_mgr_start failed");
        }
    }

    if (!scheduler_start()) {
        ESP_LOGW(TAG, "scheduler_start failed");
    } else {
        app_state_set_scheduler_started(true);
    }
}

static void on_wifi_failed(void)
{
    ESP_LOGW(TAG, "WiFi failed after max retries -> switching to AP config mode");

    char ap_ssid[32];
    snprintf(ap_ssid, sizeof(ap_ssid), "esp-%.8s", g_cfg.device_id);

    wifi_mgr_start_apsta(ap_ssid, g_cfg.web_pass);
    web_cfg_start(&g_cfg);

    ESP_LOGW(TAG,
             "AP Config ready -> connect to SSID '%s' and open http://192.168.4.1/",
             ap_ssid);

    post_system_event(WIFI_RECONNECTION_CFG);
    app_state_set_config_mode(true);
    app_state_set_mode(APP_MODE_CONFIG);
}

static void on_wifi_got_ip(const char *ip_str)
{
    if (!ip_str) {
        return;
    }

    ESP_LOGI(TAG, "DHCP IP assigned: %s -> saving to NVS", ip_str);

    strlcpy(g_cfg.ip_dhcp, ip_str, sizeof(g_cfg.ip_dhcp));
    cfg_set_active(&g_cfg);
    cfg_save(&g_cfg);

    start_web();

    ESP_LOGI(TAG, "Web UI (STA) -> http://%s/", ip_str);

    app_state_set_ip_addr(ip_str);
    app_state_set_wifi_connected(true);
    post_system_event(WIFI_RECONNECTION_STOP);
}

static void start_web(void)
{
    char host[32];
    snprintf(host, sizeof(host), "esp-%.8s", g_cfg.device_id);
    (void)host;

    web_cfg_start(&g_cfg);
}