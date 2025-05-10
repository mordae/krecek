add_executable(kal main.c)
target_include_directories(kal PRIVATE include)
generate_png_headers()
target_link_libraries(kal krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(kal)
krecek_set_target_options(kal)

if (ENABLE_OFFSETS)
  pico_set_linker_script(kal ${CMAKE_CURRENT_LIST_DIR}/memmap.ld)
endif()

#pico_set_binary_type(menu no_flash)
#pico_set_binary_type(menu copy_to_ram)
