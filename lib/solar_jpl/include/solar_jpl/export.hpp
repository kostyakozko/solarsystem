/**
 * @file solar_jpl_export.hpp
 * @brief Export/import macros for solar_jpl library
 *
 * This file provides platform-specific macros for controlling symbol visibility
 * in shared libraries. It handles Windows DLL export/import and Unix visibility
 * attributes.
 */

#ifndef SOLAR_JPL_EXPORT_HPP
#define SOLAR_JPL_EXPORT_HPP

// Platform detection
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
  #define SOLAR_JPL_PLATFORM_WINDOWS
#elif defined(__APPLE__) && defined(__MACH__)
  #define SOLAR_JPL_PLATFORM_MACOS
#elif defined(__linux__)
  #define SOLAR_JPL_PLATFORM_LINUX
#elif defined(__unix__) || defined(__unix)
  #define SOLAR_JPL_PLATFORM_UNIX
#endif

// Shared library detection
#if defined(SOLAR_JPL_STATIC_DEFINE)
  #define SOLAR_JPL_SHARED_LIBRARY 0
#else
  #define SOLAR_JPL_SHARED_LIBRARY 1
#endif

// Export/import macros for Windows
#if defined(SOLAR_JPL_PLATFORM_WINDOWS)
  #if SOLAR_JPL_SHARED_LIBRARY
    #if defined(SOLAR_JPL_EXPORTS)
      // Building the library - export symbols
      #define SOLAR_JPL_API __declspec(dllexport)
    #else
      // Using the library - import symbols
      #define SOLAR_JPL_API __declspec(dllimport)
    #endif
  #else
    // Static library - no export/import needed
    #define SOLAR_JPL_API
  #endif

  // Windows doesn't need visibility for local symbols
  #define SOLAR_JPL_LOCAL

// Export/import macros for GCC/Clang on Unix-like systems
#elif defined(__GNUC__) || defined(__clang__)
  #if SOLAR_JPL_SHARED_LIBRARY
    #if defined(SOLAR_JPL_EXPORTS)
      // Building the library - export symbols
      #define SOLAR_JPL_API __attribute__((visibility("default")))
    #else
      // Using the library - symbols are visible by default
      #define SOLAR_JPL_API __attribute__((visibility("default")))
    #endif
    // Mark internal symbols as hidden
    #define SOLAR_JPL_LOCAL __attribute__((visibility("hidden")))
  #else
    // Static library - no visibility attributes needed
    #define SOLAR_JPL_API
    #define SOLAR_JPL_LOCAL
  #endif

// Fallback for unknown compilers
#else
  #define SOLAR_JPL_API
  #define SOLAR_JPL_LOCAL
  #warning "Symbol visibility not supported on this compiler"
#endif

// Convenience macros for common use cases

/**
 * @brief Mark a class for export
 * Usage: class SOLAR_JPL_API MyClass { ... };
 */
#define SOLAR_JPL_CLASS_API SOLAR_JPL_API

/**
 * @brief Mark a function for export
 * Usage: SOLAR_JPL_FUNCTION_API void myFunction();
 */
#define SOLAR_JPL_FUNCTION_API SOLAR_JPL_API

/**
 * @brief Mark a template class for export (header-only, no export needed)
 * Usage: template<typename T> class SOLAR_JPL_TEMPLATE_API MyTemplate { ... };
 */
#define SOLAR_JPL_TEMPLATE_API

/**
 * @brief Mark internal implementation details (not part of public API)
 * Usage: class SOLAR_JPL_INTERNAL MyInternalClass { ... };
 */
#define SOLAR_JPL_INTERNAL SOLAR_JPL_LOCAL

/**
 * @brief Deprecated API marker
 * Usage: SOLAR_JPL_DEPRECATED("Use newFunction instead") void oldFunction();
 */
#if defined(__GNUC__) || defined(__clang__)
  #define SOLAR_JPL_DEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER)
  #define SOLAR_JPL_DEPRECATED(msg) __declspec(deprecated(msg))
#else
  #define SOLAR_JPL_DEPRECATED(msg)
#endif

#endif // SOLAR_JPL_EXPORT_HPP
