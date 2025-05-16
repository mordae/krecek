add_executable(
  krec2ddoom
  main.c
)
target_include_directories(krec2ddoom PRIVATE include)
generate_png_headers()
target_link_libraries(krec2ddoom krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(krec2ddoom)
krecek_set_target_options(krec2ddoom)

if (ENABLE_OFFSETS)
  pico_set_linker_script(krec2ddoom ${CMAKE_CURRENT_LIST_DIR}/memmap.ld)
endif()

#pico_set_binary_type(krec2ddoom no_flash)
#pico_set_binary_type(krec2ddoomcopy_to_ram)
