cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/model)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(firwmare-qc-qcc744m-evk)

include(utils/cmake/utils.cmake)

# Needed for colorful output
zephyr_compile_options(-fdiagnostics-color=always)

# Project include directories
set(INCLUDES
    .
    src
    )
include_directories(${INCLUDES})

# --- DSP / inference performance -------------------------------------------
if(TARGET edge-impulse-sdk)
    get_target_property(_ei_srcs edge-impulse-sdk SOURCES)
    get_target_property(_ei_dir  edge-impulse-sdk SOURCE_DIR)
    set_source_files_properties(
        ${_ei_srcs}
        DIRECTORY ${_ei_dir}
        PROPERTIES COMPILE_OPTIONS
        # -fsingle-precision-constant: unsuffixed FP literals (2.0, 0.5, M_PI)
        # stay 'float' instead of promoting neighbouring math to soft-float double
        # (no hardware double on the E907 / -mabi=ilp32f).
        "-O3;-funroll-loops;-fno-math-errno;-ffp-contract=fast;-ffast-math;-fsingle-precision-constant")
endif()

# add all sources to the project
target_sources(app PRIVATE src/main.cpp src/ei_loop.cpp)
target_sources(app PRIVATE src/sensors/ei_inertial.cpp)
target_sources(app PRIVATE src/sensors/ei_adc.cpp)
target_sources(app PRIVATE src/sensors/ei_pixels.cpp)
# Sensor-specific inference; each file compiles only for its EI_CLASSIFIER_SENSOR
target_sources(app PRIVATE src/inference/imu_inferencing.cpp)
target_sources(app PRIVATE src/data-fwd/data_fw.cpp)
target_sources(app PRIVATE src/msg_handler/msg_handler.cpp)
