#pragma once

#include "esp_adc/adc_oneshot.h"

namespace app_config {

// ESP32-C6 ADC1_CH0 is GPIO0. Change both fields when moving the signal pin.
constexpr adc_unit_t kAdcUnit = ADC_UNIT_1;
constexpr adc_channel_t kAdcChannel = ADC_CHANNEL_0;
constexpr adc_atten_t kAdcAttenuation = ADC_ATTEN_DB_12;
constexpr adc_bitwidth_t kAdcBitWidth = ADC_BITWIDTH_DEFAULT;

constexpr uint32_t kSamplePeriodUs = 1000; // 1 kHz
constexpr uint32_t kQueueLength = 4096;   // about 4 seconds of buffering

constexpr int kSamplingTaskPriority = 12;
constexpr int kSerialTaskPriority = 10;
constexpr uint32_t kSamplingTaskStack = 4096;
constexpr uint32_t kSerialTaskStack = 4096;

} // namespace app_config
