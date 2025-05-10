function(generate_bin_headers)
  # Find Python interpreter once
  find_package(Python3 REQUIRED)

  # Get absolute path to the converter script
  set(CONVERTER_SCRIPT "${CMAKE_SOURCE_DIR}/../tools/bin_to_header.py")

  # Get all BIN files in assets/ relative to current source dir
  file(GLOB_RECURSE BIN_FILES FOLLOW_SYMLINKS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/*.bin")

  # Setup output directory in binary dir
  set(HEADERS_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets")

  # Create list to store generated files
  set(GENERATED_HEADERS "")

  # Create unique target names based on current directory
  set(TARGET_NAME "${CMAKE_CURRENT_SOURCE_DIR}/assets")
  string(MAKE_C_IDENTIFIER "${TARGET_NAME}" TARGET_NAME)

  # Add custom target to group the assets.
  add_custom_target(${TARGET_NAME}_bin_headers)

  foreach(BIN_FILE ${BIN_FILES})
    file(RELATIVE_PATH BIN_FILE_REL "${CMAKE_CURRENT_SOURCE_DIR}/assets" "${BIN_FILE}")
    set(HEADER_FILE "${HEADERS_OUTPUT_DIR}/${BIN_FILE_REL}.h")

    # Create unique target name for this header
    set(HEADER_TARGET "${TARGET_NAME}_${BIN_FILE_REL}_header")
    string(MAKE_C_IDENTIFIER "${HEADER_TARGET}" HEADER_TARGET)

    add_custom_command(
      OUTPUT ${HEADER_FILE}
      COMMAND ${Python3_EXECUTABLE} ${CONVERTER_SCRIPT} -n ${BIN_FILE_REL} -o ${HEADER_FILE} ${BIN_FILE}
      DEPENDS ${BIN_FILE} ${CONVERTER_SCRIPT}
      COMMENT "Generating ${HEADER_FILE}.h"
      VERBATIM
    )

    # Create custom target for this header
    add_custom_target(${HEADER_TARGET} DEPENDS ${HEADER_FILE})

    # Add the target as dependency
    add_dependencies(${TARGET_NAME}_bin_headers ${HEADER_TARGET})
  endforeach()

  # Create an INTERFACE library to handle include directory
  add_library(${TARGET_NAME}_bin INTERFACE)
  add_dependencies(${TARGET_NAME}_bin ${TARGET_NAME}_bin_headers)
  target_include_directories(${TARGET_NAME}_bin INTERFACE ${HEADERS_OUTPUT_DIR})

  # Export the target name to parent scope
  set(BIN_HEADERS_TARGET ${TARGET_NAME}_bin PARENT_SCOPE)
endfunction()
