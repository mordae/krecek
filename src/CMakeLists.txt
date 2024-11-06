cmake_minimum_required(VERSION 3.21)

set(PICO_BOARD krecek-09)
list(APPEND PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR}/include/boards)

include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)

project(krecek)

list(APPEND PICO_CONFIG_HEADER_FILES ${CMAKE_CURRENT_LIST_DIR}/vendor/pico-task/include/task_hooks.h)
pico_sdk_init()

function(krecek_set_target_options target)
  set_property(TARGET ${target} PROPERTY C_STANDARD 23)
  target_compile_options(${target} PRIVATE -Wall -Wextra -Wnull-dereference -g)
  target_compile_definitions(${target} PUBLIC PICO_MAX_SHARED_IRQ_HANDLERS=8u)
  target_compile_definitions(${target} PUBLIC PICO_STDIO_ENABLE_CRLF_SUPPORT=1)
  target_compile_definitions(${target} PUBLIC PICO_STDIO_DEFAULT_CRLF=1)
endfunction()

add_subdirectory(vendor)
add_subdirectory(sdk)
add_subdirectory(tecka)
add_subdirectory(cerviiik)
add_subdirectory(peckovana)
add_subdirectory(hrabosi)
add_subdirectory(pong)
add_subdirectory(snake)
add_subdirectory(demolice)
add_subdirectory(pallete)
