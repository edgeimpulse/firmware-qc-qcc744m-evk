cmake_minimum_required(VERSION 3.20.0)

RECURSIVE_FIND_FILE_APPEND(MODEL_SOURCE "tflite-model" "*.cpp")
zephyr_include_directories(.)

# add all sources to the project
zephyr_library_sources(${MODEL_SOURCE})

