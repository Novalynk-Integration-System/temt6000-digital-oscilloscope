cmake_minimum_required(VERSION 3.16)

# Keep this sensor-only firmware small and make clean builds much faster.
set(COMPONENTS main)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(temt6000_digital_oscilloscope)
