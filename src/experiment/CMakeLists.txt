add_executable(experiment main.c)
target_include_directories(experiment PRIVATE include)
generate_png_headers()
generate_bin_headers()
target_link_libraries(experiment krecek ${PNG_HEADERS_TARGET} ${BIN_HEADERS_TARGET})
pico_add_extra_outputs(experiment)
krecek_set_target_options(experiment)

if (ENABLE_OFFSETS)
  pico_set_linker_script(experiment ${CMAKE_CURRENT_LIST_DIR}/memmap.ld)
endif()

#pico_set_binary_type(experiment no_flash)
#pico_set_binary_type(experiment copy_to_ram)
