add_executable(
  krec2ddoom
  ../../src/krec2ddoom/main.c
)

generate_png_headers()
target_link_libraries(krec2ddoom PRIVATE sdk m ${PNG_HEADERS_TARGET})

set_property(TARGET krec2ddoom PROPERTY C_STANDARD 23)
target_compile_options(krec2ddoom PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(krec2ddoom PRIVATE include)

install(TARGETS krec2ddoom DESTINATION bin)
