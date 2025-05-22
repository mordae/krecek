add_executable(
  arnost
  ../../src/arnost/main.c
)

generate_png_headers()
target_link_libraries(arnost PRIVATE sdk m ${PNG_HEADERS_TARGET})

set_property(TARGET arnost PROPERTY C_STANDARD 23)
target_compile_options(arnost PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(arnost PRIVATE include)

install(TARGETS arnost DESTINATION bin)
