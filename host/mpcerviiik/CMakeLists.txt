add_executable(
  mpcerviiik
  ../../src/mpcerviiik/main.c
  ../../src/mpcerviiik/root.c
  ../../src/mpcerviiik/round.c
)

generate_png_headers()
target_link_libraries(mpcerviiik PRIVATE sdk m ${PNG_HEADERS_TARGET})

set_property(TARGET mpcerviiik PROPERTY C_STANDARD 23)
target_compile_options(mpcerviiik PRIVATE -Wall -Wextra -Wnull-dereference)
target_include_directories(mpcerviiik PRIVATE include)

install(TARGETS mpcerviiik DESTINATION bin)
