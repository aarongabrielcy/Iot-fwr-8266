idf_component_register(
    SRCS "time_sync.c"
    INCLUDE_DIRS "include"
    REQUIRES driver lwip esp_timer app_core
)
