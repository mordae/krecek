add_executable(
  experiment
  ../../src/experiment/main.c
)

generate_png_headers()
generate_bin_headers()
target_link_libraries(experiment PRIVATE sdk m ${PNG_HEADERS_TARGET} ${BIN_HEADERS_TARGET})

set_property(TARGET experiment PROPERTY C_STANDARD 23)
target_compile_options(experiment PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(experiment PRIVATE include)

install(TARGETS experiment DESTINATION bin)
