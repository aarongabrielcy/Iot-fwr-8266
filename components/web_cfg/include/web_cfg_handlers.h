#pragma once

#include "esp_http_server.h"
#include "cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

void web_cfg_handlers_set_cfg(app_cfg_t *cfg);
void web_cfg_handlers_register(httpd_handle_t server);

#ifdef __cplusplus
}
#endif