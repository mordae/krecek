add_executable(cardtest main.c)
target_include_directories(cardtest PRIVATE include)
target_link_libraries(cardtest krecek)
pico_add_extra_outputs(cardtest)
krecek_set_target_options(cardtest)

#pico_set_binary_type(cardtest no_flash)
#pico_set_binary_type(cardtest copy_to_ram)
