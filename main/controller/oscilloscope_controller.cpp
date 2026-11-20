#include "controller/oscilloscope_controller.hpp"

#include <new>

#include "common/sensor_sample.hpp"
#include "config/app_config.hpp"
#include "esp_check.h"
#include "service/sampling_service.hpp"
#include "service/serial_stream_service.hpp"

esp_err_t OscilloscopeController::init()
{
    ESP_RETURN_ON_ERROR(sensor_.init(), "ScopeController", "sensor init");
    queue_ = xQueueCreate(app_config::kQueueLength, sizeof(SensorSampleBlock));
    if (queue_ == nullptr) {
        return ESP_ERR_NO_MEM;
    }
    sampling_ = new (std::nothrow) SamplingService(sensor_, queue_);
    serial_ = new (std::nothrow) SerialStreamService(queue_);
    return sampling_ != nullptr && serial_ != nullptr ? ESP_OK : ESP_ERR_NO_MEM;
}

esp_err_t OscilloscopeController::start()
{
    // Start the consumer before the producer.
    ESP_RETURN_ON_ERROR(serial_->start(), "ScopeController", "serial task");
    return sampling_->start();
}
