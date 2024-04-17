add_executable(hrabosi main.c)
target_include_directories(hrabosi PRIVATE include tiles)
target_link_libraries(hrabosi krecek)
pico_add_extra_outputs(hrabosi)
krecek_set_target_options(hrabosi)

#pico_set_binary_type(hrabosi no_flash)
#pico_set_binary_type(hrabosi copy_to_ram)
