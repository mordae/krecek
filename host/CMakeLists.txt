cmake_minimum_required(VERSION 3.21)

project(krecek)

find_package(PkgConfig REQUIRED)
find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)

pkg_check_modules(SDL2_Pango REQUIRED IMPORTED_TARGET SDL2_Pango)
pkg_check_modules(freetype2 REQUIRED IMPORTED_TARGET freetype2)
pkg_check_modules(PA REQUIRED libpulse-simple)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif()


add_compile_definitions(RUNNING_ON_HOST=1)
# Screen to simulate:
add_compile_definitions(TFT_DRIVER=TFT_DRIVER_ILI9341)
add_compile_definitions(TFT_VSYNC=1)
add_compile_definitions(TFT_SWAP_XY=1)
add_compile_definitions(TFT_SCALE=2)

add_subdirectory(sdk)
add_subdirectory(tecka)
add_subdirectory(cerviiik)
add_subdirectory(peckovana)
add_subdirectory(hrabosi)
add_subdirectory(pong)
add_subdirectory(snake)
add_subdirectory(demolice)
add_subdirectory(pallete)
add_subdirectory(pacman)
add_subdirectory(krectris)
