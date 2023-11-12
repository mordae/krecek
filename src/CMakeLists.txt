cmake_minimum_required(VERSION 3.21)

include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

project(krecek)
pico_sdk_init()

add_executable(krecek main.c)

add_subdirectory(vendor/pico-stdio-usb-simple)

set(TFT_DRIVER "ili9341")
add_subdirectory(vendor/pico-tft)

target_compile_definitions(
  pico_tft
  INTERFACE
    TFT_CS_PIN=1
    TFT_SCK_PIN=2
    TFT_MOSI_PIN=3
    TFT_RST_PIN=4
    TFT_RS_PIN=5
    TFT_SPI_DEV=spi0
    TFT_BAUDRATE=80000000
    TFT_SWAP_XY=1
    TFT_FLIP_X=1
    TFT_FLIP_Y=1
    TFT_SCALE=2
)

add_subdirectory(vendor/pico-task)
add_definitions(-I${CMAKE_CURRENT_LIST_DIR}/vendor/pico-task/include)
list(APPEND PICO_CONFIG_HEADER_FILES task_hooks.h)

target_compile_definitions(
  pico_task
  INTERFACE
    MAX_TASKS=4
    TASK_STACK_SIZE=2048
)

target_link_libraries(
  krecek
  pico_divider
  pico_multicore
  pico_stdio_usb_simple
  pico_stdlib
  pico_util
  pico_tft
  pico_task
  hardware_adc
  hardware_divider
  hardware_dma
  hardware_pio
  hardware_timer
  hardware_vreg
)

pico_add_extra_outputs(krecek)

set_property(TARGET krecek PROPERTY C_STANDARD 23)
target_compile_options(krecek PRIVATE -Wall -Wextra -Wnull-dereference)
target_compile_definitions(krecek PUBLIC PICO_MAX_SHARED_IRQ_HANDLERS=8u)
target_compile_definitions(krecek PUBLIC PICO_STDIO_ENABLE_CRLF_SUPPORT=1)
target_compile_definitions(krecek PUBLIC PICO_STDIO_DEFAULT_CRLF=1)

target_include_directories(krecek PRIVATE include)

#pico_set_binary_type(krecek no_flash)
pico_set_binary_type(krecek copy_to_ram)
