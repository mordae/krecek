add_executable(pong main.c)
target_include_directories(pong PRIVATE include)
generate_png_headers()
target_link_libraries(pong krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(pong)
krecek_set_target_options(pong)

if (ENABLE_OFFSETS)
  pico_set_linker_script(pong ${CMAKE_CURRENT_LIST_DIR}/memmap.ld)
endif()

#pico_set_binary_type(pong no_flash)
#pico_set_binary_type(pong copy_to_ram)
