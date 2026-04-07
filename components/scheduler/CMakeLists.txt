idf_component_register(
    SRCS "scheduler.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_timer app_core 
)
