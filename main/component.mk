#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

COMPONENT_SRCDIRS := .
#COMPONENT_DEPENDS := esp_event cfg  mqtt_mgr wifi_mgr web_cfg app_core drivers serial_monitor 
COMPONENT_DEPENDS := \
    cfg \
    wifi_mgr \
    web_cfg \
    mqtt_mgr \
    scheduler \
    app_core \
    app_handlers \
    app_services \
    telemetry \
    sensors \
    drivers \
    serial_monitor \
    esp_event