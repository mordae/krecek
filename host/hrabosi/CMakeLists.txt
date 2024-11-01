add_executable(
  hrabosi
  ../../src/hrabosi/main.c
)

target_link_libraries(hrabosi PRIVATE sdk m)

set_property(TARGET hrabosi PROPERTY C_STANDARD 23)
target_compile_options(hrabosi PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(hrabosi PRIVATE include ${CMAKE_CURRENT_LIST_DIR}/../../src/hrabosi/tiles)

install(TARGETS hrabosi DESTINATION bin)
