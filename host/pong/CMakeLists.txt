add_executable(
  pong
  ../../src/pong/main.c
)

target_link_libraries(pong PRIVATE sdk m)

set_property(TARGET pong PROPERTY C_STANDARD 23)
target_compile_options(pong PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(pong PRIVATE include)

install(TARGETS pong DESTINATION bin)
