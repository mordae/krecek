add_executable(
  tecka
  ../../src/tecka/main.c
)

generate_png_headers()
target_link_libraries(tecka PRIVATE sdk m ${PNG_HEADERS_TARGET})

set_property(TARGET tecka PROPERTY C_STANDARD 23)
target_compile_options(tecka PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(tecka PRIVATE include)

install(TARGETS tecka DESTINATION bin)
