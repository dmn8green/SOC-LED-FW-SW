# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS 
    "${CMAKE_CURRENT_LIST_DIR}/components/esp-aws-iot/libraries/"
    "${CMAKE_CURRENT_LIST_DIR}/components/ArduinoJson/"
)


include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# include_directories(BEFORE "${CMAKE_CURRENT_LIST_DIR}/main")

project(led_strip)

