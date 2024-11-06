add_executable(
  pallete
  ../../src/pallete/main.c
)

target_link_libraries(pallete PRIVATE sdk m)

set_property(TARGET pallete PROPERTY C_STANDARD 23)
target_compile_options(pallete PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(pallete PRIVATE include)

install(TARGETS pallete DESTINATION bin)
