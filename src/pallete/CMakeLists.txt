add_executable(pallete main.c)
target_include_directories(pallete PRIVATE include)
target_link_libraries(pallete krecek)
pico_add_extra_outputs(pallete)
krecek_set_target_options(pallete)

#pico_set_binary_type(pallete no_flash)
#pico_set_binary_type(pallete copy_to_ram)
