add_executable(
  3notd
  main.c
)
target_include_directories(3notd PRIVATE include)
generate_png_headers()
target_link_libraries(3notd krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(3notd)
krecek_set_target_options(3notd)

#pico_set_binary_type(3notd no_flash)
#pico_set_binary_type(3notd copy_to_ram)
