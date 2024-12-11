add_executable(minesweeper main.c)
target_include_directories(minesweeper PRIVATE include)
target_link_libraries(minesweeper krecek)
pico_add_extra_outputs(minesweeper)
krecek_set_target_options(minesweeper)

#pico_set_binary_type(minesweeper no_flash)
#pico_set_binary_type(minesweeper copy_to_ram)
