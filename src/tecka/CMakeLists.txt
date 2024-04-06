add_executable(tecka main.c)
target_include_directories(tecka PRIVATE include)
target_link_libraries(tecka krecek)
pico_add_extra_outputs(tecka)
krecek_set_target_options(tecka)

#pico_set_binary_type(tecka no_flash)
#pico_set_binary_type(tecka copy_to_ram)
