add_executable(slave main.c stdio.c cc1101.c)

target_link_libraries(
  slave

  pico_task

  pico_divider
  pico_stdlib
  hardware_adc
  hardware_dma
  hardware_i2c
  hardware_spi
  hardware_pio
  hardware_pwm
  hardware_timer
  hardware_divider
)

krecek_set_target_options(slave)
pico_add_extra_outputs(slave)
pico_set_binary_type(slave no_flash)
pico_set_linker_script(slave "${CMAKE_CURRENT_LIST_DIR}/memmap_no_flash.ld")

set(SLAVE_BIN_FILE "${CMAKE_CURRENT_BINARY_DIR}/slave.bin")
set(SLAVE_HEADER_FILE "${CMAKE_CURRENT_BINARY_DIR}/slave.bin.h")

find_package(Python3 REQUIRED)

set(CONVERTER_SCRIPT "${CMAKE_SOURCE_DIR}/../tools/bin_to_header.py")

add_custom_command(
  OUTPUT ${SLAVE_HEADER_FILE}
  COMMAND ${Python3_EXECUTABLE} ${CONVERTER_SCRIPT} -n slave_bin -o ${SLAVE_HEADER_FILE} ${SLAVE_BIN_FILE}
  DEPENDS slave ${CONVERTER_SCRIPT}
  COMMENT "Generating slave.bin.h from slave.bin"
  VERBATIM
)

add_custom_target(slave_packed DEPENDS ${SLAVE_HEADER_FILE})

add_library(slave_packed_lib INTERFACE)
add_dependencies(slave_packed_lib slave_packed)
target_include_directories(slave_packed_lib INTERFACE ${CMAKE_CURRENT_BINARY_DIR} include)
