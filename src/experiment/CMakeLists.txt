add_executable(experiment main.c)
target_include_directories(experiment PRIVATE include)
generate_png_headers()
target_link_libraries(experiment krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(experiment)
krecek_set_target_options(experiment)

#pico_set_binary_type(experiment no_flash)
#pico_set_binary_type(experiment copy_to_ram)
