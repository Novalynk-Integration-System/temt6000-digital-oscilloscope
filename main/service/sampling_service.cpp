#include "service/sampling_service.hpp"

#include <cstring>

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
    if (sensor_.start() != ESP_OK) {
        vTaskDelete(nullptr);
        return;
    }

    while (true) {
        SensorSampleBlock block = {
            .magic = kSampleBlockMagic,
            .version = kSampleBlockVersion,
            .sample_count = 0,
            .sequence_start = sequence,
            .first_timestamp_us = 0,
            .sample_rate_hz = app_config::kSampleRateHz,
            .raw = {},
        };
        uint16_t raw[app_config::kSamplesPerBlock]{};
        size_t sample_count = 0;
        if (sensor_.read(raw, app_config::kSamplesPerBlock, sample_count) == ESP_OK &&
            sample_count > 0) {
            std::memcpy(block.raw, raw, sample_count * sizeof(uint16_t));
            block.sample_count = static_cast<uint16_t>(sample_count);
            const int64_t block_end_us = esp_timer_get_time();
            block.first_timestamp_us = block_end_us -
                static_cast<int64_t>((sample_count - 1) * 1000000ULL /
                                     app_config::kSampleRateHz);
            sequence += static_cast<uint32_t>(sample_count);
            xQueueSend(queue_, &block, portMAX_DELAY);
        }
    }
}
