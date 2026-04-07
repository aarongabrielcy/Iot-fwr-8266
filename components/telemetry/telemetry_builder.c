#include "telemetry_builder.h"

#include "cfg.h"
#include "app_state.h"
#include "app_events_ids.h"
#include "io_sensor.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

static const char *TAG = "TELEMETRY_BUILDER";

static unsigned long telemetry_get_uptime_s(void)
{
    TickType_t ticks = xTaskGetTickCount();
    return (unsigned long)((ticks * portTICK_PERIOD_MS) / 1000U);
}

static unsigned long telemetry_get_epoch_time(void)
{
    time_t now = time(NULL);
    if (now <= 0) {
        return 0UL;
    }
    return (unsigned long)now;
}

static void telemetry_copy_str(char *dst, size_t dst_len, const char *src)
{
    if (dst == NULL || dst_len == 0) {
        return;
    }

    if (src == NULL) {
        dst[0] = '\0';
        return;
    }

    strlcpy(dst, src, dst_len);
}

static int telemetry_get_output_1(void)
{
    bool out1 = false;
    if (!io_sensor_get_output(1, &out1)) {
        return 0;
    }
    return out1 ? 1 : 0;
}

static bool telemetry_build_common(char *buf, size_t len, int event_id)
{
    if (buf == NULL || len == 0) {
        return false;
    }

    buf[0] = '\0';

    char device_id[CFG_DEVICE_ID_MAX] = {0};
    char fw[16] = {0};
    char ssid[CFG_WIFI_SSID_MAX] = {0};
    char ip_addr[16] = {0};
    char mac_wifi[CFG_WIFI_BT_MAC] = {0};

    /* placeholders actuales */
    char mac_bt[18] = {0};
    char latitude[20] = {0};
    char longitude[20] = {0};

    telemetry_copy_str(device_id, sizeof(device_id), cfg_get_device_id());

    {
        const app_cfg_t *active = cfg_get_active();
        if (active != NULL) {
            telemetry_copy_str(fw, sizeof(fw), active->cfg_version);
        }
    }

    telemetry_copy_str(ssid, sizeof(ssid), cfg_get_wifi_ssid());
    telemetry_copy_str(ip_addr, sizeof(ip_addr), app_state_get_ip_addr());
    telemetry_copy_str(mac_wifi, sizeof(mac_wifi), cfg_get_wifi_mac());

    unsigned long dt = telemetry_get_epoch_time();
    unsigned long uptime_s = telemetry_get_uptime_s();

    int net_mode = (int)app_state_get_mode();
    int rssi = 0;
    int out1 = telemetry_get_output_1();

    int written = snprintf(
        buf,
        len,
        "{"
            "\"event\":%d,"
            "\"dt\":%lu,"
            "\"device_id\":\"%s\","
            "\"fw\":\"%s\","
            "\"uptime_s\":%lu,"
            "\"net\":{"
                "\"mode\":\"%d\","
                "\"ssid\":\"%s\","
                "\"ip\":\"%s\","
                "\"rssi\":%d"
            "},"
            "\"hw\":{"
                "\"mac_wifi\":\"%s\","
                "\"mac_bt\":\"%s\""
            "},"
            "\"location\":{"
                "\"latitude\":\"%s\","
                "\"longitude\":\"%s\""
            "},"
            "\"io\":{"
                "\"outputs\":{"
                    "\"out1\":%d"
                "}"
            "}"
        "}",
        event_id,
        dt,
        device_id,
        fw,
        uptime_s,
        net_mode,
        ssid,
        ip_addr,
        rssi,
        mac_wifi,
        mac_bt,
        latitude,
        longitude,
        out1
    );

    if (written < 0) {
        ESP_LOGE(TAG, "snprintf failed");
        buf[0] = '\0';
        return false;
    }

    if ((size_t)written >= len) {
        ESP_LOGW(TAG, "Telemetry buffer too small");
        buf[0] = '\0';
        return false;
    }

    return true;
}

bool telemetry_builder_build_tracking(char *buf, size_t len)
{
    return telemetry_build_common(buf, len, APP_RPT_TRACKING);
}

bool telemetry_builder_build_boot(char *buf, size_t len)
{
    return telemetry_build_common(buf, len, APP_RPT_BOOT);
}

bool telemetry_builder_build_output_status(char *buf, size_t len)
{
    return telemetry_build_common(buf, len, APP_RPT_OUTPUT_STATUS);
}

bool telemetry_builder_build_input_status(char *buf, size_t len)
{
    return telemetry_build_common(buf, len, APP_RPT_INPUT_STATUS);
}

bool telemetry_builder_build_sensor_status(char *buf, size_t len)
{
    return telemetry_build_common(buf, len, APP_RPT_SENSOR_STATUS);
}

bool telemetry_builder_build_alert(char *buf, size_t len)
{
    return telemetry_build_common(buf, len, APP_RPT_ALERT);
}