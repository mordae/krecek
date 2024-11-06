add_executable(
  pacman
  ../../src/pacman/main.c
)

target_link_libraries(pacman PRIVATE sdk m)

set_property(TARGET pacman PROPERTY C_STANDARD 23)
target_compile_options(pacman PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(pacman PRIVATE include)

install(TARGETS pacman DESTINATION bin)
