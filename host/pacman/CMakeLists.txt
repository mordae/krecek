add_executable(
  pacman
  ../../src/pacman/main.c
)

generate_png_headers()
target_link_libraries(pacman PRIVATE sdk m ${PNG_HEADERS_TARGET})

set_property(TARGET pacman PROPERTY C_STANDARD 23)
target_compile_options(pacman PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(pacman PRIVATE include ../../src/pacman/tiles)

install(TARGETS pacman DESTINATION bin)
