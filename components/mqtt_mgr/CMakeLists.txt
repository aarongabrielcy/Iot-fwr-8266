idf_component_register(
    SRCS "mqtt_topics.c" "mqtt_mgr.c"
    INCLUDE_DIRS "include"
    REQUIRES mqtt cfg app_core
)
