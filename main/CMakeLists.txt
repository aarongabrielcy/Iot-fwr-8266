idf_component_register(SRCS "app_runtime.c" "app_boot.c" "app_main.c"
                    INCLUDE_DIRS ""
                    REQUIRES 
                       esp_event
                       cfg 
                       mqtt_mgr
                       wifi_mgr
                       web_cfg
                       app_core
                       drivers
                       serial_monitor
                       )