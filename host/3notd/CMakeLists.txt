add_executable(
  3notd
  ../../src/3notd/main.c
)

generate_png_headers()
target_link_libraries(3notd PRIVATE sdk m ${PNG_HEADERS_TARGET})

set_property(TARGET 3notd PROPERTY C_STANDARD 23)
target_compile_options(3notd PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(3notd PRIVATE include)

install(TARGETS 3notd DESTINATION bin)
