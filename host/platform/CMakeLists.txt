add_executable(
  platform
  ../../src/platform/main.c
  ../../src/platform/maps/map0.c
  ../../src/platform/maps/map1.c
  ../../src/platform/maps/map2.c
  ../../src/platform/maps/map3.c
  ../../src/platform/maps/map4.c
  ../../src/platform/maps/mapwin.c
  ../../src/platform/maps/map1c.c
  ../../src/platform/maps/map2c.c
  ../../src/platform/maps/map3c.c
)

generate_png_headers()
target_link_libraries(platform PRIVATE sdk m ${PNG_HEADERS_TARGET})

set_property(TARGET platform PROPERTY C_STANDARD 23)
target_compile_options(platform PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(platform PRIVATE include)

install(TARGETS platform DESTINATION bin)
