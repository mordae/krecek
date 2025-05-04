add_executable(krectris main.c)
target_include_directories(krectris PRIVATE include)
generate_png_headers()
target_link_libraries(krectris krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(krectris)
krecek_set_target_options(krectris)

if (ENABLE_OFFSETS)
  pico_set_linker_script(krectris ${CMAKE_CURRENT_LIST_DIR}/memmap.ld)
endif()

#pico_set_binary_type(krectris no_flash)
#pico_set_binary_type(krectris copy_to_ram)
