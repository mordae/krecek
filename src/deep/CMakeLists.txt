add_executable(deep main.c level.c)
target_include_directories(deep PRIVATE include)
generate_png_headers()
generate_bin_headers()
target_link_libraries(deep krecek ${PNG_HEADERS_TARGET} ${BIN_HEADERS_TARGET})
pico_add_extra_outputs(deep)
krecek_set_target_options(deep)

if (ENABLE_OFFSETS)
  pico_set_linker_script(deep ${CMAKE_CURRENT_LIST_DIR}/memmap.ld)
endif()

#pico_set_binary_type(deep no_flash)
#pico_set_binary_type(deep copy_to_ram)
