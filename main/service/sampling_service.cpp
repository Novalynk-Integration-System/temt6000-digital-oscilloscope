#include "service/sampling_service.hpp"

#include "common/sensor_sample.hpp"
#include "config/app_config.hpp"
#include "esp_timer.h"
#include "freertos/task.h"

esp_err_t SamplingService::start()
{
    const BaseType_t result = xTaskCreate(task_entry, "sensor_sampling",
                                          app_config::kSamplingTaskStack, this,
                                          app_config::kSamplingTaskPriority, nullptr);
    return result == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}

void SamplingService::task_entry(void* context)
{
    static_cast<SamplingService*>(context)->run();
}

void SamplingService::run()
{
    uint32_t sequence = 0;
    const TickType_t sample_period = pdMS_TO_TICKS(1);
    TickType_t last_wake = xTaskGetTickCount();

    while (true) {
        // Block until the next 1 ms tick. Unlike a busy-wait, this leaves CPU time
        // available for the lower-priority serial consumer on the single-core C6.
        const TickType_t now = xTaskGetTickCount();
        if ((now - last_wake) > sample_period) {
            // Do not emit a catch-up burst after queue backpressure or a long stall.
            last_wake = now;
        }
        xTaskDelayUntil(&last_wake, sample_period);

        SensorSample sample = {
            .sequence = sequence++,
            .timestamp_us = esp_timer_get_time(),
            .raw = 0,
            .millivolts = -1,
        };
        int raw = 0;
        int millivolts = -1;
        if (sensor_.read(raw, millivolts) == ESP_OK) {
            sample.raw = static_cast<int32_t>(raw);
            sample.millivolts = static_cast<int32_t>(millivolts);
            // Blocking send provides backpressure: a sample is never silently discarded.
            xQueueSend(queue_, &sample, portMAX_DELAY);
        }
    }
}
