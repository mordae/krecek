add_executable(
  s_r_tester
  ../../src/s_r_tester/main.c
)

generate_png_headers()
target_link_libraries(s_r_tester PRIVATE sdk m ${PNG_HEADERS_TARGET})

set_property(TARGET s_r_tester PROPERTY C_STANDARD 23)
target_compile_options(s_r_tester PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(s_r_tester PRIVATE include)

install(TARGETS s_r_tester DESTINATION bin)
