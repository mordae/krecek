add_executable(platform main.c)
target_include_directories(platform PRIVATE include)
generate_png_headers()
target_link_libraries(platform krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(platform)
krecek_set_target_options(platform)

#pico_set_binary_type(platform no_flash)
#pico_set_binary_type(platform copy_to_ram)
