add_executable(
  minesweeper
  ../../src/minesweeper/main.c
)

target_link_libraries(minesweeper PRIVATE sdk m)

set_property(TARGET minesweeper PROPERTY C_STANDARD 23)
target_compile_options(minesweeper PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(minesweeper PRIVATE include)

install(TARGETS minesweeper DESTINATION bin)
