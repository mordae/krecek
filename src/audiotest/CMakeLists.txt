add_executable(audiotest main.c)
target_include_directories(audiotest PRIVATE include)
target_link_libraries(audiotest krecek)
pico_add_extra_outputs(audiotest)
krecek_set_target_options(audiotest)

#pico_set_binary_type(audiotest no_flash)
#pico_set_binary_type(audiotest copy_to_ram)
