add_executable(tecka main.c)
target_include_directories(tecka PRIVATE include)
generate_png_headers()
target_link_libraries(tecka krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(tecka)
krecek_set_target_options(tecka)

if (ENABLE_OFFSETS)
  pico_set_linker_script(tecka ${CMAKE_CURRENT_LIST_DIR}/memmap.ld)
endif()

#pico_set_binary_type(tecka no_flash)
#pico_set_binary_type(tecka copy_to_ram)
