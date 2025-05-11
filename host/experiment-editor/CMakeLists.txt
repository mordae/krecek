add_executable(
  experiment-editor
  ../../src/experiment-editor/main.c
  ../../src/experiment-editor/common.c
  ../../src/experiment-editor/root.c
  ../../src/experiment-editor/level.c
  ../../src/experiment-editor/target.c
)

generate_png_headers()
target_link_libraries(experiment-editor PRIVATE sdk m ${PNG_HEADERS_TARGET})

set_property(TARGET experiment-editor PROPERTY C_STANDARD 23)
target_compile_options(experiment-editor PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(experiment-editor PRIVATE include)

install(TARGETS experiment-editor DESTINATION bin)
