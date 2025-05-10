add_executable(
  janek
  main.c
)
target_include_directories(janek PRIVATE include)
generate_png_headers()
target_link_libraries(janek krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(janek)
krecek_set_target_options(janek)

if (ENABLE_OFFSETS)
  pico_set_linker_script(janek ${CMAKE_CURRENT_LIST_DIR}/memmap.ld)
endif()

#pico_set_binary_type(janek no_flash)
#pico_set_binary_type(janek copy_to_ram)
