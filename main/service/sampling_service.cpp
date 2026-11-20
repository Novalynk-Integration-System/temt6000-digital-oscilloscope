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
    int64_t next_sample_us = esp_timer_get_time();

    while (true) {
        next_sample_us += app_config::kSamplePeriodUs;
        const int64_t remaining_us = next_sample_us - esp_timer_get_time();
        if (remaining_us >= 1000) {
            vTaskDelay(pdMS_TO_TICKS(static_cast<uint32_t>(remaining_us / 1000)));
        }
        while (esp_timer_get_time() < next_sample_us) {
            taskYIELD();
        }

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

        // Recover cleanly if execution was delayed by more than one period.
        const int64_t now_us = esp_timer_get_time();
        if (now_us - next_sample_us > static_cast<int64_t>(app_config::kSamplePeriodUs)) {
            next_sample_us = now_us;
        }
    }
}
