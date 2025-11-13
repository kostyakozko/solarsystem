# Solar System Suite Testing Guide

## Overview

This guide provides comprehensive documentation for developing, executing, and maintaining tests in the Solar System Suite project.

## Table of Contents

1. [Test Development Guidelines](#test-development-guidelines)
2. [Test Execution Procedures](#test-execution-procedures)
3. [Troubleshooting Guide](#troubleshooting-guide)
4. [Test Framework API](#test-framework-api)

---

## Test Development Guidelines

### Writing Unit Tests

Unit tests should be focused, fast, and isolated. Follow these best practices:

```cpp
TEST_CASE("Descriptive Test Name") {
    // Arrange: Set up test data
    MyClass obj;

    // Act: Execute the functionality
    auto result = obj.do_something();

    // Assert: Verify the results
    ASSERT_TRUE(result);
    ASSERT_EQ(obj.get_value(), 42);
}
```

### Test Naming Conventions

- Use descriptive names that explain what is being tested
- Format: `test_<component>_<functionality>.cpp`
- Test case names should be clear and specific
- Example: `TEST_CASE("CelestialBody Position Calculation")`

### Test Organization

```
tests/
├── unit/           # Unit tests for individual components
├── integration/    # Integration tests for workflows
├── benchmarks/     # Performance benchmarks
└── utils/          # Test utilities and helpers
```

### Assertions

Available assertion macros:

- `ASSERT_TRUE(condition)` - Verify condition is true
- `ASSERT_FALSE(condition)` - Verify condition is false
- `ASSERT_EQ(a, b)` - Verify equality
- `ASSERT_NE(a, b)` - Verify inequality
- `ASSERT_LT(a, b)` - Verify less than
- `ASSERT_LE(a, b)` - Verify less than or equal
- `ASSERT_GT(a, b)` - Verify greater than
- `ASSERT_GE(a, b)` - Verify greater than or equal

### Test Isolation

Each test should be independent:

```cpp
TEST_CASE("Independent Test") {
    // Create fresh test data
    TestData data = create_test_data();

    // Test doesn't depend on other tests
    auto result = process(data);

    // Clean up is automatic
    ASSERT_TRUE(result.is_valid());
}
```

---

## Test Execution Procedures

### Running All Tests

```bash
# Build tests
cmake --build build -j$(nproc)

# Run all tests
ctest --test-dir build --output-on-failure

# Run with verbose output
ctest --test-dir build --verbose
```

### Running Specific Tests

```bash
# Run tests matching a pattern
ctest --test-dir build -R "UnitTest_*"

# Run tests with specific label
ctest --test-dir build -L "unit"

# Run a single test
ctest --test-dir build -R "UnitTest_CelestialBody"
```

### Running Tests by Phase

```bash
# Phase 1: Unit tests
ctest --test-dir build -L "phase1"

# Phase 7: Security tests
ctest --test-dir build -L "phase7"

# Phase 8: Concurrency tests
ctest --test-dir build -L "phase8"
```

### Test Labels

Tests are organized with labels:

- `unit` - Unit tests
- `integration` - Integration tests
- `benchmark` - Performance tests
- `security` - Security tests
- `concurrency` - Concurrency tests
- `phase1` through `phase12` - Test phases
- `task1` through `task35` - Specific tasks

### Continuous Integration

Tests run automatically on:

- Every commit (unit tests)
- Pull requests (full test suite)
- Nightly builds (including benchmarks)

---

## Troubleshooting Guide

### Common Issues

#### Test Compilation Errors

**Problem**: Tests fail to compile

**Solution**:
```bash
# Clean build directory
rm -rf build
mkdir build
cd build

# Reconfigure with testing enabled
cmake -DENABLE_TESTING=ON ..

# Rebuild
cmake --build . -j$(nproc)
```

#### Test Failures

**Problem**: Tests fail unexpectedly

**Diagnosis**:
```bash
# Run with verbose output
ctest --test-dir build --verbose --output-on-failure

# Run specific failing test
./build/tests/unit/test_name
```

**Common causes**:
- Race conditions in concurrent tests
- Platform-specific behavior
- Resource constraints
- Timing-dependent tests

#### Flaky Tests

**Problem**: Tests pass/fail intermittently

**Solution**:
- Add proper synchronization for concurrent tests
- Increase timeouts for slow operations
- Use deterministic test data
- Avoid timing-dependent assertions

#### Memory Issues

**Problem**: Tests crash or show memory errors

**Diagnosis**:
```bash
# Run with address sanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address" ..
cmake --build . -j$(nproc)
ctest --test-dir build
```

### Performance Issues

**Problem**: Tests run slowly

**Solutions**:
- Run tests in parallel: `ctest -j$(nproc)`
- Reduce test data size
- Mock expensive operations
- Use test fixtures for setup/teardown

### Debugging Tests

```bash
# Run test under debugger
lldb ./build/tests/unit/test_name

# Set breakpoint
(lldb) b test_file.cpp:123

# Run
(lldb) run

# Inspect variables
(lldb) p variable_name
```

---

## Test Framework API

### Test Suite Definition

```cpp
TEST_SUITE("Suite Name");
```

Defines a test suite. All tests in the file belong to this suite.

### Test Case Definition

```cpp
TEST_CASE("Test Case Name") {
    // Test code here
}
```

Defines an individual test case within the suite.

### Assertions

#### Boolean Assertions

```cpp
ASSERT_TRUE(condition);   // Fails if condition is false
ASSERT_FALSE(condition);  // Fails if condition is true
```

#### Equality Assertions

```cpp
ASSERT_EQ(actual, expected);  // Fails if not equal
ASSERT_NE(actual, expected);  // Fails if equal
```

#### Comparison Assertions

```cpp
ASSERT_LT(a, b);  // Fails if a >= b
ASSERT_LE(a, b);  // Fails if a > b
ASSERT_GT(a, b);  // Fails if a <= b
ASSERT_GE(a, b);  // Fails if a < b
```

### Test Utilities

#### Test Data Generation

```cpp
#include "test_utils.h"

// Generate test celestial body
auto body = create_test_body("Earth", 5.972e24, 6371000.0);

// Generate test ephemeris data
auto ephemeris = create_test_ephemeris(body, start_time, end_time);
```

#### Mock Objects

```cpp
// Create mock JPL client
MockJPLClient mock_client;
mock_client.set_response(test_data);

// Use in tests
auto result = system_under_test.fetch_data(mock_client);
```

### Performance Testing

```cpp
TEST_CASE("Performance Test") {
    auto start = std::chrono::steady_clock::now();

    // Code to benchmark
    perform_operation();

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Assert performance requirement
    ASSERT_LT(duration.count(), 1000);  // Must complete in < 1 second
}
```

### Concurrent Testing

```cpp
TEST_CASE("Thread Safety Test") {
    ThreadSafeObject obj;
    std::vector<std::thread> threads;

    // Launch concurrent operations
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&obj]() {
            obj.thread_safe_operation();
        });
    }

    // Wait for completion
    for (auto& t : threads) {
        t.join();
    }

    // Verify consistency
    ASSERT_TRUE(obj.is_consistent());
}
```

---

## Best Practices

### DO

✅ Write tests first (TDD)
✅ Keep tests simple and focused
✅ Use descriptive test names
✅ Test edge cases and error conditions
✅ Make tests independent
✅ Use appropriate assertions
✅ Clean up resources
✅ Document complex test scenarios

### DON'T

❌ Write tests that depend on execution order
❌ Use hardcoded paths or system-specific values
❌ Ignore test failures
❌ Write tests without assertions
❌ Test implementation details
❌ Create overly complex test setups
❌ Leave commented-out test code

---

## Contributing

When adding new tests:

1. Follow the existing test structure
2. Add appropriate labels in CMakeLists.txt
3. Ensure tests pass locally before committing
4. Update this documentation if adding new patterns
5. Run the full test suite: `ctest --test-dir build`

---

## Additional Resources

- [CMake Testing Documentation](https://cmake.org/cmake/help/latest/manual/ctest.1.html)
- [C++ Testing Best Practices](https://google.github.io/googletest/primer.html)
- Project README: `../README.md`
- Test Framework Source: `tests/utils/test_framework.h`

---

**Last Updated**: 2025-11-13
**Version**: 4.0.0
