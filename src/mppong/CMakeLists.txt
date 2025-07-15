add_executable(
  mppong
  main.c
  root.c
  round.c
)
target_include_directories(mppong PRIVATE include)
generate_png_headers()
target_link_libraries(mppong krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(mppong)
krecek_set_target_options(mppong)

if (ENABLE_OFFSETS)
  pico_set_linker_script(mppong ${CMAKE_CURRENT_LIST_DIR}/memmap.ld)
endif()

#pico_set_binary_type(pong no_flash)
#pico_set_binary_type(pong copy_to_ram)
