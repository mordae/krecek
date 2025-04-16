add_executable(
  janek
  ../../src/janek/main.c
)

generate_png_headers()
target_link_libraries(janek PRIVATE sdk m ${PNG_HEADERS_TARGET})

set_property(TARGET janek PROPERTY C_STANDARD 23)
target_compile_options(janek PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(janek PRIVATE include)

install(TARGETS janek DESTINATION bin)
