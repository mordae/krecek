add_executable(menu main.c)
target_include_directories(menu PRIVATE include)
generate_png_headers()
target_link_libraries(menu krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(menu)
krecek_set_target_options(menu)

if (ENABLE_OFFSETS)
  pico_set_linker_script(menu ${CMAKE_CURRENT_LIST_DIR}/memmap.ld)
endif()

#pico_set_binary_type(menu no_flash)
#pico_set_binary_type(menu copy_to_ram)
