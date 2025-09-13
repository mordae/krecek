add_library(
  krecek
  main.c
  save.c
  audio.c
  melody.c
  melody-lexer.c
  scene.c
  video.c
  input.c
  slave.c
  remote.c
  dap.c
  nau88c22.c
  comms.c
  sdcard_task.c

  # card & filesystem
  sdcard.c
  fatfs/diskio.c
  fatfs/ff.c
  fatfs/ffsystem.c
  fatfs/ffunicode.c
)

pico_generate_pio_header(krecek ${CMAKE_CURRENT_LIST_DIR}/i2s.pio)
target_include_directories(krecek PUBLIC include)
generate_png_headers()
target_link_libraries(krecek ${PNG_HEADERS_TARGET})
krecek_set_target_options(krecek)

target_link_libraries(
  krecek

  # src/sdk/slave/
  slave_packed_lib

  # vendor/
  pico_stdio_usb_simple
  pico_task
  pico_tft

  # pico-sdk/
  pico_divider
  pico_multicore
  pico_stdlib
  pico_util
  pico_rand

  hardware_adc
  hardware_divider
  hardware_dma
  hardware_i2c
  hardware_pio
  hardware_pwm
  hardware_timer
  hardware_vreg
)
