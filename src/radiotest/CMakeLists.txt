add_executable(radiotest main.c)
target_include_directories(radiotest PRIVATE include)
target_link_libraries(radiotest krecek)
pico_add_extra_outputs(radiotest)
krecek_set_target_options(radiotest)

#pico_set_binary_type(radiotest no_flash)
#pico_set_binary_type(radiotest copy_to_ram)
