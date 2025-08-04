# CompilerSelection.cmake
# Solar System Suite - Cross-Platform Compiler Selection
# 
# Platform-specific compiler selection strategy:
# 1. macOS: Always use clang (both for gcc and clang commands)
# 2. Linux: Prefer clang, fallback to gcc if clang not available
# 3. GitHub Ubuntu workflows: Force gcc usage for consistency

function(configure_solar_system_compiler)
    # Skip if compilers are already explicitly set
    if(DEFINED CMAKE_C_COMPILER AND DEFINED CMAKE_CXX_COMPILER)
        message(STATUS "Compilers already set: ${CMAKE_C_COMPILER}, ${CMAKE_CXX_COMPILER}")
        return()
    endif()

    # Detect platform
    if(APPLE)
        # macOS: Always use clang (both gcc and clang commands point to clang on macOS)
        find_program(CLANG_C_COMPILER NAMES clang)
        find_program(CLANG_CXX_COMPILER NAMES clang++)
        
        if(CLANG_C_COMPILER AND CLANG_CXX_COMPILER)
            set(CMAKE_C_COMPILER ${CLANG_C_COMPILER} PARENT_SCOPE)
            set(CMAKE_CXX_COMPILER ${CLANG_CXX_COMPILER} PARENT_SCOPE)
            message(STATUS "macOS: Using clang compilers")
            message(STATUS "  C compiler: ${CLANG_C_COMPILER}")
            message(STATUS "  C++ compiler: ${CLANG_CXX_COMPILER}")
        else()
            message(WARNING "macOS: clang not found, using system default")
        endif()
        
    elseif(UNIX AND NOT APPLE)
        # Linux: Check for GitHub Actions environment variable
        if(DEFINED ENV{GITHUB_ACTIONS} AND "$ENV{GITHUB_ACTIONS}" STREQUAL "true")
            # GitHub Actions: Force gcc usage for Ubuntu builds
            find_program(GCC_C_COMPILER NAMES gcc)
            find_program(GCC_CXX_COMPILER NAMES g++)
            
            if(GCC_C_COMPILER AND GCC_CXX_COMPILER)
                set(CMAKE_C_COMPILER ${GCC_C_COMPILER} PARENT_SCOPE)
                set(CMAKE_CXX_COMPILER ${GCC_CXX_COMPILER} PARENT_SCOPE)
                message(STATUS "GitHub Actions Ubuntu: Using gcc compilers")
                message(STATUS "  C compiler: ${GCC_C_COMPILER}")
                message(STATUS "  C++ compiler: ${GCC_CXX_COMPILER}")
            else()
                message(WARNING "GitHub Actions: gcc not found, using system default")
            endif()
        else()
            # Regular Linux: Prefer clang, fallback to gcc
            find_program(CLANG_C_COMPILER NAMES clang)
            find_program(CLANG_CXX_COMPILER NAMES clang++)
            find_program(GCC_C_COMPILER NAMES gcc)
            find_program(GCC_CXX_COMPILER NAMES g++)
            
            if(CLANG_C_COMPILER AND CLANG_CXX_COMPILER)
                set(CMAKE_C_COMPILER ${CLANG_C_COMPILER} PARENT_SCOPE)
                set(CMAKE_CXX_COMPILER ${CLANG_CXX_COMPILER} PARENT_SCOPE)
                message(STATUS "Linux: Using clang compilers (preferred)")
                message(STATUS "  C compiler: ${CLANG_C_COMPILER}")
                message(STATUS "  C++ compiler: ${CLANG_CXX_COMPILER}")
            elseif(GCC_C_COMPILER AND GCC_CXX_COMPILER)
                set(CMAKE_C_COMPILER ${GCC_C_COMPILER} PARENT_SCOPE)
                set(CMAKE_CXX_COMPILER ${GCC_CXX_COMPILER} PARENT_SCOPE)
                message(STATUS "Linux: Using gcc compilers (clang not available)")
                message(STATUS "  C compiler: ${GCC_C_COMPILER}")
                message(STATUS "  C++ compiler: ${GCC_CXX_COMPILER}")
            else()
                message(WARNING "Linux: Neither clang nor gcc found, using system default")
            endif()
        endif()
        
    else()
        # Windows or other platforms: Use system default
        message(STATUS "Non-Unix platform: Using system default compilers")
    endif()
endfunction()

# Function to display compiler information after configuration
function(display_compiler_info)
    message(STATUS "Final compiler configuration:")
    message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
    message(STATUS "  C compiler: ${CMAKE_C_COMPILER}")
    message(STATUS "  C++ compiler: ${CMAKE_CXX_COMPILER}")
    message(STATUS "  C++ compiler ID: ${CMAKE_CXX_COMPILER_ID}")
    message(STATUS "  C++ compiler version: ${CMAKE_CXX_COMPILER_VERSION}")
    
    # Additional info for debugging
    if(DEFINED ENV{GITHUB_ACTIONS})
        message(STATUS "  GitHub Actions: $ENV{GITHUB_ACTIONS}")
    endif()
    
    # Verify compiler capabilities
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "  Compiler supports: C++20, optimization flags, LTO")
    else()
        message(WARNING "  Compiler may not support all optimization features")
    endif()
endfunction()
