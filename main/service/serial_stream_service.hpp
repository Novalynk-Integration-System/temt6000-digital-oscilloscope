#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

class SerialStreamService {
public:
    explicit SerialStreamService(QueueHandle_t queue) : queue_(queue) {}
    esp_err_t start();

private:
    static void task_entry(void* context);
    void run();

    QueueHandle_t queue_;
};
