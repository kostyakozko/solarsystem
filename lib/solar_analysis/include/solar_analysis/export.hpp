/**
 * @file export.hpp
 * @brief Export/import macros for solar_analysis library
 */

#ifndef SOLAR_ANALYSIS_EXPORT_HPP
#define SOLAR_ANALYSIS_EXPORT_HPP

// Platform detection
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#define SOLAR_ANALYSIS_PLATFORM_WINDOWS
#elif defined(__APPLE__) && defined(__MACH__)
#define SOLAR_ANALYSIS_PLATFORM_MACOS
#elif defined(__linux__)
#define SOLAR_ANALYSIS_PLATFORM_LINUX
#endif

// Shared library detection
#if defined(SOLAR_ANALYSIS_STATIC_DEFINE)
#define SOLAR_ANALYSIS_SHARED_LIBRARY 0
#else
#define SOLAR_ANALYSIS_SHARED_LIBRARY 1
#endif

// Export/import macros
#if defined(SOLAR_ANALYSIS_PLATFORM_WINDOWS)
#if SOLAR_ANALYSIS_SHARED_LIBRARY
#if defined(SOLAR_ANALYSIS_EXPORTS)
#define SOLAR_ANALYSIS_API __declspec(dllexport)
#else
#define SOLAR_ANALYSIS_API __declspec(dllimport)
#endif
#else
#define SOLAR_ANALYSIS_API
#endif
#define SOLAR_ANALYSIS_LOCAL

#elif defined(__GNUC__) || defined(__clang__)
#if SOLAR_ANALYSIS_SHARED_LIBRARY && defined(SOLAR_ANALYSIS_EXPORTS)
#define SOLAR_ANALYSIS_API __attribute__((visibility("default")))
#define SOLAR_ANALYSIS_LOCAL __attribute__((visibility("hidden")))
#else
#define SOLAR_ANALYSIS_API
#define SOLAR_ANALYSIS_LOCAL
#endif

#else
#define SOLAR_ANALYSIS_API
#define SOLAR_ANALYSIS_LOCAL
#endif

#endif  // SOLAR_ANALYSIS_EXPORT_HPP
