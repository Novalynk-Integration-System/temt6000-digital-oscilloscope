#include "service/serial_stream_service.hpp"

#include <inttypes.h>
#include <cstdio>

#include "common/sensor_sample.hpp"
#include "config/app_config.hpp"
#include "driver/usb_serial_jtag.h"
#include "esp_check.h"
#include "freertos/task.h"

esp_err_t SerialStreamService::start()
{
    usb_serial_jtag_driver_config_t usb_config = {
        .tx_buffer_size = app_config::kUsbSerialTxBufferSize,
        .rx_buffer_size = app_config::kUsbSerialRxBufferSize,
    };
    ESP_RETURN_ON_ERROR(usb_serial_jtag_driver_install(&usb_config),
                        "SerialStream", "install USB Serial/JTAG driver");

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
    constexpr size_t kMaxLineLength = 80;
    // These buffers are intentionally static. Keeping a multi-kilobyte batch on the
    // 4 KiB FreeRTOS task stack leaves too little room for snprintf's own stack use.
    static SensorSample samples[app_config::kSerialBatchSamples]{};
    static char output[app_config::kSerialBatchSamples * kMaxLineLength];

    while (true) {
        size_t sample_count = 0;
        if (xQueueReceive(queue_, &samples[sample_count], portMAX_DELAY) == pdTRUE) {
            ++sample_count;
        }
        while (sample_count < app_config::kSerialBatchSamples &&
               xQueueReceive(queue_, &samples[sample_count], 0) == pdTRUE) {
            ++sample_count;
        }

        size_t output_length = 0;
        for (size_t index = 0; index < sample_count; ++index) {
            const SensorSample& sample = samples[index];
            const int length = std::snprintf(
                output + output_length, sizeof(output) - output_length,
                "D,%" PRIu32 ",%" PRId64 ",%" PRId32 ",%" PRId32 "\n",
                sample.sequence, sample.timestamp_us, sample.raw, sample.millivolts);
            if (length > 0) {
                output_length += static_cast<size_t>(length);
            }
        }
        if (output_length > 0) {
            usb_serial_jtag_write_bytes(output, output_length, portMAX_DELAY);
        }
    }
}
