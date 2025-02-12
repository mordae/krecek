add_executable(
  experiment
  ../../src/experiment/main.c
)

target_link_libraries(experiment PRIVATE sdk m)

set_property(TARGET experiment PROPERTY C_STANDARD 23)
target_compile_options(experiment PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(experiment PRIVATE include)

install(TARGETS experiment DESTINATION bin)
