add_executable(
  snake
  ../../src/snake/main.c
)

generate_png_headers()
target_link_libraries(snake PRIVATE sdk m ${PNG_HEADERS_TARGET})

set_property(TARGET snake PROPERTY C_STANDARD 23)
target_compile_options(snake PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(snake PRIVATE include)

install(TARGETS snake DESTINATION bin)
