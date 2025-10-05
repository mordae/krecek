add_executable(
  krec2ddoom
  main.c
  ../krecdoom/maps/map1.c
  ../krecdoom/maps/map2.c
)
target_include_directories(krec2ddoom PRIVATE include)
generate_png_headers()
target_link_libraries(krec2ddoom krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(krec2ddoom)
krecek_set_target_options(krec2ddoom)

#pico_set_binary_type(krec2ddoom no_flash)
#pico_set_binary_type(krec2ddoomcopy_to_ram)
