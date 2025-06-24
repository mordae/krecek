add_executable(puntog main.c maps/tuturial.c)
target_include_directories(puntog PRIVATE include)
generate_png_headers()
target_link_libraries(puntog krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(puntog)
krecek_set_target_options(puntog)

if (ENABLE_OFFSETS)
  pico_set_linker_script(puntog ${CMAKE_CURRENT_LIST_DIR}/memmap.ld)
endif()

#pico_set_binary_type(puntog no_flash)
#pico_set_binary_type(puntog copy_to_ram)
