#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "web_cfg_handlers.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_http_server.h"

#include "cfg.h"
#include "app_events.h"
#include "app_events_ids.h"
#include "app_event_data.h"
#include "app_state.h"
#include "scheduler.h"

#ifndef HTTPD_RESP_USE_STRLEN
#define HTTPD_RESP_USE_STRLEN -1
#endif

#define SESSION_TIMEOUT_SECONDS 1800   // 30 min
#define SESSION_COOKIE_NAME     "session_id"
static char s_session_id[64] = {0};
static time_t s_session_last_activity = 0;

static const char *TAG = "WEB_CFG_HDL";

/* cfg pointer ------------------------------------------------------------ */
static app_cfg_t *s_cfg = NULL;

#define SESSION_TIMEOUT_SECONDS 1800   // 30 min
#define SESSION_COOKIE_NAME     "session_id"

static char s_session_id[64] = {0};
static time_t s_session_last_activity = 0;

static void session_clear(void)
{
    s_session_id[0] = '\0';
    s_session_last_activity = 0;
}

static void session_create(void)
{
    uint32_t r1 = esp_random();
    uint32_t r2 = esp_random();
    time_t now = time(NULL);

    snprintf(s_session_id, sizeof(s_session_id), "%08lx%08lx%08lx",
             (unsigned long)now,
             (unsigned long)r1,
             (unsigned long)r2);

    s_session_last_activity = now;
}

static bool cookie_extract_value(const char *cookie_header,
                                 const char *name,
                                 char *out,
                                 size_t out_len)
{
    if (!cookie_header || !name || !out || out_len == 0) return false;

    const char *p = cookie_header;
    size_t name_len = strlen(name);

    while (*p) {
        while (*p == ' ') p++;

        if (strncmp(p, name, name_len) == 0 && p[name_len] == '=') {
            p += name_len + 1;
            const char *end = strchr(p, ';');
            size_t len = end ? (size_t)(end - p) : strlen(p);
            if (len >= out_len) len = out_len - 1;
            memcpy(out, p, len);
            out[len] = '\0';
            return true;
        }

        p = strchr(p, ';');
        if (!p) break;
        p++;
    }

    return false;
}

bool web_cfg_is_authenticated(httpd_req_t *req)
{
    if (!req) return false;
    if (s_session_id[0] == '\0') return false;

    time_t now = time(NULL);
    if (now <= 0) {
        // Si no hay tiempo válido, de todos modos usa la sesión en RAM,
        // pero no permitas que dure para siempre si el tiempo luego aparece.
        now = s_session_last_activity;
    }

    if (s_session_last_activity > 0 &&
        (now - s_session_last_activity) > SESSION_TIMEOUT_SECONDS) {
        session_clear();
        return false;
    }

    char cookie_header[256] = {0};
    if (httpd_req_get_hdr_value_str(req, "Cookie", cookie_header, sizeof(cookie_header)) != ESP_OK) {
        return false;
    }

    char session_cookie[80] = {0};
    if (!cookie_extract_value(cookie_header, SESSION_COOKIE_NAME, session_cookie, sizeof(session_cookie))) {
        return false;
    }

    if (strcmp(session_cookie, s_session_id) != 0) {
        return false;
    }

    s_session_last_activity = now;
    return true;
}

esp_err_t web_cfg_redirect_login(httpd_req_t *req)
{
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, NULL, 0);
}

static bool require_auth_json_or_401(httpd_req_t *req)
{
    if (web_cfg_is_authenticated(req)) return true;

    httpd_resp_set_status(req, "401 Unauthorized");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_send(req, "Unauthorized", HTTPD_RESP_USE_STRLEN);
    return false;
}

static void reboot_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(800));
    esp_restart();
}

void web_cfg_handlers_set_cfg(app_cfg_t *cfg)
{
    s_cfg = cfg;
}

static void http_send_json(httpd_req_t *req, const char *json)
{
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t http_send_text(httpd_req_t *req, const char *text)
{
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_send(req, text, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static int http_read_body(httpd_req_t *req, char *buf, int buf_len)
{
    if (buf == NULL || buf_len <= 1) {
        return -1;
    }

    int total = req->content_len;
    if (total <= 0) {
        buf[0] = '\0';
        return 0;
    }

    if (total >= buf_len) {
        total = buf_len - 1;
    }

    int received = 0;
    while (received < total) {
        int r = httpd_req_recv(req, buf + received, total - received);
        if (r <= 0) {
            return -1;
        }
        received += r;
    }

    buf[received] = '\0';
    return received;
}

static void url_decode_inplace(char *s)
{
    if (!s) {
        return;
    }

    char *src = s;
    char *dst = s;

    while (*src) {
        if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else if (*src == '%' && src[1] && src[2]) {
            char h1 = src[1];
            char h2 = src[2];

            int v1 = (h1 >= '0' && h1 <= '9') ? (h1 - '0') :
                     (h1 >= 'A' && h1 <= 'F') ? (h1 - 'A' + 10) :
                     (h1 >= 'a' && h1 <= 'f') ? (h1 - 'a' + 10) : -1;

            int v2 = (h2 >= '0' && h2 <= '9') ? (h2 - '0') :
                     (h2 >= 'A' && h2 <= 'F') ? (h2 - 'A' + 10) :
                     (h2 >= 'a' && h2 <= 'f') ? (h2 - 'a' + 10) : -1;

            if (v1 >= 0 && v2 >= 0) {
                *dst++ = (char)((v1 << 4) | v2);
                src += 3;
            } else {
                *dst++ = *src++;
            }
        } else {
            *dst++ = *src++;
        }
    }

    *dst = '\0';
}

static bool form_get(char *body, const char *key, char *out, size_t out_len)
{
    if (!body || !key || !out || out_len == 0) {
        return false;
    }

    char pattern[64];
    snprintf(pattern, sizeof(pattern), "%s=", key);

    char *p = strstr(body, pattern);
    if (!p) {
        return false;
    }

    p += strlen(pattern);

    char *end = strchr(p, '&');
    size_t n = end ? (size_t)(end - p) : strlen(p);

    if (n >= out_len) {
        n = out_len - 1;
    }

    memcpy(out, p, n);
    out[n] = '\0';
    url_decode_inplace(out);
    return true;
}

static esp_err_t cfg_get_handler(httpd_req_t *req)
{
    if (s_cfg == NULL) {
        http_send_json(req, "{\"ok\":false,\"error\":\"cfg not set\"}");
        return ESP_OK;
    }

    char json[768];
    snprintf(json, sizeof(json),
        "{\"ok\":true,"
        "\"device_id\":\"%s\","
        "\"wifi_ssid\":\"%s\","
        "\"ip_dhcp\":\"%s\","
        "\"mqtt_host\":\"%s\","
        "\"mqtt_port\":%d,"
        "\"mqtt_user\":\"%s\","
        "\"telemetry_interval\":%u,"
        "\"cfg_version\":\"%s\"}",
        s_cfg->device_id,
        s_cfg->wifi_ssid,
        s_cfg->ip_dhcp,
        s_cfg->mqtt_host,
        s_cfg->mqtt_port,
        s_cfg->mqtt_user,
        (unsigned)s_cfg->telemetry_interval_s,
        s_cfg->cfg_version
    );

    http_send_json(req, json);
    return ESP_OK;
}

static esp_err_t scan_get_handler(httpd_req_t *req)
{
    wifi_scan_config_t scan_cfg = {
        .ssid = 0,
        .bssid = 0,
        .channel = 0,
        .show_hidden = true
    };

    esp_err_t err = esp_wifi_scan_start(&scan_cfg, true);
    if (err != ESP_OK) {
        http_send_json(req, "{\"networks\":[],\"error\":\"scan failed\"}");
        return ESP_OK;
    }

    uint16_t n = 0;
    esp_wifi_scan_get_ap_num(&n);

    if (n == 0) {
        http_send_json(req, "{\"networks\":[]}");
        return ESP_OK;
    }

    const uint16_t max = (n > 10) ? 10 : n;
    wifi_ap_record_t recs[10];
    uint16_t out_n = max;
    esp_wifi_scan_get_ap_records(&out_n, recs);

    char json[1024];
    int pos = snprintf(json, sizeof(json), "{\"networks\":[");
    for (int i = 0; i < out_n; i++) {
        pos += snprintf(json + pos, sizeof(json) - (size_t)pos,
                        "%s{\"ssid\":\"%s\",\"rssi\":%d,\"auth\":%d}",
                        (i == 0) ? "" : ",",
                        (char *)recs[i].ssid,
                        recs[i].rssi,
                        (int)recs[i].authmode);
    }
    snprintf(json + pos, sizeof(json) - (size_t)pos, "]}");

    http_send_json(req, json);
    return ESP_OK;
}

static esp_err_t save_post_handler(httpd_req_t *req)
{
    if (s_cfg == NULL) {
        return http_send_text(req, "cfg not set");
    }

    char body[512];
    if (http_read_body(req, body, sizeof(body)) < 0) {
        return ESP_FAIL;
    }

    char ssid[33] = {0};
    char pass[65] = {0};
    char mqtt_host[64] = {0};
    char mqtt_port[8] = {0};
    char mqtt_user[33] = {0};
    char mqtt_pass[65] = {0};

    form_get(body, "ssid", ssid, sizeof(ssid));
    form_get(body, "pass", pass, sizeof(pass));
    form_get(body, "mqtt_host", mqtt_host, sizeof(mqtt_host));
    form_get(body, "mqtt_port", mqtt_port, sizeof(mqtt_port));
    form_get(body, "mqtt_user", mqtt_user, sizeof(mqtt_user));
    form_get(body, "mqtt_pass", mqtt_pass, sizeof(mqtt_pass));

    if (ssid[0] != '\0') {
        strlcpy(s_cfg->wifi_ssid, ssid, sizeof(s_cfg->wifi_ssid));
    }

    /*
     * Mantengo tu comportamiento original:
     * solo sobreescribe password si viene no vacía.
     */
    if (pass[0] != '\0') {
        strlcpy(s_cfg->wifi_pass, pass, sizeof(s_cfg->wifi_pass));
    }

    if (mqtt_host[0] != '\0') {
        strlcpy(s_cfg->mqtt_host, mqtt_host, sizeof(s_cfg->mqtt_host));
    }

    if (mqtt_port[0] != '\0') {
        s_cfg->mqtt_port = atoi(mqtt_port);
    }

    if (mqtt_user[0] != '\0') {
        strlcpy(s_cfg->mqtt_user, mqtt_user, sizeof(s_cfg->mqtt_user));
    }

    if (mqtt_pass[0] != '\0') {
        strlcpy(s_cfg->mqtt_pass, mqtt_pass, sizeof(s_cfg->mqtt_pass));
    }

    cfg_set_active(s_cfg);

    if (!cfg_save(s_cfg)) {
        return http_send_text(req, "Error guardando configuracion");
    }

    http_send_text(req, "Configuracion guardada. Reiniciando...");
    xTaskCreate(reboot_task, "reboot_task", 2048, NULL, 5, NULL);
    return ESP_OK;
}

static esp_err_t set_timer_post_handler(httpd_req_t *req)
{
    if (s_cfg == NULL) {
        return http_send_text(req, "cfg not set");
    }

    char body[128];
    if (http_read_body(req, body, sizeof(body)) < 0) {
        return ESP_FAIL;
    }

    char timer_value[16] = {0};
    if (!form_get(body, "telemetry_interval", timer_value, sizeof(timer_value))) {
        return http_send_text(req, "telemetry_interval missing");
    }

    int value = atoi(timer_value);
    if (value <= 0) {
        return http_send_text(req, "invalid telemetry_interval");
    }

    s_cfg->telemetry_interval_s = (uint32_t)value;
    cfg_set_active(s_cfg);

    if (!cfg_save(s_cfg)) {
        return http_send_text(req, "Error guardando timer");
    }

    /*
     * Aquí sí conviene aplicar en caliente.
     * Si scheduler no está arrancado aún, scheduler_reload() debe degradar bien.
     */
    /*if (!scheduler_reload()) {
        ESP_LOGW(TAG, "scheduler_reload failed or not started");
    }*/

    return http_send_text(req, "Timer OK");
}

static esp_err_t update_firmware_post_handler(httpd_req_t *req)
{
    char body[256];
    if (http_read_body(req, body, sizeof(body)) < 0) {
        return ESP_FAIL;
    }

    char url[128] = {0};
    if (!form_get(body, "firmware_update", url, sizeof(url))) {
        return http_send_text(req, "firmware_update missing");
    }

    app_cmd_ota_t cmd = {0};
    strlcpy(cmd.url, url, sizeof(cmd.url));

    app_post_command_event(APP_CMD_START_OTA, &cmd, sizeof(cmd));
    return http_send_text(req, "Actualizacion iniciada");
}

static esp_err_t output_post_handler(httpd_req_t *req)
{
    char body[64];
    if (http_read_body(req, body, sizeof(body)) < 0) {
        return ESP_FAIL;
    }

    char state[16] = {0};
    if (!form_get(body, "output", state, sizeof(state))) {
        return http_send_text(req, "output missing");
    }

    int raw = atoi(state);

    app_cmd_output_t cmd = {
        .output_id = 1,
        .value = (raw != 0)
    };

    app_post_command_event(APP_CMD_SET_OUTPUT, &cmd, sizeof(cmd));
    return http_send_text(req, "OK");
}

static esp_err_t login_validate(httpd_req_t *req){
    if (!s_cfg) {
        http_send_text(req, "cfg not set");
        return ESP_OK;
    }

    if (s_cfg->login_user[0] == '\0' || s_cfg->login_pass[0] == '\0') {
        http_send_text(req, "login credentials not configured");
        return ESP_OK;
    }

    char body[512];
    int len = http_read_body(req, body, sizeof(body));
    if (len < 0) {
        http_send_text(req, "bad body");
        return ESP_OK;
    }

    char login_user[CFG_USER] = {0};
    char login_pass[CFG_PASS] = {0};

    form_get(body, "login_user", login_user, sizeof(login_user));
    form_get(body, "login_pass", login_pass, sizeof(login_pass));

    if (strcmp(login_user, s_cfg->login_user) == 0 &&
        strcmp(login_pass, s_cfg->login_pass) == 0) {

        session_create();

        char cookie[160];
        snprintf(cookie, sizeof(cookie),
                 SESSION_COOKIE_NAME "=%s; Path=/; HttpOnly; SameSite=Lax",
                 s_session_id);

        httpd_resp_set_hdr(req, "Set-Cookie", cookie);
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_set_hdr(req, "Cache-Control", "no-store");
        httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    httpd_resp_set_status(req, "401 Unauthorized");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_send(req, "Invalid User or Password", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t logout_post(httpd_req_t *req)
{
    session_clear();
    httpd_resp_set_hdr(req, "Set-Cookie",
                       SESSION_COOKIE_NAME "=; Path=/; Max-Age=0; HttpOnly; SameSite=Lax");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t save_location(httpd_req_t *req){
    if (!require_auth_json_or_401(req)) {
        return ESP_OK;
    }
    if (!s_cfg) {
        http_send_text(req, "cfg not set");
        return ESP_OK;
    }

    char body[512];
    int len = http_read_body(req, body, sizeof(body));
    if (len < 0) {
        http_send_text(req, "bad body");
        return ESP_OK;
    }
    char latitude[20] = {0};
    char longitude[20] = {0};
    form_get(body, "latitude", latitude, sizeof(latitude));
    form_get(body, "longitude",longitude, sizeof(longitude));

    strlcpy(s_cfg->latitude, latitude, sizeof(s_cfg->latitude));
    strlcpy(s_cfg->longitude, longitude, sizeof(s_cfg->longitude));
    
    
    app_state_set_latitude(s_cfg->latitude);
    app_state_set_longitude(s_cfg->longitude);

    

    if (!cfg_save(s_cfg)) {
        http_send_text(req, "No se pudo guardar en NVS");
        return ESP_OK;
    }
    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


void web_cfg_handlers_register(httpd_handle_t server)
{
    httpd_uri_t uris[] = {
        {.uri = "/scan",      .method = HTTP_GET,  .handler = scan_get_handler,            .user_ctx = NULL},
        {.uri = "/save",      .method = HTTP_POST, .handler = save_post_handler,           .user_ctx = NULL},
        {.uri = "/set_timer", .method = HTTP_POST, .handler = set_timer_post_handler,      .user_ctx = NULL},
        {.uri = "/update",    .method = HTTP_POST, .handler = update_firmware_post_handler,.user_ctx = NULL},
        {.uri = "/output",    .method = HTTP_POST, .handler = output_post_handler,         .user_ctx = NULL},
        {.uri = "/login",     .method = HTTP_POST, .handler = login_validate,              .user_ctx = NULL},
        {.uri = "/logout",    .method = HTTP_POST, .handler = logout_post,                 .user_ctx = NULL},
        {.uri = "/location",  .method = HTTP_POST, .handler = save_location,               .user_ctx = NULL},
        {.uri = "/cfg",       .method = HTTP_GET,  .handler = cfg_get_handler,             .user_ctx = NULL},
        {.uri = "/login",     .method = HTTP_POST, .handler = login_validate,              .user_ctx = NULL},
        {.uri = "/logout",    .method = HTTP_POST, .handler = logout_post,                 .user_ctx = NULL}
    };

    for (size_t i = 0; i < sizeof(uris) / sizeof(uris[0]); i++) {
        httpd_register_uri_handler(server, &uris[i]);
    }
}