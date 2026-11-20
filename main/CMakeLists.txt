idf_component_register(
    SRCS
        "app_main.cpp"
        "controller/oscilloscope_controller.cpp"
        "driver/temt6000_adc_driver.cpp"
        "service/sampling_service.cpp"
        "service/serial_stream_service.cpp"
    INCLUDE_DIRS "."
    REQUIRES esp_adc esp_driver_usb_serial_jtag esp_timer
)
