# Implementation Plan - Build System and Dependency Modernization

## Task Overview

This implementation plan systematically modernizes the Solar System Suite's build system to support shared libraries and integrates industry-standard external dependencies. The plan follows a phased approach to minimize disruption and maintain backward compatibility throughout the migration.

### Phase 1: Build Infrastructure Setup

- [x] 1. Add CMake shared library support
  - Add BUILD_SHARED_LIBS option to root CMakeLists.txt
  - Configure library type selection (STATIC/SHARED)
  - Set up version properties (VERSION, SOVERSION)
  - Add RPATH/RUNPATH configuration for runtime loading
  - Test basic shared library builds on all platforms
  - _Requirements: 1.1, 1.2, 1.4_

- [x] 1.1 Configure root CMakeLists.txt
  - Add BUILD_SHARED_LIBS option with default OFF
  - Add USE_SYSTEM_GTEST and USE_SYSTEM_JSON options
  - Add ENABLE_SYMBOL_VISIBILITY option
  - Set up project versioning variables (MAJOR.MINOR.PATCH)
  - Configure RPATH settings for development and install
  - _Requirements: 1.1, 5.1_

- [x] 1.2 Update library CMakeLists.txt files
  - Modify lib/solar_core/CMakeLists.txt for shared/static selection
  - Modify lib/solar_jpl/CMakeLists.txt for shared/static selection
  - Modify lib/solar_utils/CMakeLists.txt for shared/static selection
  - Modify lib/solar_test/CMakeLists.txt for shared/static selection
  - Set VERSION and SOVERSION properties for shared libraries
  - _Requirements: 1.2, 6.3_

- [x] 1.3 Configure platform-specific settings
  - Add macOS install_name configuration
  - Add Linux SONAME configuration
  - Add Windows DLL output directory settings
  - Configure platform-appropriate library installation paths
  - _Requirements: 1.5, 9.1, 9.2, 9.3_

- [x] 2. Create symbol visibility infrastructure
  - Create export header template
  - Generate export headers for each library
  - Configure compiler visibility settings
  - Test symbol exports on all platforms
  - Document symbol visibility guidelines
  - _Requirements: 2.1, 2.2, 2.3, 2.4, 2.5_

- [x] 2.1 Create export header template
  - Create cmake/export_header_template.hpp.in
  - Add platform detection (Windows, Unix)
  - Add export/import macros for Windows (__declspec)
  - Add visibility attributes for GCC/Clang (__attribute__)
  - Add class and function export macros
  - _Requirements: 2.2, 2.3_

- [x] 2.2 Generate library-specific export headers
  - Generate lib/solar_core/include/solar_core/export.hpp (SOLAR_CORE_API)
  - Generate lib/solar_jpl/include/solar_jpl/export.hpp (SOLAR_JPL_API)
  - Generate lib/solar_utils/include/solar_utils/export.hpp (SOLAR_UTILS_API)
  - Generate lib/solar_test/include/solar_test/export.hpp (SOLAR_TEST_API)
  - Add export headers to CMake configuration
  - _Requirements: 2.1, 2.2_

- [x] 2.3 Configure compiler visibility settings
  - Set CXX_VISIBILITY_PRESET to hidden for shared libraries
  - Set VISIBILITY_INLINES_HIDDEN for shared libraries
  - Add -fvisibility=hidden to compiler flags (GCC/Clang)
  - Configure MSVC export settings
  - _Requirements: 2.1, 2.4_

- [x] 3. Audit and mark public API
  - Identify public API classes and functions
  - Add export macros to public headers
  - Verify internal symbols are hidden
  - Test symbol exports with nm/objdump
  - Document public API surface
  - _Requirements: 2.1, 2.2, 2.5_

- [x] 3.1 Audit solar_core public API
  - Review all public headers in solar_core/include/
  - Mark public classes with SOLAR_CORE_API
  - Mark public functions with SOLAR_CORE_API
  - Verify template classes don't need export (header-only)
  - Test that internal implementation details are hidden
  - _Requirements: 2.1, 2.5_

- [x] 3.2 Audit solar_jpl public API
  - Review all public headers in solar_jpl/include/
  - Mark JPLClient and related classes with SOLAR_JPL_API
  - Mark public utility functions with SOLAR_JPL_API
  - Ensure cache and internal classes are not exported
  - _Requirements: 2.1, 2.5_

- [x] 3.3 Audit solar_utils public API
  - Review all public headers in solar_utils/include/
  - Mark utility classes with SOLAR_UTILS_API
  - Mark validation and error handling functions with SOLAR_UTILS_API
  - Keep internal helpers unexported
  - _Requirements: 2.1, 2.5_

- [x] 3.4 Audit solar_test public API
  - Review test framework headers
  - Mark test utilities with SOLAR_TEST_API
  - Mark mock classes with SOLAR_TEST_API
  - Ensure test internals are hidden
  - _Requirements: 2.1, 2.5_

### Phase 2: External Dependencies Integration

- [ ] 4. Integrate Google Test
  - Add Google Test via FetchContent or find_package
  - Create test compatibility layer
  - Test Google Test integration
  - Update test CMakeLists.txt files
  - Document Google Test usage
  - _Requirements: 3.1, 3.2, 3.3, 3.4, 3.5_

- [x] 4.1 Add Google Test to build system
  - Add FetchContent configuration for Google Test in tests/CMakeLists.txt
  - Add USE_SYSTEM_GTEST option for system installations
  - Configure find_package fallback for system Google Test
  - Set up gtest_discover_tests for automatic test discovery
  - _Requirements: 3.1, 3.5_

- [ ] 4.2 Create test compatibility layer
  - Create tests/utils/gtest_compat.hpp
  - Map TEST_CASE to TEST macro
  - Map ASSERT_* macros to EXPECT_*/ASSERT_* macros
  - Map test suite macros to Google Test equivalents
  - Document compatibility layer usage
  - _Requirements: 3.2, 10.2_

- [ ] 4.3 Update test build configuration
  - Link test executables with GTest::gtest_main
  - Remove dependencies on custom test framework
  - Enable Google Test discovery in CMake
  - Configure test timeout and parallel execution
  - _Requirements: 3.1, 3.4_

- [ ] 5. Integrate nlohmann/json
  - Add nlohmann/json via FetchContent or find_package
  - Create JSON migration utilities
  - Test JSON integration
  - Document JSON usage patterns
  - _Requirements: 4.1, 4.2, 4.3, 4.4, 4.5_

- [ ] 5.1 Add nlohmann/json to build system
  - Add FetchContent configuration for nlohmann/json
  - Add USE_SYSTEM_JSON option for system installations
  - Configure find_package fallback
  - Link libraries that use JSON with nlohmann_json::nlohmann_json
  - _Requirements: 4.1, 4.5_

- [ ] 5.2 Create JSON migration utilities
  - Create wrapper functions for common JSON operations
  - Add error handling helpers for JSON exceptions
  - Create conversion utilities from old JSON code
  - Document migration patterns
  - _Requirements: 4.2, 4.3, 4.4, 10.2_

### Phase 3: Package Configuration

- [ ] 6. Generate pkg-config files
  - Create .pc.in templates for each library
  - Configure pkg-config file generation
  - Install pkg-config files
  - Test pkg-config integration
  - _Requirements: 6.1, 6.2_

- [ ] 6.1 Create pkg-config templates
  - Create lib/solar_core/solar_core.pc.in
  - Create lib/solar_jpl/solar_jpl.pc.in
  - Create lib/solar_utils/solar_utils.pc.in
  - Add version, dependencies, and flags to templates
  - _Requirements: 6.1_

- [ ] 6.2 Configure pkg-config installation
  - Add configure_file() for each .pc template
  - Install .pc files to ${CMAKE_INSTALL_LIBDIR}/pkgconfig
  - Test pkg-config --cflags and --libs output
  - _Requirements: 6.1, 6.2_

- [ ] 7. Generate CMake config files
  - Create CMake config templates
  - Generate version files
  - Export targets
  - Install CMake configs
  - Test find_package integration
  - _Requirements: 6.2, 6.3_

- [ ] 7.1 Create CMake config templates
  - Create cmake/SolarSystemConfig.cmake.in
  - Add find_dependency() calls for required dependencies
  - Include exported targets file
  - Add version compatibility checking
  - _Requirements: 6.2_

- [ ] 7.2 Configure CMake package installation
  - Use configure_package_config_file() for config generation
  - Use write_basic_package_version_file() for version file
  - Export library targets with install(EXPORT)
  - Install config files to lib/cmake/SolarSystem
  - _Requirements: 6.2, 6.3_

### Phase 4: Test Migration

- [ ] 8. Migrate unit tests to Google Test
  - Convert test files one by one
  - Use compatibility layer initially
  - Verify test results match
  - Remove compatibility layer usage
  - _Requirements: 3.2, 3.3, 10.1, 10.4_

- [ ] 8.1 Convert core library tests
  - Convert tests/unit/test_simulation*.cpp to Google Test
  - Convert tests/unit/test_body*.cpp to Google Test
  - Convert tests/unit/test_math*.cpp to Google Test
  - Verify all tests pass with Google Test
  - _Requirements: 3.2, 10.4_

- [ ] 8.2 Convert JPL library tests
  - Convert tests/unit/test_jpl*.cpp to Google Test
  - Convert tests/unit/test_cache*.cpp to Google Test
  - Verify JPL tests pass with Google Test
  - _Requirements: 3.2, 10.4_

- [ ] 8.3 Convert utility library tests
  - Convert tests/unit/test_utils*.cpp to Google Test
  - Convert tests/unit/test_validation*.cpp to Google Test
  - Verify utility tests pass with Google Test
  - _Requirements: 3.2, 10.4_

- [ ] 9. Migrate integration tests to Google Test
  - Convert integration test files
  - Update test fixtures and helpers
  - Verify integration tests pass
  - _Requirements: 3.2, 3.3, 10.4_

- [ ] 9.1 Convert integration tests
  - Convert tests/integration/test_*.cpp to Google Test
  - Update test data managers for Google Test
  - Convert test utilities to Google Test fixtures
  - Verify all integration tests pass
  - _Requirements: 3.2, 10.4_

- [ ] 10. Migrate benchmark tests to Google Test
  - Convert benchmark files
  - Integrate with Google Benchmark (optional)
  - Verify benchmarks run correctly
  - _Requirements: 3.2, 10.4_

### Phase 5: JSON Migration

- [ ] 11. Replace custom JSON with nlohmann/json
  - Identify all custom JSON code
  - Replace with nlohmann/json
  - Test JSON operations
  - Remove custom JSON utilities
  - _Requirements: 4.2, 4.3, 4.4, 10.1, 10.4_

- [ ] 11.1 Migrate JSON in solar_core
  - Replace manual JSON building in output formatters
  - Replace JSON parsing in configuration loaders
  - Use nlohmann::json for serialization
  - Test all JSON operations
  - _Requirements: 4.2, 4.3, 10.4_

- [ ] 11.2 Migrate JSON in solar_jpl
  - Replace JSON cache format handling
  - Use nlohmann::json for JPL response parsing
  - Update cache serialization/deserialization
  - _Requirements: 4.2, 4.3, 10.4_

- [ ] 11.3 Migrate JSON in web server
  - Replace manual JSON building in API endpoints
  - Use nlohmann::json for request/response handling
  - Update error response formatting
  - _Requirements: 4.2, 4.3, 10.4_

- [ ] 11.4 Migrate JSON in test framework
  - Replace JSON in test reporters
  - Use nlohmann::json for test result serialization
  - Update CI artifact generation
  - _Requirements: 4.2, 4.3, 10.4_

### Phase 6: Validation and Documentation

- [ ] 12. Cross-platform validation
  - Test shared libraries on macOS
  - Test shared libraries on Linux
  - Test shared libraries on Windows
  - Verify symbol exports on all platforms
  - Test installation on all platforms
  - _Requirements: 9.1, 9.2, 9.3, 9.4_

- [ ] 12.1 macOS validation
  - Build shared libraries (.dylib)
  - Verify install_name with otool -L
  - Test RPATH resolution
  - Verify framework build (optional)
  - Test installation and pkg-config
  - _Requirements: 9.1, 9.4_

- [ ] 12.2 Linux validation
  - Build shared libraries (.so)
  - Verify SONAME with readelf
  - Test RPATH resolution with ldd
  - Verify symbol visibility with nm
  - Test installation and pkg-config
  - _Requirements: 9.2, 9.4_

- [ ] 12.3 Windows validation
  - Build DLLs and import libraries
  - Verify exports with dumpbin
  - Test DLL loading and PATH resolution
  - Verify all applications run with DLLs
  - Test installation
  - _Requirements: 9.3, 9.4_

- [ ] 13. Performance benchmarking
  - Benchmark static vs shared library performance
  - Measure link time improvements
  - Measure startup time differences
  - Measure memory usage
  - Document performance characteristics
  - _Requirements: 7.1, 7.2, 7.3, 7.4_

- [ ] 13.1 Runtime performance benchmarks
  - Run simulation benchmarks with static libraries
  - Run simulation benchmarks with shared libraries
  - Compare performance (should be within 5%)
  - Document PIC overhead if significant
  - _Requirements: 7.1, 7.3, 7.4_

- [ ] 13.2 Build performance benchmarks
  - Measure clean build time (static vs shared)
  - Measure incremental build time (static vs shared)
  - Measure link time for applications
  - Document build time improvements (expect 50%+ for incremental)
  - _Requirements: 7.1, 8.1, 8.2_

- [ ] 14. Update documentation
  - Update README.md with build options
  - Create migration guide
  - Update developer documentation
  - Document API stability guarantees
  - Create troubleshooting guide
  - _Requirements: 10.3_

- [ ] 14.1 Update build documentation
  - Document BUILD_SHARED_LIBS option
  - Document external dependency options
  - Add platform-specific build instructions
  - Document RPATH troubleshooting
  - Add examples of using installed libraries
  - _Requirements: 10.3_

- [ ] 14.2 Create migration guide
  - Document migrating from static to shared builds
  - Document test framework migration
  - Document JSON library migration
  - Provide code examples for common patterns
  - Add FAQ section
  - _Requirements: 10.3_

- [ ] 14.3 Update API documentation
  - Mark exported symbols in Doxygen comments
  - Document ABI stability policy
  - Document versioning scheme
  - Add examples of using public API
  - _Requirements: 6.4_

- [ ] 15. Update CI/CD pipelines
  - Add shared library builds to CI matrix
  - Test both static and shared builds
  - Add cross-platform testing
  - Update deployment scripts
  - _Requirements: 5.4, 9.4_

- [ ] 15.1 Update GitHub Actions workflows
  - Add BUILD_SHARED_LIBS=ON to test matrix
  - Add BUILD_SHARED_LIBS=OFF to test matrix
  - Test with system dependencies where available
  - Add Windows, macOS, Linux to matrix
  - _Requirements: 5.4, 9.4_

- [ ] 15.2 Update deployment configuration
  - Configure artifact uploads for shared libraries
  - Update installation scripts for shared libraries
  - Add library dependency packaging
  - Test deployment on all platforms
  - _Requirements: 9.4_

### Phase 7: Cleanup and Finalization

- [ ] 16. Remove custom test framework
  - Remove custom test framework code
  - Remove test compatibility layer
  - Update all test documentation
  - Verify all tests use Google Test
  - _Requirements: 3.2, 10.5_

- [ ] 16.1 Remove custom framework files
  - Remove tests/utils/test_framework.h
  - Remove tests/utils/test_framework.cpp
  - Remove custom assertion macros
  - Remove custom test discovery code
  - _Requirements: 10.5_

- [ ] 16.2 Remove compatibility layer
  - Remove tests/utils/gtest_compat.hpp
  - Verify no tests use compatibility macros
  - Update test documentation
  - _Requirements: 10.5_

- [ ] 17. Remove custom JSON utilities
  - Remove custom JSON parsing code
  - Remove manual JSON building utilities
  - Verify all code uses nlohmann/json
  - Update documentation
  - _Requirements: 4.2, 10.5_

- [ ] 18. Final validation
  - Run complete test suite (static build)
  - Run complete test suite (shared build)
  - Verify all applications work
  - Verify installation works
  - Verify pkg-config and CMake configs work
  - _Requirements: 5.4, 10.4_

## Implementation Guidelines

### Code Quality Standards
- All public API must have export macros
- Symbol visibility must be tested on all platforms
- ABI compatibility must be maintained within major versions
- Performance regression tests must pass
- All tests must pass with both static and shared builds

### Testing Requirements
- Test both BUILD_SHARED_LIBS=ON and OFF
- Test on macOS, Linux, and Windows
- Test with system dependencies and FetchContent
- Verify symbol exports with platform tools
- Performance benchmarks must show acceptable overhead

### Documentation Standards
- Document all CMake options
- Provide migration examples
- Document ABI stability guarantees
- Include troubleshooting guides
- Update all build instructions

### Performance Requirements
- Shared library runtime overhead < 5%
- Incremental build time improvement > 50%
- Memory overhead < 10MB per application
- Startup time increase < 100ms

## Success Criteria

### Functional Success
- All libraries build as shared or static based on BUILD_SHARED_LIBS
- All applications work with shared libraries
- Google Test integration complete and all tests migrated
- nlohmann/json integration complete
- pkg-config and CMake configs generated and working

### Quality Success
- 100% test pass rate with both static and shared builds
- All platforms (macOS, Linux, Windows) working
- Symbol visibility correct on all platforms
- No ABI compatibility issues
- Documentation complete and accurate

### Performance Success
- Runtime performance within 5% of static builds
- Incremental build time reduced by 50%+
- Memory overhead acceptable
- No performance regressions in benchmarks

