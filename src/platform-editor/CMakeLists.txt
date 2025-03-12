add_executable(platform_editor main.c)
target_include_directories(platform_editor PRIVATE include)
generate_png_headers()
target_link_libraries(platform_editor krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(platform_editor)
krecek_set_target_options(platform_editor)

#pico_set_binary_type(platform_editor no_flash)
#pico_set_binary_type(platform_editor copy_to_ram)
