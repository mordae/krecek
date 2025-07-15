add_executable(
  mppong
  ../../src/mppong/main.c
  ../../src/mppong/root.c
  ../../src/mppong/round.c
)

generate_png_headers()
target_link_libraries(mppong PRIVATE sdk m ${PNG_HEADERS_TARGET})

set_property(TARGET mppong PROPERTY C_STANDARD 23)
target_compile_options(mppong PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(mppong PRIVATE include)

install(TARGETS mppong DESTINATION bin)
