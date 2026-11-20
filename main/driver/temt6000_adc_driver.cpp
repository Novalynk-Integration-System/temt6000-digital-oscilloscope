#include "driver/temt6000_adc_driver.hpp"

#include "config/app_config.hpp"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_check.h"

esp_err_t Temt6000AdcDriver::init()
{
    const adc_oneshot_unit_init_cfg_t unit_config = {
        .unit_id = app_config::kAdcUnit,
        .clk_src = ADC_DIGI_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_RETURN_ON_ERROR(adc_oneshot_new_unit(&unit_config, &adc_handle_),
                        "Temt6000Adc", "create ADC unit");

    const adc_oneshot_chan_cfg_t channel_config = {
        .atten = app_config::kAdcAttenuation,
        .bitwidth = app_config::kAdcBitWidth,
    };
    ESP_RETURN_ON_ERROR(
        adc_oneshot_config_channel(adc_handle_, app_config::kAdcChannel, &channel_config),
        "Temt6000Adc", "configure ADC channel");

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    const adc_cali_curve_fitting_config_t calibration_config = {
        .unit_id = app_config::kAdcUnit,
        .chan = app_config::kAdcChannel,
        .atten = app_config::kAdcAttenuation,
        .bitwidth = app_config::kAdcBitWidth,
    };
    // Raw samples remain usable if eFuse calibration data is not available.
    adc_cali_create_scheme_curve_fitting(&calibration_config, &calibration_handle_);
#endif
    return ESP_OK;
}

esp_err_t Temt6000AdcDriver::read(int& raw, int& millivolts)
{
    ESP_RETURN_ON_ERROR(adc_oneshot_read(adc_handle_, app_config::kAdcChannel, &raw),
                        "Temt6000Adc", "ADC read");
    millivolts = -1;
    if (calibration_handle_ != nullptr) {
        adc_cali_raw_to_voltage(calibration_handle_, raw, &millivolts);
    }
    return ESP_OK;
}
