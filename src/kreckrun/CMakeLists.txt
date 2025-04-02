add_executable(kreckrun main.c)
target_include_directories(kreckrun PRIVATE include)
generate_png_headers()
target_link_libraries(kreckrun krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(kreckrun)
krecek_set_target_options(kreckrun)

#pico_set_binary_type(kreckrun no_flash)
#pico_set_binary_type(kreckrun copy_to_ram)
