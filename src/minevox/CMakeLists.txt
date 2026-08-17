add_executable(
    minevox
    main.c
    blocks.c
    world.c
    player.c
    render.c
    hud.c
)

target_link_libraries(minevox krecek)
pico_add_extra_outputs(minevox)
krecek_set_target_options(minevox)

if (ENABLE_OFFSETS)
  pico_set_linker_script(minevox ${CMAKE_CURRENT_LIST_DIR}/memmap.ld)
endif()
