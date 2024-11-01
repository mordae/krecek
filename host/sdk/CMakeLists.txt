add_library(
  sdk
  text.c
  main.c
  input.c
  audio.c
  video.c
  tft.c
  pico.c
  ${CMAKE_CURRENT_LIST_DIR}/../../src/vendor/pico-tft/font.c
)

if (TARGET SDL2::SDL2main)
  target_link_libraries(sdk PRIVATE SDL2::SDL2main)
endif()

target_link_libraries(sdk PRIVATE SDL2::SDL2 PkgConfig::SDL2_Pango PkgConfig::freetype2 ${PA_LIBRARIES} m)

set_property(TARGET sdk PROPERTY C_STANDARD 23)
target_compile_options(sdk PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(sdk PRIVATE include)

if (TARGET SDL2::SDL2main)
  target_link_libraries(sdk PRIVATE SDL2::SDL2main)
endif()

target_link_libraries(sdk PRIVATE SDL2::SDL2 PkgConfig::SDL2_Pango PkgConfig::freetype2 m)

set_property(TARGET sdk PROPERTY C_STANDARD 23)
target_compile_options(sdk PRIVATE -Wall -Wextra -Wnull-dereference)

target_include_directories(
  sdk
  PUBLIC
    include
    ${CMAKE_CURRENT_LIST_DIR}/../../src/sdk/include
    ${CMAKE_CURRENT_LIST_DIR}/../../src/vendor/pico-tft/include
)
