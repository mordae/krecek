add_executable(
  cardtest
  ../../src/cardtest/main.c
)

target_link_libraries(cardtest PRIVATE sdk m)

set_property(TARGET cardtest PROPERTY C_STANDARD 23)
target_compile_options(cardtest PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(cardtest PRIVATE include)

install(TARGETS cardtest DESTINATION bin)
