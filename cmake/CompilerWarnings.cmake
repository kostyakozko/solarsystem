# CompilerWarnings.cmake
# Solar System Suite - Compiler Warning Configuration
# Ensures all warnings and warnings-as-errors are applied to ALL targets

# Function to configure warning flags for all targets
function(configure_solar_system_warnings)
    set(MSVC_WARNINGS
        /W4     # Baseline reasonable warnings
        /w14242 # 'identifier': conversion from 'type1' to 'type1', possible loss of data
        /w14254 # 'operator': conversion from 'type1:field_bits' to 'type1:field_bits'
        /w14263 # 'function': member function does not override any base class virtual member function
        /w14265 # 'classname': class has virtual functions, but destructor is not virtual
        /w14287 # 'operator': unsigned/negative constant mismatch
        /we4289 # nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside the for-loop scope
        /w14296 # 'operator': expression is always 'boolean_value'
        /w14311 # 'variable': pointer truncation from 'type1' to 'type2'
        /w14545 # expression before comma evaluates to a function which is missing an argument list
        /w14546 # function call before comma missing argument list
        /w14547 # 'operator': operator before comma has no effect; expected operator with side-effect
        /w14549 # 'operator': operator before comma has no effect; did you intend 'operator'?
        /w14555 # expression has no effect; expected expression with side-effect
        /w14619 # pragma warning: there is no warning number 'number'
        /w14640 # Enable warning on thread un-safe static member initialization
        /w14826 # Conversion from 'type1' to 'type_2' is sign-extended. This may cause unexpected runtime behavior.
        /w14905 # wide string literal cast to 'LPSTR'
        /w14906 # string literal cast to 'LPWSTR'
        /w14928 # illegal copy-initialization; more than one user-defined conversion has been implicitly applied
        /permissive- # standards conformance mode for MSVC compiler.
        /WX     # Treat warnings as errors
    )

    set(CLANG_WARNINGS
        -Wall
        -Wextra # reasonable and standard
        -Wshadow # warn the user if a variable declaration shadows one from a parent context
        -Wnon-virtual-dtor # warn the user if a class with virtual functions has a non-virtual destructor
        -Wold-style-cast # warn for c-style casts
        -Wcast-align # warn for potential performance problem casts
        -Wunused # warn on anything being unused
        -Woverloaded-virtual # warn if you overload (not override) a virtual function
        -Wpedantic # warn if non-standard C++ is used
        -Wconversion # warn on type conversions that may lose data
        -Wsign-conversion # warn on sign conversions
        -Wnull-dereference # warn if a null dereference is detected
        -Wdouble-promotion # warn if float is implicit promoted to double
        -Wformat=2 # warn on security issues around functions that format output (ie printf)
        -Wimplicit-fallthrough # warn on statements that fallthrough without an explicit annotation
        -Werror # Treat warnings as errors
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        set(PROJECT_WARNINGS_CXX ${CLANG_WARNINGS})
        # Clang-specific: disable unused lambda capture warning in tests
        list(APPEND PROJECT_WARNINGS_CXX -Wno-unused-lambda-capture)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(PROJECT_WARNINGS_CXX ${CLANG_WARNINGS})
        list(APPEND PROJECT_WARNINGS_CXX
            -Wmisleading-indentation # warn if indentation implies blocks where blocks do not exist
            -Wduplicated-cond # warn if if / else chain has duplicated conditions
            -Wduplicated-branches # warn if if / else branches have duplicated code
            -Wlogical-op # warn about logical operations being used where bitwise were probably wanted
            -Wuseless-cast # warn if you perform a cast to the same type
        )
        # GCC 13+ has false positives with -Werror for these warnings in standard library and Google Test
        if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0)
            list(APPEND PROJECT_WARNINGS_CXX
                -Wno-null-dereference
                -Wno-conversion
                -Wno-float-conversion
                -Wno-sign-conversion
                -Wno-useless-cast
                -Wno-type-limits
            )
            message(STATUS "GCC 13+ detected: disabling problematic warnings")
        endif()
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(PROJECT_WARNINGS_CXX ${MSVC_WARNINGS})
    endif()

    # Store warning flags for use by all targets
    set(SOLAR_SYSTEM_WARNING_FLAGS ${PROJECT_WARNINGS_CXX} CACHE INTERNAL "Warning flags for all targets")

    # Apply globally via CMAKE_CXX_FLAGS (most reliable method)
    string(JOIN " " WARNING_FLAGS_STRING ${PROJECT_WARNINGS_CXX})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS_STRING}" PARENT_SCOPE)

    # Also apply via add_compile_options for any targets that might override CMAKE_CXX_FLAGS
    add_compile_options(${PROJECT_WARNINGS_CXX})

    message(STATUS "Warnings as errors enabled for ALL targets")
    message(STATUS "Applied warning flags: ${WARNING_FLAGS_STRING}")
endfunction()

# Function to apply warnings to a specific target (for manual application if needed)
function(apply_solar_system_warnings target_name)
    if(DEFINED SOLAR_SYSTEM_WARNING_FLAGS)
        target_compile_options(${target_name} PRIVATE ${SOLAR_SYSTEM_WARNING_FLAGS})
        message(STATUS "Applied warnings to target: ${target_name}")
    else()
        message(WARNING "Warning flags not configured. Call configure_solar_system_warnings() first.")
    endif()
endfunction()
