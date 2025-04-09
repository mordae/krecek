add_executable(
  krectris
  ../../src/krectris/main.c
)

generate_png_headers()
target_link_libraries(krectris PRIVATE sdk m ${PNG_HEADERS_TARGET})

set_property(TARGET krectris PROPERTY C_STANDARD 23)
target_compile_options(krectris PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(krectris PRIVATE include)

install(TARGETS krectris DESTINATION bin)
