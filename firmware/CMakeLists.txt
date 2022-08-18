cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico_examples C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

# Initialize the SDK
pico_sdk_init()

include(example_auto_set_url.cmake)

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

add_executable(rp1_firmware
        rp1.c
        console.c
        uart.c
        menu.c
        usb_descriptors.c
        cdc.c
        msc_disk.c
        challs/chall1.c
        challs/chall2.c
        challs/chall3.c
        challs/chall4.c
        )

add_executable(rp2_firmware
        rp2.c
        uart.c
        nvs.c
        )


add_executable(rp_test
        rp_test.c
        )

target_include_directories(rp1_firmware PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# pull in common dependencies
target_link_libraries(rp1_firmware pico_stdlib tinyusb_device tinyusb_board)
target_link_libraries(rp2_firmware pico_stdlib hardware_flash)
target_link_libraries(rp_test pico_stdlib)


# enable usb output, disable uart output
# pico_enable_stdio_usb(rp1_firmware 1)
# pico_enable_stdio_uart(rp1_firmware 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(rp1_firmware)
pico_add_extra_outputs(rp2_firmware)
pico_add_extra_outputs(rp_test)

# add url via pico_set_program_url
example_auto_set_url(rp1_firmware)
example_auto_set_url(rp_test)

