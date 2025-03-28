cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(IN12B-8DIGIT)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project

add_executable(in12b_8digit
    in12b_8digit.c
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(in12b_8digit
    pico_stdlib
    pico_multicore
    hardware_uart)

# enable usb output, disable uart output
#pico_enable_stdio_usb(in12b_8digit 0)
#pico_enable_stdio_uart(in12b_8digit 1)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(in12b_8digit)