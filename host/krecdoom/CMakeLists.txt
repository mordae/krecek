add_executable(
  krecdoom
  ../../src/krecdoom/main.c
  ../../src/krecdoom/extras/volume.c
  ../../src/krecdoom/maps/map1.c
  ../../src/krecdoom/maps/map2.c
  ../../src/krecdoom/maps/map3.c
  ../../src/krecdoom/maps/map4.c
  draw_wall_column.S
)

generate_png_headers()
target_link_libraries(krecdoom PRIVATE sdk m ${PNG_HEADERS_TARGET})

set_property(TARGET krecdoom PROPERTY C_STANDARD 23)
target_compile_options(krecdoom PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(krecdoom PRIVATE include)

install(TARGETS krecdoom DESTINATION bin)
