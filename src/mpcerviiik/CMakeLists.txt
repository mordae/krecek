add_executable(mpcerviiik main.c root.c round.c)
target_include_directories(mpcerviiik PRIVATE include)
generate_png_headers()
target_link_libraries(mpcerviiik krecek ${PNG_HEADERS_TARGET})
pico_add_extra_outputs(mpcerviiik)
krecek_set_target_options(mpcerviiik)

if (ENABLE_OFFSETS)
  pico_set_linker_script(mpcerviiik ${CMAKE_CURRENT_LIST_DIR}/memmap.ld)
endif()

#pico_set_binary_type(mpcerviiik no_flash)
#pico_set_binary_type(mpcerviiik copy_to_ram)
