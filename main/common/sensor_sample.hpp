#pragma once

#include <cstdint>

struct SensorSample {
    uint32_t sequence;
    int64_t timestamp_us;
    int32_t raw;
    int32_t millivolts; // -1 when ADC calibration is unavailable
};
