add_executable(cerviiik main.c)
target_include_directories(cerviiik PRIVATE include)
target_link_libraries(cerviiik krecek)
pico_add_extra_outputs(cerviiik)
krecek_set_target_options(cerviiik)

#pico_set_linker_script(cerviiik ${CMAKE_CURRENT_LIST_DIR}/memmap.ld)

#pico_set_binary_type(cerviiik no_flash)
#pico_set_binary_type(cerviiik copy_to_ram)
