add_executable(
  minevox
  ../../src/minevox/main.c
  ../../src/minevox/blocks.c
  ../../src/minevox/world.c
  ../../src/minevox/player.c
  ../../src/minevox/render.c
  ../../src/minevox/hud.c
)

target_link_libraries(minevox PRIVATE sdk m)

set_property(TARGET minevox PROPERTY C_STANDARD 23)
target_compile_options(minevox PRIVATE -Wall -Wextra -Wnull-dereference)

install(TARGETS minevox DESTINATION bin)
