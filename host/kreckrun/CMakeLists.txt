add_executable(
  kreckrun
  ../../src/kreckrun/main.c
)

generate_png_headers()
target_link_libraries(kreckrun PRIVATE sdk m ${PNG_HEADERS_TARGET})

set_property(TARGET kreckrun PROPERTY C_STANDARD 23)
target_compile_options(kreckrun PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(kreckrun PRIVATE include)

install(TARGETS kreckrun DESTINATION bin)
