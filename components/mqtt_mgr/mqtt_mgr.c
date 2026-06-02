#include "mqtt_mgr.h"
#include "mqtt_topics.h"
#include "mqtt_parser.h"
#include "app_events.h"
#include "app_events_ids.h"
#include "app_event_data.h"

#include "esp_log.h"
#include "esp_err.h"
#include "mqtt_client.h"
#include <cJson.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "cfg.h"

static const char *TAG = "MQTT_MGR";

static esp_mqtt_client_handle_t s_client = NULL;
static bool s_connected = false;

static char s_device_id[64];
static char s_topic_cmd[128];
static char s_topic_status[128];
static char s_topic_telemetry[128];
static char s_topic_ack[128];

static void publish_command_ack(const char *command_id, bool ok, int command_code, const char *value, const char *detail);
static bool process_command(const char *command_id, int command_code, const char *value);
static const char *json_value_to_string(cJSON *item, char *buffer, size_t buffer_size);

static bool mqtt_mgr_prepare_topics(void)
{
    return mqtt_topics_build_cmd(s_topic_cmd, sizeof(s_topic_cmd)) &&
           mqtt_topics_build_status(s_topic_status, sizeof(s_topic_status)) &&
           mqtt_topics_build_telemetry(s_topic_telemetry, sizeof(s_topic_telemetry)) &&
           mqtt_topics_build_ack(s_topic_ack, sizeof(s_topic_ack));
}

static bool mqtt_mgr_publish_raw(const char *topic, const char *payload, int qos, int retain)
{
    if (s_client == NULL || topic == NULL || payload == NULL)
    {
        return false;
    }

    int msg_id = esp_mqtt_client_publish(s_client, topic, payload, 0, qos, retain);
    if (msg_id < 0)
    {
        ESP_LOGW(TAG, "MQTT raw publish failed topic=%s", topic);
        return false;
    }

    ESP_LOGI(TAG, "MQTT raw publish ok msg_id=%d topic=%s payload=%s", msg_id, topic, payload);
    return true;
}

static void mqtt_mgr_publish_connected_ack(void)
{
    char payload[128];

    int written = snprintf(
        payload,
        sizeof(payload),
        "{\"ok\":true,\"msg\":\"connected\",\"code\":-1}");

    if (written < 0 || (size_t)written >= sizeof(payload))
    {
        ESP_LOGW(TAG, "ACK payload buffer too small");
        return;
    }

    mqtt_mgr_publish_raw(s_topic_ack, payload, 1, 0);
}

static void mqtt_mgr_handle_connected(void)
{
    s_connected = true;

    int msg_id = esp_mqtt_client_subscribe(s_client, s_topic_cmd, 1);
    ESP_LOGI(TAG, "Subscribed to cmd topic, msg_id=%d topic=%s", msg_id, s_topic_cmd);

    mqtt_mgr_publish_raw(s_topic_status, "online", 1, 1);
    mqtt_mgr_publish_raw(s_topic_telemetry, "{\"boot\":1}", 1, 0);
    mqtt_mgr_publish_connected_ack();

    app_post_system_event(APP_SYS_MQTT_CONNECTED, NULL, 0);
}

static void mqtt_mgr_handle_disconnected(void)
{
    s_connected = false;
    app_post_system_event(APP_SYS_MQTT_DISCONNECTED, NULL, 0);
}

static void mqtt_mgr_handle_data(const char *topic, const char *payload)
{
    if (topic == NULL)
    {
        return;
    }

    if (payload == NULL)
    {
        publish_command_ack(NULL, false, -1, NULL, "Empty payload");
    }

    cJSON *root = cJSON_Parse(payload);
    if (!root)
    {
        publish_command_ack(NULL, false, -1, NULL, "invalid json");
        return;
    }

    cJSON *command_id_item = cJSON_GetObjectItem(root, "commandId");
    if (!cJSON_IsString(command_id_item) || !command_id_item->valuestring || strlen(command_id_item->valuestring) == 0)
    {
        publish_command_ack(NULL, false, -1, NULL, "missing commandId");
        cJSON_Delete(root);
        return;
    }

    const char *command_id = command_id_item->valuestring;

    cJSON *commands = cJSON_GetObjectItem(root, "commands");
    if (!cJSON_IsObject(commands))
    {
        publish_command_ack(command_id, false, -1, NULL, "missing commands object");
        cJSON_Delete(root);
        return;
    }

    int processed_count = 0;

    cJSON *command_item = NULL;
    cJSON_ArrayForEach(command_item, commands)
    {
        if (!command_item->string)
        {
            continue;
        }

        int command_code = atoi(command_item->string);

        char value_buffer[256] = {0};
        const char *value = json_value_to_string(command_item, value_buffer, sizeof(value_buffer));

        bool ok = process_command(command_id, command_code, value);

        if (ok)
        {
            publish_command_ack(command_id, true, command_code, value, "command accepted");
        }
        else
        {
            publish_command_ack(command_id, false, command_code, value, "unsupported or invalid command");
        }

        processed_count++;
    }

    if (processed_count == 0)
    {
        publish_command_ack(command_id, false, -1, NULL, "commands object is empty");
    }

    cJSON_Delete(root);
}

static bool process_command(const char *command_id, int command_code, const char *value)
{
    (void)command_id;
    switch (command_code)
    {
    case OUTPUT_1:
        if (!value || (strcmp(value, "0") != 0 && strcmp(value, "1") != 0))
        {
            ESP_LOGW(TAG, "OUTPUT_1 invalid value: %s", value ? value : "null");
            return false;
        }

        app_cmd_output_t cmd = {
            .output_id = 1,
            .value = strcmp(value, "1") == 0};

        ESP_LOGI(TAG, "Command OUTPUT_1 -> %s", value);
        app_post_command_event(APP_CMD_SET_OUTPUT, &cmd, sizeof(cmd));
        return true;

    case OTA:
        // if (!value || strlen(value) == 0)
        // {
        //     ESP_LOGW(TAG, "OTA requires URL value");
        //     return false;
        // }

        // ESP_LOGI(TAG, "Command OTA -> %s", value);
        // post_system_event(OTA_UPDATE, value);
        return true;

    case REBOOT:
        // ESP_LOGI(TAG, "Command REBOOT");
        // post_system_event(REBOOT_SYSTEM);
        return true;

    case TRK:
        ESP_LOGI(TAG, "Command TRK");
        app_post_command_event(APP_CMD_REQUEST_REPORT, NULL, 0);
        return true;

    default:
        ESP_LOGW(TAG, "Unsupported command code: %d", command_code);
        return false;
    }
}

static esp_err_t mqtt_event_handler_legacy(esp_mqtt_event_handle_t event)
{
    if (event == NULL)
    {
        return ESP_FAIL;
    }

    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected");
        mqtt_mgr_handle_connected();
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT disconnected");
        mqtt_mgr_handle_disconnected();
        break;

    case MQTT_EVENT_DATA:
    {
        int topic_len = event->topic_len;
        int data_len = event->data_len;

        if (topic_len <= 0 || data_len <= 0)
        {
            ESP_LOGW(TAG, "MQTT_EVENT_DATA with empty topic or payload");
            break;
        }

        char *topic = (char *)calloc((size_t)topic_len + 1U, sizeof(char));
        char *payload = (char *)calloc((size_t)data_len + 1U, sizeof(char));
        if (topic == NULL || payload == NULL)
        {
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
    if (s_client != NULL)
    {
        ESP_LOGW(TAG, "MQTT manager already initialized");
        return true;
    }

    if (!mqtt_mgr_prepare_topics())
    {
        ESP_LOGE(TAG, "Failed to prepare MQTT topics");
        return false;
    }

    const char *host = cfg_get_mqtt_host();
    int port = cfg_get_mqtt_port();
    const char *user = cfg_get_mqtt_user();
    const char *pass = cfg_get_mqtt_pass();

    if (host == NULL || host[0] == '\0' || port <= 0)
    {
        ESP_LOGE(TAG, "Invalid MQTT config");
        return false;
    }

    esp_mqtt_client_config_t mqtt_cfg = {
        .host = host,
        .port = port,
        .username = user,
        .password = pass,
        .event_handle = mqtt_event_handler_legacy,

        .keepalive = 60,
        .reconnect_timeout_ms = 5000,

        .disable_auto_reconnect = false,

        .lwt_topic = s_topic_status,
        .lwt_msg = "offline",
        .lwt_msg_len = strlen("offline"),
        .lwt_qos = 1,
        .lwt_retain = 1,

        .buffer_size = 1024,
    };

    s_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_client == NULL)
    {
        ESP_LOGE(TAG, "esp_mqtt_client_init failed");
        return false;
    }

    ESP_LOGI(TAG, "MQTT manager initialized");
    return true;
}

bool mqtt_mgr_start(app_cfg_t *cfg)
{
    if (s_client == NULL)
    {
        ESP_LOGE(TAG, "MQTT manager not initialized");
        return false;
    }

    memset(s_device_id, 0, sizeof(s_device_id));
    snprintf(s_device_id, sizeof(s_device_id), "%s", cfg->device_id);

    esp_err_t err = esp_mqtt_client_start(s_client);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_mqtt_client_start failed: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "MQTT manager started");
    return true;
}

bool mqtt_mgr_stop(void)
{
    if (s_client == NULL)
    {
        ESP_LOGW(TAG, "MQTT manager not initialized");
        return false;
    }

    esp_err_t err = esp_mqtt_client_stop(s_client);
    if (err != ESP_OK)
    {
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
    if (s_client == NULL || topic == NULL || payload == NULL)
    {
        return false;
    }

    if (!s_connected)
    {
        ESP_LOGW(TAG, "MQTT publish skipped: not connected");
        return false;
    }

    int msg_id = esp_mqtt_client_publish(s_client, topic, payload, 0, qos, retain);
    if (msg_id < 0)
    {
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

static void publish_command_ack(const char *command_id, bool ok, int command_code, const char *value, const char *detail)
{
    if (!s_client)
    {
        return;
    }

    char payload[512];

    const char *safe_detail = detail ? detail : "";
    const char *safe_value = value ? value : "";

    if (command_id && strlen(command_id) > 0)
    {
        if (value)
        {
            snprintf(
                payload,
                sizeof(payload),
                "{\"commandId\":\"%s\",\"ok\":%s,\"command\":%d,\"value\":\"%s\",\"detail\":\"%s\",\"device_id\":\"%s\"}",
                command_id,
                ok ? "true" : "false",
                command_code,
                safe_value,
                safe_detail,
                s_device_id);
        }
        else
        {
            snprintf(
                payload,
                sizeof(payload),
                "{\"commandId\":\"%s\",\"ok\":%s,\"command\":%d,\"detail\":\"%s\",\"device_id\":\"%s\"}",
                command_id,
                ok ? "true" : "false",
                command_code,
                safe_detail,
                s_device_id);
        }
    }
    else
    {
        snprintf(
            payload,
            sizeof(payload),
            "{\"commandId\":null,\"ok\":%s,\"command\":%d,\"detail\":\"%s\",\"device_id\":\"%s\"}",
            ok ? "true" : "false",
            command_code,
            safe_detail,
            s_device_id);
    }

    esp_mqtt_client_publish(s_client, s_topic_ack, payload, 0, 1, 0);
}

static const char *json_value_to_string(cJSON *item, char *buffer, size_t buffer_size)
{
    if (!item || !buffer || buffer_size == 0)
    {
        return NULL;
    }

    if (cJSON_IsString(item))
    {
        return item->valuestring;
    }

    if (cJSON_IsNumber(item))
    {
        snprintf(buffer, buffer_size, "%d", item->valueint);
        return buffer;
    }

    if (cJSON_IsBool(item))
    {
        snprintf(buffer, buffer_size, "%d", cJSON_IsTrue(item) ? 1 : 0);
        return buffer;
    }

    if (cJSON_IsNull(item))
    {
        return NULL;
    }

    char *printed = cJSON_PrintUnformatted(item);
    if (!printed)
    {
        return NULL;
    }

    snprintf(buffer, buffer_size, "%s", printed);
    cJSON_free(printed);

    return buffer;
}