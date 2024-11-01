add_executable(
  cerviiik
  ../../src/cerviiik/main.c
)

target_link_libraries(cerviiik PRIVATE sdk m)

set_property(TARGET cerviiik PROPERTY C_STANDARD 23)
target_compile_options(cerviiik PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(cerviiik PRIVATE include)

install(TARGETS cerviiik DESTINATION bin)
