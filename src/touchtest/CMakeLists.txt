add_executable(touchtest main.c)
target_include_directories(touchtest PRIVATE include)
target_link_libraries(touchtest krecek)
pico_add_extra_outputs(touchtest)
krecek_set_target_options(touchtest)

#pico_set_binary_type(touchtest no_flash)
#pico_set_binary_type(touchtest copy_to_ram)
