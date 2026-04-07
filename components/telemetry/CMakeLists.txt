idf_component_register(
    SRCS "telemetry_builder.c" "telemetry_service.c"
    INCLUDE_DIRS "include"
    REQUIRES driver app_core mqtt_mgr time_sync wifi_mgr
)
