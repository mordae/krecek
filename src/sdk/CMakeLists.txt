add_library(krecek main.c dap.c es8312.c)
pico_generate_pio_header(krecek ${CMAKE_CURRENT_LIST_DIR}/i2s.pio)
target_include_directories(krecek PUBLIC include)
krecek_set_target_options(krecek)

target_link_libraries(
  krecek

  # vendor/
  pico_stdio_usb_simple
  pico_task
  pico_tft

  # pico-sdk/
  pico_divider
  pico_multicore
  pico_stdlib
  pico_util

  hardware_adc
  hardware_divider
  hardware_dma
  hardware_i2c
  hardware_pio
  hardware_pwm
  hardware_timer
  hardware_vreg
)
