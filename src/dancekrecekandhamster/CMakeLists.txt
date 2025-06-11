add_executable(dancekrecekandhamster main.c)
target_include_directories(dancekrecekandhamster PRIVATE include)
generate_png_headers()
target_link_libraries(dancekrecekandhamster krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(dancekrecekandhamster)
krecek_set_target_options(dancekrecekandhamster)

if (ENABLE_OFFSETS)
  pico_set_linker_script(dancekrecekandhamster ${CMAKE_CURRENT_LIST_DIR}/memmap.ld)
endif()

#pico_set_binary_type(menu no_flash)
#pico_set_binary_type(menu copy_to_ram)
