add_executable(arnost main.c)
target_include_directories(arnost PRIVATE include)
generate_png_headers()
target_link_libraries(arnost krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(arnost)
krecek_set_target_options(arnost)

if (ENABLE_OFFSETS)
  pico_set_linker_script(arnost ${CMAKE_CURRENT_LIST_DIR}/memmap.ld)
endif()

#pico_set_binary_type(arnost no_flash)
#pico_set_binary_type(arnost copy_to_ram)
