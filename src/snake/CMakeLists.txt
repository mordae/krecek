add_executable(snake main.c)
target_include_directories(snake PRIVATE include)
generate_png_headers()
target_link_libraries(snake krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(snake)
krecek_set_target_options(snake)

if (ENABLE_OFFSETS)
  pico_set_linker_script(snake ${CMAKE_CURRENT_LIST_DIR}/memmap.ld)
endif()

#pico_set_binary_type(snake no_flash)
#pico_set_binary_type(snake copy_to_ram)
