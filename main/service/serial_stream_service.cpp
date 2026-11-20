#include "service/serial_stream_service.hpp"

#include <cstring>

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
    static SensorSampleBlock blocks[app_config::kSerialBatchBlocks]{};
    static uint8_t output[app_config::kSerialBatchBlocks * sizeof(SensorSampleBlock)];

    while (true) {
        size_t block_count = 0;
        if (xQueueReceive(queue_, &blocks[block_count], portMAX_DELAY) == pdTRUE) {
            ++block_count;
        }
        while (block_count < app_config::kSerialBatchBlocks &&
               xQueueReceive(queue_, &blocks[block_count], 0) == pdTRUE) {
            ++block_count;
        }

        size_t output_length = 0;
        for (size_t index = 0; index < block_count; ++index) {
            const size_t block_length = sizeof(SensorSampleBlock) -
                (app_config::kSamplesPerBlock - blocks[index].sample_count) * sizeof(uint16_t);
            std::memcpy(output + output_length, &blocks[index], block_length);
            output_length += block_length;
        }
        if (output_length > 0) {
            usb_serial_jtag_write_bytes(output, output_length, portMAX_DELAY);
        }
    }
}
