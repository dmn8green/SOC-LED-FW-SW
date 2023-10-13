

cmake_minimum_required(VERSION 3.6)
project(led-test)

include_directories(../LED ../LED/Animations ../)
include_directories(mock)

set(SOURCE_FILES ../LED/Animations/ChargeIndicator.cpp ../LED/Animations/ProgressAnimation.cpp ../LED/Animations/ChargingAnimation.cpp ../LED/Animations/StaticAnimation.cpp tests.cpp)

add_compile_options (-DUNIT_TEST -g)
add_executable(led-test ${SOURCE_FILES})
target_link_libraries (led-test gtest pthread)

