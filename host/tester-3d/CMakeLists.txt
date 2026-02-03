add_executable(
  tester-3d
  ../../src/tester-3d/main.c
)

target_link_libraries(tester-3d PRIVATE sdk m)

set_property(TARGET tester-3d PROPERTY C_STANDARD 23)
target_compile_options(tester-3d PRIVATE -Wall -Wextra -Wnull-dereference)

install(TARGETS tester-3d DESTINATION bin)
