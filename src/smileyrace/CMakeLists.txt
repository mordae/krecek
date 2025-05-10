add_executable(smileyrace main.c)
target_include_directories(smileyrace PRIVATE include)
generate_png_headers()
target_link_libraries(smileyrace krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(smileyrace)
krecek_set_target_options(smileyrace)

if (ENABLE_OFFSETS)
  pico_set_linker_script(smileyrace ${CMAKE_CURRENT_LIST_DIR}/memmap.ld)
endif()

#pico_set_binary_type(smileyrace no_flash)
#pico_set_binary_type(smileyrace copy_to_ram)
