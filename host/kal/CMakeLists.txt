add_executable(
  kal
  ../../src/kal/main.c
)

generate_png_headers()
target_link_libraries(kal PRIVATE sdk m ${PNG_HEADERS_TARGET})

set_property(TARGET kal PROPERTY C_STANDARD 23)
target_compile_options(kal PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(kal PRIVATE include ${CMAKE_CURRENT_LIST_DIR}/../../src/kal/tiles)

install(TARGETS kal DESTINATION bin)
