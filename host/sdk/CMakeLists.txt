add_library(
  sdk
  main.c
  save.c
  input.c
  audio.c
  video.c
  comms.c
  tft.c
  pico.c
  sdcard.c
  ../../src/sdk/melody.c
  ../../src/sdk/melody-lexer.c
  ../../src/sdk/scene.c
  ../../src/vendor/pico-tft/font.c
)

if (TARGET SDL2::SDL2main)
  target_link_libraries(sdk PRIVATE SDL2::SDL2main)
endif()

target_link_libraries(sdk PRIVATE SDL2::SDL2 ${PA_LIBRARIES} m)

set_property(TARGET sdk PROPERTY C_STANDARD 23)
target_compile_options(sdk PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(sdk PRIVATE include)

if (TARGET SDL2::SDL2main)
  target_link_libraries(sdk PRIVATE SDL2::SDL2main)
endif()

target_link_libraries(sdk PRIVATE SDL2::SDL2 m)

set_property(TARGET sdk PROPERTY C_STANDARD 23)
target_compile_options(sdk PRIVATE -Wall -Wextra -Wnull-dereference)

target_include_directories(
  sdk
  PUBLIC
    include
    ${CMAKE_CURRENT_LIST_DIR}/../../src/sdk/include
    ${CMAKE_CURRENT_LIST_DIR}/../../src/vendor/pico-tft/include
)
