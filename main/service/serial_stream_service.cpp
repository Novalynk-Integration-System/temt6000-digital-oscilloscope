#include "service/serial_stream_service.hpp"

#include <cstdio>
#include <inttypes.h>

#include "common/sensor_sample.hpp"
#include "config/app_config.hpp"
#include "freertos/task.h"

esp_err_t SerialStreamService::start()
{
    const BaseType_t result = xTaskCreate(task_entry, "serial_stream",
                                          app_config::kSerialTaskStack, this,
                                          app_config::kSerialTaskPriority, nullptr);
    return result == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}

void SerialStreamService::task_entry(void* context)
{
    static_cast<SerialStreamService*>(context)->run();
}

void SerialStreamService::run()
{
    // One line is one atomic application frame:
    // D,<sequence>,<timestamp_us>,<raw>,<millivolts>
    SensorSample sample{};
    char line[80];
    while (true) {
        if (xQueueReceive(queue_, &sample, portMAX_DELAY) == pdTRUE) {
            const int length = std::snprintf(
                line, sizeof(line), "D,%" PRIu32 ",%" PRId64 ",%" PRId32 ",%" PRId32 "\n",
                sample.sequence, sample.timestamp_us, sample.raw, sample.millivolts);
            if (length > 0) {
                std::fwrite(line, 1, static_cast<size_t>(length), stdout);
            }
        }
    }
}
