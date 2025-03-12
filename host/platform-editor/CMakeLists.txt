add_executable(
  platform_editor
  ../../src/platform-editor/main.c
)

generate_png_headers()
target_link_libraries(platform_editor PRIVATE sdk m ${PNG_HEADERS_TARGET})

set_property(TARGET platform_editor PROPERTY C_STANDARD 23)
target_compile_options(platform_editor PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(platform_editor PRIVATE include)

install(TARGETS platform_editor DESTINATION bin)
