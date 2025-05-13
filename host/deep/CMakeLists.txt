add_executable(
  deep
  ../../src/deep/main.c
  ../../src/deep/level.c
)

generate_png_headers()
generate_bin_headers()
target_link_libraries(deep PRIVATE sdk m ${PNG_HEADERS_TARGET} ${BIN_HEADERS_TARGET})

set_property(TARGET deep PROPERTY C_STANDARD 23)
target_compile_options(deep PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(deep PRIVATE include)

install(TARGETS deep DESTINATION bin)
