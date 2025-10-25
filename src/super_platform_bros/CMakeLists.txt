add_executable(
  super_platform_bros
  main.c
  math.S
)
target_include_directories(super_platform_bros PRIVATE include)
generate_png_headers()
target_link_libraries(super_platform_bros krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(super_platform_bros)
krecek_set_target_options(super_platform_bros)
