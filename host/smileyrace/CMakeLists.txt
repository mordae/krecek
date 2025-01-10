add_executable(
  smileyrace
  ../../src/smileyrace/main.c
)

target_link_libraries(smileyrace PRIVATE sdk m)

set_property(TARGET smileyrace PROPERTY C_STANDARD 23)
target_compile_options(smileyrace PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(smileyrace PRIVATE include)

install(TARGETS smileyrace DESTINATION bin)
