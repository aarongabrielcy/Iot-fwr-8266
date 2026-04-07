idf_component_register(
    SRCS "uart_drv.c" "i2c_drv.c" "adc_drv.c" "io_drv.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_adc common
)
