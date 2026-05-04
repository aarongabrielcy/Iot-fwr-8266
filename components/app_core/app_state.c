#include "app_state.h"
#include "esp_log.h"
#include <string.h>
#include <stddef.h>

static const char *TAG = "APP_STATE";

typedef struct {
    bool wifi_connected;
    bool mqtt_connected;
    bool time_synced;
    bool config_mode;
    bool scheduler_started;
    app_mode_t mode;
    char ip_addr[16];
    GeneralState general;
} app_state_data_t;

static app_state_data_t s_app_state = {
    .wifi_connected = false,
    .mqtt_connected = false,
    .time_synced = false,
    .config_mode = false,
    .scheduler_started = false,
    .mode = APP_MODE_NONE,
    .ip_addr = {0}
};


static void safe_copy_str(char *dst, size_t dst_size, const char *src)
{
    if (dst == NULL || dst_size == 0) {
        return;
    }
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, dst_size - 1);
    dst[dst_size - 1] = '\0';
}

void app_state_init(void) {
    s_app_state.wifi_connected = false;
    s_app_state.mqtt_connected = false;
    s_app_state.time_synced = false;
    s_app_state.config_mode = false;
    s_app_state.scheduler_started = false;
    s_app_state.mode = APP_MODE_NONE;
    s_app_state.ip_addr[0] = '\0';

    ESP_LOGI(TAG, "App state initialized");
}

void app_state_set_wifi_connected(bool connected) {
    s_app_state.wifi_connected = connected;
}

bool app_state_get_wifi_connected(void) {
    return s_app_state.wifi_connected;
}

void app_state_set_mqtt_connected(bool connected) {
    s_app_state.mqtt_connected = connected;
}

bool app_state_get_mqtt_connected(void) {
    return s_app_state.mqtt_connected;
}

void app_state_set_time_synced(bool synced) {
    s_app_state.time_synced = synced;
}

bool app_state_get_time_synced(void) {
    return s_app_state.time_synced;
}

void app_state_set_mode(app_mode_t mode) {
    s_app_state.mode = mode;
}

app_mode_t app_state_get_mode(void) {
    return s_app_state.mode;
}

void app_state_set_config_mode(bool enabled) {
    s_app_state.config_mode = enabled;
}

bool app_state_get_config_mode(void) {
    return s_app_state.config_mode;
}

void app_state_set_scheduler_started(bool started) {
    s_app_state.scheduler_started = started;
}

bool app_state_get_scheduler_started(void) {
    return s_app_state.scheduler_started;
}

void app_state_set_ip_addr(const char *ip_addr) {
    if (ip_addr == NULL) {
        s_app_state.ip_addr[0] = '\0';
        return;
    }

    strlcpy(s_app_state.ip_addr, ip_addr, sizeof(s_app_state.ip_addr));
}

const char *app_state_get_ip_addr(void) {
    return s_app_state.ip_addr;
}

/* general */
void app_state_set_device_id(const char *device_id){
    safe_copy_str(s_app_state.general.device_id, sizeof(s_app_state.general.device_id), device_id);
}

void app_state_set_fw_version(const char *fw_version){
    safe_copy_str(s_app_state.general.fw_version, sizeof(s_app_state.general.fw_version), fw_version);
}

void app_state_set_latitude(const char *latitude){
    safe_copy_str(s_app_state.general.latitude, sizeof(s_app_state.general.latitude), latitude);
}

void app_state_set_longitude(const char *longitude){
    safe_copy_str(s_app_state.general.longitude, sizeof(s_app_state.general.longitude), longitude);
}

void app_state_get_latitude(char *latitude, size_t len){
    safe_copy_str(latitude, len, s_app_state.general.latitude);
}

void app_state_get_longitude(char *longitude, size_t len){
    safe_copy_str(longitude, len, s_app_state.general.longitude);
}