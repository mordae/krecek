add_library(krecek sdk.c dap.c)
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
  hardware_pio
  hardware_pwm
  hardware_timer
  hardware_vreg
)
