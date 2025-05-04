add_executable(minesweeper main.c)
target_include_directories(minesweeper PRIVATE include)
generate_png_headers()
target_link_libraries(minesweeper krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(minesweeper)
krecek_set_target_options(minesweeper)

if (ENABLE_OFFSETS)
  pico_set_linker_script(minesweeper ${CMAKE_CURRENT_LIST_DIR}/memmap.ld)
endif()

#pico_set_binary_type(minesweeper no_flash)
#pico_set_binary_type(minesweeper copy_to_ram)
