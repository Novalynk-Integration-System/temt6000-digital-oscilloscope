#pragma once

#include <cstddef>

#include "esp_adc/adc_oneshot.h"

namespace app_config {

// ESP32-C6 ADC1_CH0 is GPIO0. Change both fields when moving the signal pin.
constexpr adc_unit_t kAdcUnit = ADC_UNIT_1;
constexpr adc_channel_t kAdcChannel = ADC_CHANNEL_0;
constexpr adc_atten_t kAdcAttenuation = ADC_ATTEN_DB_12;
constexpr adc_bitwidth_t kAdcBitWidth = ADC_BITWIDTH_DEFAULT;

constexpr uint32_t kSampleRateHz = 20000;
constexpr size_t kSamplesPerBlock = 64;
constexpr size_t kAdcDmaFrameBytes = kSamplesPerBlock * 4;
constexpr uint32_t kQueueLength = 64; // blocks, about 205 ms of buffering
constexpr size_t kSerialBatchBlocks = 8;
constexpr size_t kUsbSerialRxBufferSize = 256;
constexpr size_t kUsbSerialTxBufferSize = 4096;

constexpr int kSamplingTaskPriority = 12;
constexpr int kSerialTaskPriority = 10;
constexpr uint32_t kSamplingTaskStack = 4096;
constexpr uint32_t kSerialTaskStack = 4096;

} // namespace app_config
