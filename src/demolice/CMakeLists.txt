add_executable(demolice main.c)
target_include_directories(demolice PRIVATE include)
target_link_libraries(demolice krecek)
pico_add_extra_outputs(demolice)
krecek_set_target_options(demolice)

#pico_set_binary_type(demolice no_flash)
#pico_set_binary_type(demolice copy_to_ram)
