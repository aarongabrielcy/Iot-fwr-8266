idf_component_register(
    SRCS "app_init.c" "app_core_loop.c" "app_state.c" "app_events.c" "event_hdl.c" "system_hdl.c" "sensors_hdl.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_event sensors scheduler ota telemetry time_sync
)
