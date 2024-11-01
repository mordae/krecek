add_executable(
  demolice
  ../../src/demolice/main.c
)

target_link_libraries(demolice PRIVATE sdk m)

set_property(TARGET demolice PROPERTY C_STANDARD 23)
target_compile_options(demolice PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(demolice PRIVATE include)

install(TARGETS demolice DESTINATION bin)
