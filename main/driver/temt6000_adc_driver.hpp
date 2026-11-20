#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_adc/adc_continuous.h"
#include "esp_err.h"

class Temt6000AdcDriver {
public:
    esp_err_t init();
    esp_err_t start();
    esp_err_t read(uint16_t* samples, size_t capacity, size_t& sample_count);

private:
    adc_continuous_handle_t adc_handle_ = nullptr;
};
