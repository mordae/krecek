add_executable(scoundrel main.c)
target_include_directories(scoundrel PRIVATE include tiles)
generate_png_headers()
target_link_libraries(scoundrel krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(scoundrel)
krecek_set_target_options(scoundrel)

#pico_set_binary_type(scoundrel no_flash)
#pico_set_binary_type(scoundrel copy_to_ram)
