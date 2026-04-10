#include "mqtt_parser.h"
#include "mqtt_topics.h"
#include "mqtt_mgr.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "MQTT_PARSER";

static bool payload_extract_bool(const char *payload, const char *key, bool *value) {
    if (payload == NULL || key == NULL || value == NULL) {
        return false;
    }

    char pattern[64];
    int ret = snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    if (ret < 0 || (size_t)ret >= sizeof(pattern)) {
        return false;
    }

    const char *pos = strstr(payload, pattern);
    if (!pos) {
        return false;
    }

    pos += strlen(pattern);

    while (*pos == ' ' || *pos == '\t') {
        pos++;
    }

    if (strncmp(pos, "true", 4) == 0) {
        *value = true;
        return true;
    }

    if (strncmp(pos, "false", 5) == 0) {
        *value = false;
        return true;
    }

    if (*pos == '1') {
        *value = true;
        return true;
    }

    if (*pos == '0') {
        *value = false;
        return true;
    }

    return false;
}

bool mqtt_parser_parse_output_command(const char *topic,
                                      const char *payload,
                                      app_cmd_output_t *out_cmd,
                                      int *command) {
    if (topic == NULL || payload == NULL || out_cmd == NULL) {
        return false;
    }

    char expected_cmd_topic[128];
    if (!mqtt_topics_build_cmd(expected_cmd_topic, sizeof(expected_cmd_topic))) {
        ESP_LOGE(TAG, "failed to build expected cmd topic");
        return false;
    }

    if (strcmp(topic, expected_cmd_topic) != 0) {
        return false;
    }

    /*
     * Formato mínimo esperado:
     * {"output_id":1,"value":true}
     * o
     * {"output_id":1,"value":1}
     *
     * Esto no es un parser JSON real. Es deliberadamente simple.
     * Si luego tu payload crece, migras a cJSON o parser serio.
     */

    unsigned output_id = 0;
    //bool value = false;
    int s_command;
    char val[512];
    int found = sscanf(payload, "{\"%d\":\"%[^\"]\"", &s_command, val);
    if(found == 2){
        switch (s_command){
            case OUTPUT_1:
                *command = s_command;
                out_cmd->output_id = 1;
                if(strcmp(val, "1") == 0){
                    out_cmd->value = true;
                }
                else{
                    out_cmd->value = false;
                }
                return true;
                //post_system_event(OUTPUT, value);
                break;
            case OUTPUT_2:
                /* code */
                break;
            case OTA:
                //post_system_event(OTA_UPDATE, value);
                break;
            case TRK:
                *command = s_command;
                return true;
                break;
            default:
                return false;
                break;
            }
        
    }
    else{
        return false;
    }
    /*const char *out_id_pos = strstr(payload, "\"output_id\":");
    if (!out_id_pos) {
        return false;
    }

    if (sscanf(out_id_pos, "\"output_id\":%u", &output_id) != 1) {
        return false;
    }

    if (!payload_extract_bool(payload, "value", &value)) {
        return false;
    }

    out_cmd->output_id = (uint8_t)output_id;
    out_cmd->value = value;*/

    return true;
}