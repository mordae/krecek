add_executable(
  puntog
  ../../src/puntog/main.c
  ../../src/puntog/maps/tuturial.c
)

generate_png_headers()
target_link_libraries(puntog PRIVATE sdk m ${PNG_HEADERS_TARGET})

set_property(TARGET puntog PROPERTY C_STANDARD 23)
target_compile_options(puntog PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(puntog PRIVATE include)

install(TARGETS puntog DESTINATION bin)
