add_executable(
    krecdoom
    main.c
    volume.c
    maps/map1.c
    maps/map2.c
)

target_include_directories(krecdoom PRIVATE include)
generate_png_headers()
target_link_libraries(krecdoom krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(krecdoom)
krecek_set_target_options(krecdoom)

#pico_set_binary_type(krecdoom no_flash)
#pico_set_binary_type(krecdoom copy_to_ram)
