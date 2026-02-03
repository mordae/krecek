add_executable(
    tester-3d
    main.c
)

target_link_libraries(tester-3d krecek)
pico_add_extra_outputs(tester-3d)
krecek_set_target_options(tester-3d)
