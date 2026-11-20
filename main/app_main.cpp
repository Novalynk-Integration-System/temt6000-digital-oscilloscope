#include "controller/oscilloscope_controller.hpp"

#include "esp_log.h"

namespace {
OscilloscopeController controller;
}

extern "C" void app_main()
{
    ESP_ERROR_CHECK(controller.init());
    ESP_ERROR_CHECK(controller.start());
}
