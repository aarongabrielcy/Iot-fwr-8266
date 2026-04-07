#include "wifi_mgr.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "tcpip_adapter.h"
#include "lwip/inet.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_err.h"

#include "app_events.h"
#include "app_events_ids.h"

#include <string.h>
#include <stdint.h>
#include <stdio.h>

static const char *TAG = "WIFI_MGR";

static wifi_mgr_cb_t s_on_connected = NULL;
static wifi_mgr_cb_t s_on_failed = NULL;
static wifi_mgr_got_ip_cb_t s_on_got_ip = NULL;

static EventGroupHandle_t s_wifi_ev = NULL;

static bool s_sta_active = false;
static bool s_ap_active = false;
static bool s_sta_autoconnect = true;
static bool s_sta_connected = false;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry = 0;
static const int s_max_retry = 10;

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data);

void wifi_mgr_set_on_connected(wifi_mgr_cb_t cb) {
    s_on_connected = cb;
}

void wifi_mgr_set_on_failed(wifi_mgr_cb_t cb) {
    s_on_failed = cb;
}

void wifi_mgr_set_on_got_ip(wifi_mgr_got_ip_cb_t cb) {
    s_on_got_ip = cb;
}

bool wifi_mgr_is_sta_connected(void) {
    return s_sta_connected;
}

static void wifi_mgr_init_once(void) {
    static bool inited = false;
    if (inited) {
        return;
    }
    inited = true;

    tcpip_adapter_init();

    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_event_loop_create_default failed: %s", esp_err_to_name(err));
        return;
    }

    s_wifi_ev = xEventGroupCreate();
    if (s_wifi_ev == NULL) {
        ESP_LOGE(TAG, "Failed to create wifi event group");
        return;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
}

static void wifi_mgr_mark_disconnected(void) {
    s_sta_connected = false;
    if (s_wifi_ev) {
        xEventGroupClearBits(s_wifi_ev, WIFI_CONNECTED_BIT);
    }
    app_post_system_event(APP_SYS_WIFI_DISCONNECTED, NULL, 0);
}

static void wifi_mgr_mark_connected(void) {
    s_sta_connected = true;
    if (s_wifi_ev) {
        xEventGroupSetBits(s_wifi_ev, WIFI_CONNECTED_BIT);
    }
    app_post_system_event(APP_SYS_WIFI_CONNECTED, NULL, 0);
}

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data) {
    (void)arg;

    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        if (s_sta_autoconnect) {
            ESP_LOGI(TAG, "STA started, connecting...");
            esp_wifi_connect();
        }
        return;
    }

    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "STA disconnected");

        wifi_mgr_mark_disconnected();

        if (!s_sta_autoconnect) {
            return;
        }

        if (s_retry < s_max_retry) {
            s_retry++;
            ESP_LOGW(TAG, "Retrying STA connection %d/%d", s_retry, s_max_retry);
            esp_wifi_connect();
        } else {
            if (s_wifi_ev) {
                xEventGroupSetBits(s_wifi_ev, WIFI_FAIL_BIT);
            }
            if (s_on_failed) {
                s_on_failed();
            }
        }
        return;
    }

    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;
        s_retry = 0;

        char ip_str[16] = {0};
        snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&event->ip_info.ip));

        ESP_LOGI(TAG, "STA got IP: %s", ip_str);

        wifi_mgr_mark_connected();

        if (s_on_got_ip) {
            s_on_got_ip(ip_str);
        }

        if (s_on_connected) {
            s_on_connected();
        }
        return;
    }
}

bool wifi_mgr_start_sta(const char *ssid, const char *pass) {
    if (!ssid || ssid[0] == '\0') {
        ESP_LOGW(TAG, "wifi_mgr_start_sta called with empty SSID");
        return false;
    }

    wifi_mgr_init_once();

    s_sta_active = true;
    wifi_mode_t mode = s_ap_active ? WIFI_MODE_APSTA : WIFI_MODE_STA;

    ESP_ERROR_CHECK(esp_wifi_set_mode(mode));

    wifi_config_t wifi_cfg = {0};
    strlcpy((char *)wifi_cfg.sta.ssid, ssid, sizeof(wifi_cfg.sta.ssid));
    if (pass) {
        strlcpy((char *)wifi_cfg.sta.password, pass, sizeof(wifi_cfg.sta.password));
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));

    s_sta_autoconnect = true;
    s_sta_connected = false;

    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Starting STA: %s", ssid);
    return true;
}

bool wifi_mgr_start_ap(const char *ap_ssid, const char *ap_pass) {
    if (!ap_ssid || ap_ssid[0] == '\0') {
        ESP_LOGW(TAG, "wifi_mgr_start_ap called with empty SSID");
        return false;
    }

    wifi_mgr_init_once();

    s_ap_active = true;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    wifi_config_t ap_cfg = {0};
    strlcpy((char *)ap_cfg.ap.ssid, ap_ssid, sizeof(ap_cfg.ap.ssid));
    ap_cfg.ap.ssid_len = strlen(ap_ssid);
    ap_cfg.ap.max_connection = 4;
    ap_cfg.ap.channel = 1;
    ap_cfg.ap.beacon_interval = 100;
    ap_cfg.ap.authmode = (ap_pass && strlen(ap_pass) >= 8) ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;

    if (ap_cfg.ap.authmode != WIFI_AUTH_OPEN) {
        strlcpy((char *)ap_cfg.ap.password, ap_pass, sizeof(ap_cfg.ap.password));
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Starting AP: %s", ap_ssid);
    return true;
}

bool wifi_mgr_start_apsta(const char *ap_ssid, const char *ap_pass) {
    if (!ap_ssid || ap_ssid[0] == '\0') {
        ESP_LOGW(TAG, "wifi_mgr_start_apsta called with empty SSID");
        return false;
    }

    wifi_mgr_init_once();

    s_sta_active = true;
    s_ap_active = true;
    s_sta_connected = false;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    wifi_config_t ap_cfg = {0};
    strlcpy((char *)ap_cfg.ap.ssid, ap_ssid, sizeof(ap_cfg.ap.ssid));
    ap_cfg.ap.ssid_len = strlen(ap_ssid);
    ap_cfg.ap.max_connection = 4;
    ap_cfg.ap.channel = 1;
    ap_cfg.ap.beacon_interval = 100;
    ap_cfg.ap.authmode = (ap_pass && strlen(ap_pass) >= 8) ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;

    if (ap_cfg.ap.authmode != WIFI_AUTH_OPEN) {
        strlcpy((char *)ap_cfg.ap.password, ap_pass, sizeof(ap_cfg.ap.password));
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));

    s_sta_autoconnect = false;
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Starting APSTA: AP=%s", ap_ssid);
    return true;
}

void get_rssi(void) {
    wifi_ap_record_t rec;
    esp_err_t err = esp_wifi_sta_get_ap_info(&rec);
    if (err == ESP_OK) {
        int8_t rssi = rec.rssi;
        ESP_LOGI(TAG, "RSSI: %d", (int)rssi);
        return;
    }

    ESP_LOGW(TAG, "RSSI unavailable: %s", esp_err_to_name(err));
}