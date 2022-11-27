cmake_minimum_required(VERSION 3.13)
set(ENV{PICO_SDK_PATH} "~/pico/pico-sdk/")

include(pico_sdk_import.cmake)

project(i2cTest C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(BallBal
    BallBal.cpp
)


pico_enable_stdio_usb(BallBal 1)
pico_enable_stdio_uart(BallBal 0)

pico_add_extra_outputs(BallBal)
target_link_libraries(BallBal pico_stdlib hardware_i2c hardware_pwm hardware_timer hardware_irq hardware_uart)
