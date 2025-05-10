function(generate_png_headers)
  # Find Python interpreter once
  find_package(Python3 REQUIRED)

  # Get absolute path to the converter script
  set(CONVERTER_SCRIPT "${CMAKE_SOURCE_DIR}/../tools/png_to_header.py")

  # Get all PNG files in assets/ relative to current source dir
  file(GLOB_RECURSE PNG_FILES FOLLOW_SYMLINKS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/*.png")

  # Setup output directory in binary dir
  set(HEADERS_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets")

  # Create list to store generated files
  set(GENERATED_HEADERS "")

  # Create unique target names based on current directory
  set(TARGET_NAME "${CMAKE_CURRENT_SOURCE_DIR}/assets")
  string(MAKE_C_IDENTIFIER "${TARGET_NAME}" TARGET_NAME)

  # Add custom target to group the assets.
  add_custom_target(${TARGET_NAME}_png_headers)

  foreach(PNG_FILE ${PNG_FILES})
    file(RELATIVE_PATH PNG_FILE_REL "${CMAKE_CURRENT_SOURCE_DIR}/assets" "${PNG_FILE}")
    set(HEADER_FILE "${HEADERS_OUTPUT_DIR}/${PNG_FILE_REL}.h")

    # Create unique target name for this header
    set(HEADER_TARGET "${TARGET_NAME}_${PNG_FILE_REL}_header")
    string(MAKE_C_IDENTIFIER "${HEADER_TARGET}" HEADER_TARGET)

    add_custom_command(
      OUTPUT ${HEADER_FILE}
      COMMAND ${Python3_EXECUTABLE} ${CONVERTER_SCRIPT} -n ${PNG_FILE_REL} -o ${HEADER_FILE} ${PNG_FILE}
      DEPENDS ${PNG_FILE} ${CONVERTER_SCRIPT}
      COMMENT "Generating ${PNG_FILE_REL}.h"
      VERBATIM
    )

    # Create custom target for this header
    add_custom_target(${HEADER_TARGET} DEPENDS ${HEADER_FILE})

    # Add the target as dependency
    add_dependencies(${TARGET_NAME}_png_headers ${HEADER_TARGET})
  endforeach()

  # Create an INTERFACE library to handle include directory
  add_library(${TARGET_NAME}_png INTERFACE)
  add_dependencies(${TARGET_NAME}_png ${TARGET_NAME}_png_headers)
  target_include_directories(${TARGET_NAME}_png INTERFACE ${HEADERS_OUTPUT_DIR})

  # Export the target name to parent scope
  set(PNG_HEADERS_TARGET ${TARGET_NAME}_png PARENT_SCOPE)
endfunction()
