# Function to generate export headers for libraries
# Usage: generate_library_export_header(library_name)

function(generate_library_export_header LIBRARY_NAME)
    # Convert library name to uppercase for macros
    string(TOUPPER ${LIBRARY_NAME} LIBRARY_NAME_UPPER)

    # Set template variables
    set(LIBRARY_NAME ${LIBRARY_NAME})
    set(LIBRARY_NAME_UPPER ${LIBRARY_NAME_UPPER})

    # Configure the export header from template
    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/export_header_template.hpp.in
        ${CMAKE_SOURCE_DIR}/lib/${LIBRARY_NAME}/include/${LIBRARY_NAME}/export.hpp
        @ONLY
    )

    message(STATUS "Generated export header for ${LIBRARY_NAME}")
endfunction()
