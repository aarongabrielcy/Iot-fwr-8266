idf_component_register(
    SRCS "serial_monitor.c"
    INCLUDE_DIRS "include"
    REQUIRES driver app_core drivers common ota
)
