#include "mqtt_topics.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

/*
 * Ajusta estos includes/nombres a tu cfg real.
 * Aquí asumo que tu componente cfg expone algo parecido
 * a una función para recuperar device_id.
 */
#include "cfg.h"

static const char *TAG = "MQTT_TOPICS";

static bool mqtt_topic_build(char *buf, size_t len, const char *suffix) {
    if (buf == NULL || len == 0 || suffix == NULL) {
        return false;
    }

    const char *device_id = cfg_get_device_id();
    if (device_id == NULL || device_id[0] == '\0') {
        ESP_LOGE(TAG, "device_id is empty");
        buf[0] = '\0'; 
        return false;
    }

    int written = snprintf(buf, len, "devices/%s/%s", device_id, suffix);
    if (written < 0 || (size_t)written >= len) {
        ESP_LOGE(TAG, "topic buffer too small");
        buf[0] = '\0';
        return false;
    }

    return true;
}

bool mqtt_topics_build_cmd(char *buf, size_t len) {
    return mqtt_topic_build(buf, len, "cmd");
}

bool mqtt_topics_build_status(char *buf, size_t len) {
    return mqtt_topic_build(buf, len, "status");
}

bool mqtt_topics_build_telemetry(char *buf, size_t len) {
    return mqtt_topic_build(buf, len, "telemetry");
}

bool mqtt_topics_build_ack(char *buf, size_t len) {
    return mqtt_topic_build(buf, len, "ack");
}