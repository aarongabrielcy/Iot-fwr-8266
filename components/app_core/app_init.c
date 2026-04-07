#include "app_init.h"
#include "app_event_loop.h"
#include "app_state.h"

void app_core_init(void) {
    app_event_loop_init();
    app_state_init();
}