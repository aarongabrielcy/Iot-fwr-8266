#include "mqtt_mgr.h"
#include "mqtt_topics.h"
#include "mqtt_parser.h"
#include "app_events.h"
#include "app_events_ids.h"
#include "app_event_data.h"

#include "esp_log.h"
#include "esp_err.h"
#include "mqtt_client.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "cfg.h"

static const char *TAG = "MQTT_MGR";

static esp_mqtt_client_handle_t s_client = NULL;
static bool s_connected = false;

static char s_topic_cmd[128];
static char s_topic_status[128];
static char s_topic_telemetry[128];
static char s_topic_ack[128];

static bool mqtt_mgr_prepare_topics(void)
{
    return mqtt_topics_build_cmd(s_topic_cmd, sizeof(s_topic_cmd)) &&
           mqtt_topics_build_status(s_topic_status, sizeof(s_topic_status)) &&
           mqtt_topics_build_telemetry(s_topic_telemetry, sizeof(s_topic_telemetry)) &&
           mqtt_topics_build_ack(s_topic_ack, sizeof(s_topic_ack));
}

static void mqtt_mgr_handle_connected(void)
{
    s_connected = true;

    int msg_id = esp_mqtt_client_subscribe(s_client, s_topic_cmd, 1);
    ESP_LOGI(TAG, "Subscribed to cmd topic, msg_id=%d topic=%s", msg_id, s_topic_cmd);

    app_post_system_event(APP_SYS_MQTT_CONNECTED, NULL, 0);
}

static void mqtt_mgr_handle_disconnected(void)
{
    s_connected = false;
    app_post_system_event(APP_SYS_MQTT_DISCONNECTED, NULL, 0);
}

static void mqtt_mgr_handle_data(const char *topic, const char *payload)
{
    if (topic == NULL || payload == NULL) {
        return;
    }

    ESP_LOGI(TAG, "MQTT RX topic=%s payload=%s", topic, payload);

    app_cmd_output_t cmd;
    if (mqtt_parser_parse_output_command(topic, payload, &cmd)) {
        app_post_command_event(APP_CMD_SET_OUTPUT, &cmd, sizeof(cmd));
        return;
    }

    ESP_LOGW(TAG, "Unhandled MQTT command");
}

static esp_err_t mqtt_event_handler_legacy(esp_mqtt_event_handle_t event)
{
    if (event == NULL) {
        return ESP_FAIL;
    }

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            mqtt_mgr_handle_connected();
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected");
            mqtt_mgr_handle_disconnected();
            break;

        case MQTT_EVENT_DATA: {
            int topic_len = event->topic_len;
            int data_len = event->data_len;

            if (topic_len <= 0 || data_len <= 0) {
                ESP_LOGW(TAG, "MQTT_EVENT_DATA with empty topic or payload");
                break;
            }

            char *topic = (char *)calloc((size_t)topic_len + 1U, sizeof(char));
            char *payload = (char *)calloc((size_t)data_len + 1U, sizeof(char));
            if (topic == NULL || payload == NULL) {
                ESP_LOGE(TAG, "Memory allocation failed for MQTT RX");
                free(topic);
                free(payload);
                break;
            }

            memcpy(topic, event->topic, (size_t)topic_len);
            memcpy(payload, event->data, (size_t)data_len);
            topic[topic_len] = '\0';
            payload[data_len] = '\0';

            mqtt_mgr_handle_data(topic, payload);

            free(topic);
            free(payload);
            break;
        }

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error event");
            break;

        default:
            ESP_LOGD(TAG, "Unhandled MQTT event id=%d", event->event_id);
            break;
    }

    return ESP_OK;
}

bool mqtt_mgr_init(void)
{
    if (s_client != NULL) {
        ESP_LOGW(TAG, "MQTT manager already initialized");
        return true;
    }

    if (!mqtt_mgr_prepare_topics()) {
        ESP_LOGE(TAG, "Failed to prepare MQTT topics");
        return false;
    }

    const char *host = cfg_get_mqtt_host();
    int port = cfg_get_mqtt_port();
    const char *user = cfg_get_mqtt_user();
    const char *pass = cfg_get_mqtt_pass();

    if (host == NULL || host[0] == '\0' || port <= 0) {
        ESP_LOGE(TAG, "Invalid MQTT config");
        return false;
    }

    esp_mqtt_client_config_t mqtt_cfg = {
        .host = host,
        .port = port,
        .username = user,
        .password = pass,
        .event_handle = mqtt_event_handler_legacy
    };

    s_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_client == NULL) {
        ESP_LOGE(TAG, "esp_mqtt_client_init failed");
        return false;
    }

    ESP_LOGI(TAG, "MQTT manager initialized");
    return true;
}

bool mqtt_mgr_start(void)
{
    if (s_client == NULL) {
        ESP_LOGE(TAG, "MQTT manager not initialized");
        return false;
    }

    esp_err_t err = esp_mqtt_client_start(s_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_mqtt_client_start failed: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "MQTT manager started");
    return true;
}

bool mqtt_mgr_stop(void)
{
    if (s_client == NULL) {
        ESP_LOGW(TAG, "MQTT manager not initialized");
        return false;
    }

    esp_err_t err = esp_mqtt_client_stop(s_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_mqtt_client_stop failed: %s", esp_err_to_name(err));
        return false;
    }

    s_connected = false;
    ESP_LOGI(TAG, "MQTT manager stopped");
    return true;
}

bool mqtt_mgr_is_connected(void)
{
    return s_connected;
}

static bool mqtt_mgr_publish(const char *topic, const char *payload, int qos, int retain)
{
    if (s_client == NULL || topic == NULL || payload == NULL) {
        return false;
    }

    if (!s_connected) {
        ESP_LOGW(TAG, "MQTT publish skipped: not connected");
        return false;
    }

    int msg_id = esp_mqtt_client_publish(s_client, topic, payload, 0, qos, retain);
    if (msg_id < 0) {
        ESP_LOGW(TAG, "MQTT publish failed topic=%s", topic);
        return false;
    }

    ESP_LOGI(TAG, "MQTT publish ok msg_id=%d topic=%s payload=%s", msg_id, topic, payload);
    return true;
}

bool mqtt_mgr_publish_telemetry(const char *payload)
{
    return mqtt_mgr_publish(s_topic_telemetry, payload, 1, 0);
}

bool mqtt_mgr_publish_status(const char *payload)
{
    return mqtt_mgr_publish(s_topic_status, payload, 1, 1);
}

bool mqtt_mgr_publish_ack(const char *payload)
{
    return mqtt_mgr_publish(s_topic_ack, payload, 1, 0);
}