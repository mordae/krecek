add_executable(snake main.c)
target_include_directories(snake PRIVATE include)
target_link_libraries(snake krecek)
pico_add_extra_outputs(snake)
krecek_set_target_options(snake)

#pico_set_binary_type(snake no_flash)
#pico_set_binary_type(snake copy_to_ram)
