add_executable(smileyrace main.c)
target_include_directories(smileyrace PRIVATE include)
target_link_libraries(smileyrace krecek)
pico_add_extra_outputs(smileyrace)
krecek_set_target_options(smileyrace)

#pico_set_binary_type(smileyrace no_flash)
#pico_set_binary_type(smileyrace copy_to_ram)
