add_executable(
  menu
  ../../src/menu/main.c
)

generate_png_headers()
target_link_libraries(menu PRIVATE sdk m ${PNG_HEADERS_TARGET})

set_property(TARGET menu PROPERTY C_STANDARD 23)
target_compile_options(menu PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(menu PRIVATE include ${CMAKE_CURRENT_LIST_DIR}/../../src/menu/tiles)

install(TARGETS menu DESTINATION bin)
