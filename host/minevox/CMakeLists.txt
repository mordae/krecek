add_executable(
  minevox
  ../../src/minevox/main.c
  ../../src/minevox/mem.c
  ../../src/minevox/blocks.c
  ../../src/minevox/world.c
  ../../src/minevox/save.c
  ../../src/minevox/items.c
  ../../src/minevox/mobs.c
  ../../src/minevox/nether.c
  ../../src/minevox/player.c
  ../../src/minevox/render.c
  ../../src/minevox/hud.c
  ../../src/minevox/menu.c
)

target_link_libraries(minevox PRIVATE sdk m)

set_property(TARGET minevox PROPERTY C_STANDARD 23)
target_compile_options(minevox PRIVATE -Wall -Wextra -Wnull-dereference)

install(TARGETS minevox DESTINATION bin)
