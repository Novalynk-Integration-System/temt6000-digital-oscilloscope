#include "driver/temt6000_adc_driver.hpp"

#include <algorithm>

#include "config/app_config.hpp"
#include "esp_check.h"
#include "soc/soc_caps.h"

esp_err_t Temt6000AdcDriver::init()
{
    const adc_continuous_handle_cfg_t handle_config = {
        .max_store_buf_size = app_config::kAdcDmaFrameBytes * 16,
        .conv_frame_size = app_config::kAdcDmaFrameBytes,
        .flags = {.flush_pool = 0},
    };
    ESP_RETURN_ON_ERROR(adc_continuous_new_handle(&handle_config, &adc_handle_),
                        "Temt6000Adc", "create continuous ADC");

    adc_digi_pattern_config_t pattern = {
        .atten = app_config::kAdcAttenuation,
        .channel = static_cast<uint8_t>(app_config::kAdcChannel),
        .unit = static_cast<uint8_t>(app_config::kAdcUnit),
        .bit_width = SOC_ADC_DIGI_MAX_BITWIDTH,
    };
    const adc_continuous_config_t adc_config = {
        .pattern_num = 1,
        .adc_pattern = &pattern,
        .sample_freq_hz = app_config::kSampleRateHz,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
    };
    return adc_continuous_config(adc_handle_, &adc_config);
}

esp_err_t Temt6000AdcDriver::start()
{
    return adc_continuous_start(adc_handle_);
}

esp_err_t Temt6000AdcDriver::read(uint16_t* samples, size_t capacity, size_t& sample_count)
{
    static uint8_t dma_data[app_config::kAdcDmaFrameBytes];
    uint32_t bytes_read = 0;
    ESP_RETURN_ON_ERROR(
        adc_continuous_read(adc_handle_, dma_data, sizeof(dma_data), &bytes_read, 1000),
        "Temt6000Adc", "continuous ADC read");

    static adc_continuous_data_t parsed[app_config::kSamplesPerBlock];
    uint32_t parsed_count = 0;
    ESP_RETURN_ON_ERROR(
        adc_continuous_parse_data(adc_handle_, dma_data, bytes_read, parsed, &parsed_count),
        "Temt6000Adc", "parse ADC frame");

    sample_count = 0;
    const size_t limit = std::min(capacity, static_cast<size_t>(parsed_count));
    for (size_t index = 0; index < limit; ++index) {
        if (parsed[index].valid && parsed[index].channel == app_config::kAdcChannel) {
            samples[sample_count++] = static_cast<uint16_t>(parsed[index].raw_data);
        }
    }
    return ESP_OK;
}
