add_executable(
  peckovana
  ../../src/peckovana/main.c
)

target_link_libraries(peckovana PRIVATE sdk m)

set_property(TARGET peckovana PROPERTY C_STANDARD 23)
target_compile_options(peckovana PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(peckovana PRIVATE include ${CMAKE_CURRENT_LIST_DIR}/../../src/peckovana/tiles)

install(TARGETS peckovana DESTINATION bin)
