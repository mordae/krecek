add_executable(
  experiment_editor
  main.c
  ../experiment/maps/map1.c
  ../experiment/maps/map2.c
)
target_include_directories(experiment_editor PRIVATE include)
generate_png_headers()
target_link_libraries(experiment_editor krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(experiment_editor)
krecek_set_target_options(experiment_editor)

#pico_set_binary_type(experiment_editor no_flash)
#pico_set_binary_type(experiment_editor copy_to_ram)
