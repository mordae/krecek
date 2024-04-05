add_executable(peckovana main.c)
target_include_directories(peckovana PRIVATE include)
target_link_libraries(peckovana krecek)
pico_add_extra_outputs(peckovana)
krecek_set_target_options(peckovana)

#pico_set_binary_type(peckovana no_flash)
#pico_set_binary_type(peckovana copy_to_ram)
