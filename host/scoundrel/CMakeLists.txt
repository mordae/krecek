add_executable(
  scoundrel
  ../../src/scoundrel/main.c
)

generate_png_headers()
target_link_libraries(scoundrel PRIVATE sdk m ${PNG_HEADERS_TARGET})

set_property(TARGET scoundrel PROPERTY C_STANDARD 23)
target_compile_options(scoundrel PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(scoundrel PRIVATE include ../../src/scoundrel/tiles)

install(TARGETS scoundrel DESTINATION bin)
