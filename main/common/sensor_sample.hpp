#pragma once

#include <cstdint>

#include "config/app_config.hpp"

constexpr uint32_t kSampleBlockMagic = 0x31544D54; // ASCII "TMT1", little-endian
constexpr uint16_t kSampleBlockVersion = 1;

struct __attribute__((packed)) SensorSampleBlock {
    uint32_t magic;
    uint16_t version;
    uint16_t sample_count;
    uint32_t sequence_start;
    int64_t first_timestamp_us;
    uint32_t sample_rate_hz;
    uint16_t raw[app_config::kSamplesPerBlock];
};
