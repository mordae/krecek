add_executable(
  dancekrecekandhamster
  ../../src/dancekrecekandhamster/main.c
)

generate_png_headers()
target_link_libraries(dancekrecekandhamster PRIVATE sdk m ${PNG_HEADERS_TARGET})

set_property(TARGET dancekrecekandhamster PROPERTY C_STANDARD 23)
target_compile_options(dancekrecekandhamster PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(dancekrecekandhamster PRIVATE include ${CMAKE_CURRENT_LIST_DIR}/../../src/kal/tiles)

install(TARGETS dancekrecekandhamster DESTINATION bin)
