set(TFT_DRIVER "ili9341")

add_definitions(-I${CMAKE_CURRENT_LIST_DIR}/pico-task/include)
list(APPEND PICO_CONFIG_HEADER_FILES task_hooks.h)

add_subdirectory(pico-stdio-usb-simple)
add_subdirectory(pico-tft)
add_subdirectory(pico-task)
