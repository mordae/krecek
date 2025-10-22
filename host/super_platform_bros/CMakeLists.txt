add_executable(
  super_platform_bros
  ../../src/super_platform_bros/main.c
)

generate_png_headers()
target_link_libraries(super_platform_bros PRIVATE sdk m ${PNG_HEADERS_TARGET})

set_property(TARGET super_platform_bros PROPERTY C_STANDARD 23)
target_compile_options(super_platform_bros PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(super_platform_bros PRIVATE include)

install(TARGETS super_platform_bros DESTINATION bin)
