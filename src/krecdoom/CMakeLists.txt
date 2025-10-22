add_executable(
    krecdoom
    main.c
    extras/volume.c
    maps/map1.c
    maps/map2.c
    maps/map3.c
    maps/map4.c
)

target_include_directories(krecdoom PRIVATE include)
generate_png_headers()
target_link_libraries(krecdoom krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(krecdoom)
krecek_set_target_options(krecdoom)

if (ENABLE_OFFSETS)
  pico_set_linker_script(krecdoom ${CMAKE_CURRENT_LIST_DIR}/memmap.ld)
endif()

#pico_set_binary_type(krecdoom no_flash)
#pico_set_binary_type(krecdoom copy_to_ram)
