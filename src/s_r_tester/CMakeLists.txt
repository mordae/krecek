add_executable(
  s_r_tester
  main.c
)
target_include_directories(s_r_tester PRIVATE include)
generate_png_headers()
target_link_libraries(s_r_tester krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(s_r_tester)
krecek_set_target_options(s_r_tester)
