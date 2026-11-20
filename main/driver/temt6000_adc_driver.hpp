#pragma once

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

class Temt6000AdcDriver {
public:
    esp_err_t init();
    esp_err_t read(int& raw, int& millivolts);

private:
    adc_oneshot_unit_handle_t adc_handle_ = nullptr;
    adc_cali_handle_t calibration_handle_ = nullptr;
};
