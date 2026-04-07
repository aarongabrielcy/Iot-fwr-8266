#include "web_cfg.h"

#include <string.h>
#include "esp_log.h"
#include "esp_http_server.h"

#include "web_cfg_handlers.h"

#ifndef HTTPD_RESP_USE_STRLEN
#define HTTPD_RESP_USE_STRLEN -1
#endif

static const char *TAG = "WEB_CFG";
static httpd_handle_t s_server = NULL;

/* HTML embebido */
extern const uint8_t web_cfg_ui_html_start[] asm("_binary_web_cfg_ui_html_start");
extern const uint8_t web_cfg_ui_html_end[]   asm("_binary_web_cfg_ui_html_end");

static esp_err_t root_get(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");

    const size_t len = (size_t)(web_cfg_ui_html_end - web_cfg_ui_html_start);
    httpd_resp_send(req, (const char *)web_cfg_ui_html_start, len);
    return ESP_OK;
}

static esp_err_t health_get(httpd_req_t *req)
{
    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

bool web_cfg_start(app_cfg_t *cfg)
{
    if (s_server != NULL) {
        web_cfg_handlers_set_cfg(cfg);
        return true;
    }

    web_cfg_handlers_set_cfg(cfg);

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.stack_size = 6144;
    config.max_uri_handlers = 16;

    esp_err_t err = httpd_start(&s_server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start httpd: %s", esp_err_to_name(err));
        s_server = NULL;
        return false;
    }

    httpd_uri_t root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_get,
        .user_ctx = NULL
    };

    httpd_uri_t health = {
        .uri = "/health",
        .method = HTTP_GET,
        .handler = health_get,
        .user_ctx = NULL
    };

    httpd_register_uri_handler(s_server, &root);
    httpd_register_uri_handler(s_server, &health);

    web_cfg_handlers_register(s_server);

    ESP_LOGI(TAG, "WebCfg started:");
    ESP_LOGI(TAG, "  GET  /         (UI)");
    ESP_LOGI(TAG, "  GET  /health   (OK)");
    ESP_LOGI(TAG, "  GET  /scan     (wifi scan)");
    ESP_LOGI(TAG, "  GET  /cfg      (show cfg)");
    ESP_LOGI(TAG, "  POST /save     (save wifi+mqtt)");
    ESP_LOGI(TAG, "  POST /set_timer");
    ESP_LOGI(TAG, "  POST /update");
    ESP_LOGI(TAG, "  POST /output");

    return true;
}

void web_cfg_stop(void)
{
    if (s_server != NULL) {
        httpd_stop(s_server);
        s_server = NULL;
    }

    web_cfg_handlers_set_cfg(NULL);
}