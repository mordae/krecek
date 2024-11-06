add_executable(pacman main.c)
target_include_directories(pacman PRIVATE include)
target_link_libraries(pacman krecek)
pico_add_extra_outputs(pacman)
krecek_set_target_options(pacman)

#pico_set_binary_type(pacman no_flash)
#pico_set_binary_type(pacman copy_to_ram)
