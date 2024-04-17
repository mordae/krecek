add_executable(pong main.c)
target_include_directories(pong PRIVATE include)
target_link_libraries(pong krecek)
pico_add_extra_outputs(pong)
krecek_set_target_options(pong)

#pico_set_binary_type(pong no_flash)
#pico_set_binary_type(pong copy_to_ram)
