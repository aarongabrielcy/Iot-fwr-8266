#include "app_handlers.h"
#include "system_hdl.h"
#include "report_hdl.h"
#include "sensors_hdl.h"
#include "command_hdl.h"

void app_handlers_init(void) {
    system_hdl_init();
    report_hdl_init();
    sensor_hdl_init();
    command_hdl_init();
}