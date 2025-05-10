add_executable(
  experiment-editor
  main.c
  root.c
  level.c
)
target_include_directories(experiment-editor PRIVATE include)
generate_png_headers()
target_link_libraries(experiment-editor krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(experiment-editor)
krecek_set_target_options(experiment-editor)

#pico_set_binary_type(experiment-editor no_flash)
#pico_set_binary_type(experiment-editor copy_to_ram)
