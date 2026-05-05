#include "app_handlers.h"
#include "system_hdl.h"
#include "report_hdl.h"
#include "sensors_hdl.h"
#include "command_hdl.h"


void app_handlers_init(const app_cfg_t *cfg) {
    system_hdl_init(cfg);
    report_hdl_init();
    sensor_hdl_init();
    command_hdl_init();
}