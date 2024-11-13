add_executable(krectris main.c)
target_include_directories(krectris PRIVATE include)
target_link_libraries(krectris krecek)
pico_add_extra_outputs(krectris)
krecek_set_target_options(krectris)

#pico_set_binary_type(krectris no_flash)
#pico_set_binary_type(krectris copy_to_ram)
