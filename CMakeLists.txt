cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake) 

project(IN12B-8DIGIT)

pico_sdk_init()

add_executable(in12b_8digit
    in12b_8digit.c
)

target_link_libraries(in12b_8digit
    pico_stdlib
    pico_multicore
    hardware_uart)

pico_enable_stdio_usb(in12b_8digit 1)
pico_enable_stdio_uart(in12b_8digit 0)

pico_add_extra_outputs(in12b_8digit)