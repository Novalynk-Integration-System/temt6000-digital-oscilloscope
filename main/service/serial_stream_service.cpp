#include "service/serial_stream_service.hpp"

#include <inttypes.h>
#include <cstdio>

#include "common/sensor_sample.hpp"
#include "config/app_config.hpp"
#include "driver/uart.h"
#include "esp_check.h"
#include "freertos/task.h"

esp_err_t SerialStreamService::start()
{
    const uart_config_t uart_config = {
        .baud_rate = app_config::kSerialBaudRate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT,
        .flags = {},
    };
    ESP_RETURN_ON_ERROR(uart_param_config(app_config::kSerialUart, &uart_config),
                        "SerialStream", "configure UART");
    ESP_RETURN_ON_ERROR(
        uart_driver_install(app_config::kSerialUart, app_config::kSerialRxBufferSize,
                            app_config::kSerialTxBufferSize, 0, nullptr, 0),
        "SerialStream", "install UART driver");

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
            uart_write_bytes(app_config::kSerialUart, output, output_length);
        }
    }
}
