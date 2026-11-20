#pragma once

#include "driver/temt6000_adc_driver.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

class SamplingService {
public:
    SamplingService(Temt6000AdcDriver& sensor, QueueHandle_t queue)
        : sensor_(sensor), queue_(queue) {}

    esp_err_t start();

private:
    static void task_entry(void* context);
    void run();

    Temt6000AdcDriver& sensor_;
    QueueHandle_t queue_;
};
