add_executable(
  experiment_editor
  ../../src/experiment-editor/main.c
)

generate_png_headers()
target_link_libraries(experiment_editor PRIVATE sdk m ${PNG_HEADERS_TARGET})

set_property(TARGET experiment_editor PROPERTY C_STANDARD 23)
target_compile_options(experiment_editor PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(experiment_editor PRIVATE include)

install(TARGETS experiment_editor DESTINATION bin)
