#pragma once

#include "driver/temt6000_adc_driver.hpp"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

class SamplingService;
class SerialStreamService;

class OscilloscopeController {
public:
    esp_err_t init();
    esp_err_t start();

private:
    Temt6000AdcDriver sensor_;
    QueueHandle_t queue_ = nullptr;
    SamplingService* sampling_ = nullptr;
    SerialStreamService* serial_ = nullptr;
};
