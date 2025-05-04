add_executable(pacman main.c)
target_include_directories(pacman PRIVATE include tiles)
generate_png_headers()
target_link_libraries(pacman krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(pacman)
krecek_set_target_options(pacman)

if (ENABLE_OFFSETS)
  pico_set_linker_script(pacman ${CMAKE_CURRENT_LIST_DIR}/memmap.ld)
endif()

#pico_set_binary_type(pacman no_flash)
#pico_set_binary_type(pacman copy_to_ram)
