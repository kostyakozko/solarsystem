# Solar System Testing Framework Troubleshooting Guide

This guide helps developers diagnose and resolve common issues when working with the Solar System Testing Framework.

## Table of Contents

1. [Test Discovery Issues](#test-discovery-issues)
2. [Test Execution Problems](#test-execution-problems)
3. [Mock-Related Issues](#mock-related-issues)
4. [Performance Test Problems](#performance-test-problems)
5. [CI/CD Integration Issuesegration-issues)
6. [Memory and Resource Issues](#memory-and-resource-issues)
7. [Build and Compilation Problems](#build-and-compilation-problems)
8. [Common Error Messages](#common-error-messages)

## Test Discovery Issues

### Problem: Tests Not Found by CTest

**Symptoms:**
```bash
$ ctest --show-only
Test project /path/to/build
No tests were found!!!
```

**Diagnosis:**
```bash
# Check if testing is enabled
cmake -L | grep ENABLE_TESTING

# Check if test executables exist
ls -la tests/unit/
ls -la tests/integration/
ls -la tests/benchmarks/

# Check CMake configuration
cmake --build . --target help | grep test
```

**Solutions:**

1. **Enable testing in CMake:**
```bash
cmake -B build -DENABLE_TESTING=ON
cmake --build build
```

2. **Check CMakeLists.txt files:**
```cmake
# In tests/CMakeLists.txt
enable_testing()

# In test subdirectories
add_test(NAME TestName COMMAND test_executable)
```

3. **Verify test registration:**
```cpp
// In test files, ensure tests are registered
REGISTER_TEST(TestClassName);

// Or use automatic registration
SOLAR_TEST_MAIN()
```

4. **Check build output:**
```bash
cmake --build build --verbose
# Look for test executable compilation
```

### Problem: Tests Found But Not Categorized

**Symptoms:**
```bash
$ ctest -L "unit"
No tests were found!!!

$ ctest --show-only
# Shows tests but without labels
```

**Solutions:**

1. **Add test labels in CMake:**
```cmake
add_test(NAME TestName COMMAND test_executable)
set_tests_properties(TestName PROPERTIES
    LABELS "unit;fast"
    TIMEOUT 60
)
```

2. **Verify test tags in code:**
```cpp
class MyTest : public SolarSystem::Testing::TestCase {
public:
  MyTest() : TestCase({
    "MyTest",
    "Test description",
    {"unit", "fast", "component_name"}  // Tags here
  }) {}
};
```

3. **Check available labels:**
```bash
ctest --print-labels
```

## Test Execution Problems

### Problem: Tests Fail with Segmentation Fault

**Symptoms:**
```bash
$ ctest -R "TestName"
Segmentation fault (core dumped)
```

**Diagnosis:**
```bash
# Run with debugger
gdb ./tests/unit/test_name
(gdb) run
(gdb) bt  # Get backtrace when it crashes

# Run with Valgrind
valgrind --tool=memcheck --leak-check=full ./tests/unit/test_name

# Enable core dumps
ulimit -c unlimited
```

**Common Causes and Solutions:**

1. **Null pointer dereference:**
```cpp
// Bad
auto ptr = get_pointer();
ptr->method();  // ptr might be null

// Good
auto ptr = get_pointer();
assert_not_equals(nullptr, ptr, "Pointer should not be null");
ptr->method();
```

2. **Use after free:**
```cpp
// Bad
std::unique_ptr<Object> obj = std::make_unique<Object>();
auto raw_ptr = obj.get();
obj.reset();  // Object destroyed
raw_ptr->method();  // Use after free

// Good
std::unique_ptr<Object> obj = std::make_unique<Object>();
obj->method();  // Use before destruction
```

3. **Stack overflow from infinite recursion:**
```cpp
// Check for infinite recursion in test setup
void setup() override {
  // Make sure this doesn't call setup() again
  initialize_test_data();
}
```

### Problem: Tests Hang or Timeout

**Symptoms:**
```bash
$ ctest -R "TestName"
# Test runs but never completes
# Or: Test timeout reached
```

**Diagnosis:**
```bash
# Run with timeout
timeout 30s ./tests/unit/test_name

# Check for deadlocks
gdb ./tests/unit/test_name
(gdb) run
# When it hangs, press Ctrl+C
(gdb) bt
(gdb) info threads
```

**Common Causes and Solutions:**

1. **Infinite loops:**
```cpp
// Bad
while (condition) {
  // condition never changes
  process_data();
}

// Good
int max_iterations = 1000;
int iteration = 0;
while (condition && iteration < max_iterations) {
  process_data();
  ++iteration;
}
assert_less_than(iteration, max_iterations, "Loop should terminate");
```

2. **Deadlocks in parallel tests:**
```cpp
// Bad
std::mutex mutex1, mutex2;
// Thread 1: lock mutex1, then mutex2
// Thread 2: lock mutex2, then mutex1

// Good - consistent lock ordering
std::lock(mutex1, mutex2);
std::lock_guard<std::mutex> lock1(mutex1, std::adopt_lock);
std::lock_guard<std::mutex> lock2(mutex2, std::adopt_lock);
```

3. **Waiting for external resources:**
```cpp
// Bad - waiting indefinitely
while (!external_service.is_ready()) {
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// Good - with timeout
auto start = std::chrono::steady_clock::now();
auto timeout = std::chrono::seconds(30);
while (!external_service.is_ready()) {
  if (std::chrono::steady_clock::now() - start > timeout) {
    skip_test("External service not available");
    return;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
```

### Problem: Flaky Tests (Intermittent Failures)

**Symptoms:**
- Tests pass sometimes, fail other times
- Different results on different machines
- Failures in CI but not locally

**Diagnosis:**
```bash
# Run test multiple times
for i in {1..100}; do
  echo "Run $i"
  ctest -R "FlakyTest" || echo "Failed on run $i"
done

# Run with different thread counts
ctest -j 1  # Single threaded
ctest -j 4  # Multi-threaded
```

**Common Causes and Solutions:**

1. **Race conditions:**
```cpp
// Bad
std::vector<int> shared_data;
std::thread t1([&]() { shared_data.push_back(1); });
std::thread t2([&]() { shared_data.push_back(2); });
t1.join();
t2.join();
// Order of elements is non-deterministic

// Good
std::vector<int> shared_data;
std::mutex data_mutex;
std::thread t1([&]() {
  std::lock_guard<std::mutex> lock(data_mutex);
  shared_data.push_back(1);
});
std::thread t2([&]() {
  std::lock_guard<std::mutex> lock(data_mutex);
  shared_data.push_back(2);
});
t1.join();
t2.join();
```

2. **Time-dependent tests:**
```cpp
// Bad
auto start = std::chrono::high_resolution_clock::now();
perform_operation();
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
assert_equals(100, duration.count(), "Should take exactly 100ms");

// Good
auto start = std::chrono::high_resolution_clock::now();
perform_operation();
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
assert_greater_than(duration.count(), 90, "Should take at least 90ms");
assert_less_than(duration.count(), 200, "Should take at most 200ms");
```

3. **Uninitialized variables:**
```cpp
// Bad
int value;  // Uninitialized
assert_equals(0, value, "Value should be zero");

// Good
int value = 0;  // Explicitly initialized
assert_equals(0, value, "Value should be zero");
```

## Mock-Related Issues

### Problem: Mock Not Being Called

**Symptoms:**
```bash
Assertion failed: Expected mock to be called 1 times, but was called 0 times
```

**Diagnosis:**
```cpp
// Add debug output to verify mock setup
auto mock = std::make_shared<MockService>();
mock->set_response("test_data");

Component component(mock);  // Make sure mock is passed correctly
auto result = component.process();

// Check if mock was configured correctly
std::cout << "Mock call count: " << mock->call_count() << std::endl;
std::cout << "Mock was configured: " << mock->is_configured() << std::endl;
```

**Solutions:**

1. **Verify mock is passed to component:**
```cpp
// Bad
auto mock = std::make_shared<MockService>();
Component component;  // Uses default service, not mock

// Good
auto mock = std::make_shared<MockService>();
Component component(mock);  // Explicitly pass mock
```

2. **Check component actually uses the service:**
```cpp
// In component implementation, make sure service is called
class Component {
  std::shared_ptr<IService> service_;
public:
  Component(std::shared_ptr<IService> service) : service_(service) {}

  void process() {
    // Make sure this line exists and is executed
    auto data = service_->get_data();
  }
};
```

3. **Verify mock interface matches:**
```cpp
// Make sure mock implements the same interface
class MockService : public IService {  // Must inherit from same interface
public:
  // Must implement all pure virtual methods
  std::string get_data() override {
    call_count_++;
    return configured_response_;
  }
};
```

### Problem: Mock Returns Unexpected Values

**Symptoms:**
```bash
Assertion failed: Expected "expected_value", but got "default_value"
```

**Solutions:**

1. **Configure mock before use:**
```cpp
// Configure mock BEFORE passing to component
auto mock = std::make_shared<MockService>();
mock->set_response("expected_value");  // Configure first

Component component(mock);  // Then pass to component
auto result = component.process();
```

2. **Check mock method names:**
```cpp
// Make sure method names match exactly
mock->set_response_for_method("get_data", "value");  // Exact method name
// Not: set_response_for_method("getData", "value");
```

3. **Verify mock state:**
```cpp
// Check mock configuration
assert_true(mock->is_configured(), "Mock should be configured");
assert_equals("expected_value", mock->get_configured_response(),
              "Mock should have correct response configured");
```

## Performance Test Problems

### Problem: Performance Tests Are Unstable

**Symptoms:**
- Performance tests pass sometimes, fail other times
- Large variations in execution time
- Different results on different machines

**Solutions:**

1. **Use statistical measures:**
```cpp
// Bad - single measurement
auto start = std::chrono::high_resolution_clock::now();
operation();
auto end = std::chrono::high_resolution_clock::now();
auto duration = end - start;
assert_less_than(duration.count(), threshold, "Should be fast");

// Good - multiple measurements with statistics
Benchmark benchmark("Operation");
auto result = benchmark.measure([&]() {
  operation();
}, 1000);  // 1000 iterations

// Use statistical measures
assert_less_than(result.mean_time.count(), threshold, "Mean time should be acceptable");
assert_less_than(result.percentile_95.count(), threshold * 2, "95th percentile should be reasonable");
```

2. **Warm up before measuring:**
```cpp
// Warm up to initialize caches, JIT compilation, etc.
for (int i = 0; i < 10; ++i) {
  operation();
}

// Now measure
Benchmark benchmark("Operation");
auto result = benchmark.measure([&]() {
  operation();
}, 1000);
```

3. **Control system load:**
```bash
# Run performance tests on dedicated machines
# Or set CPU affinity
taskset -c 0 ./performance_test

# Set process priority
nice -n -10 ./performance_test
```

### Problem: Memory Usage Tests Fail

**Symptoms:**
```bash
Assertion failed: Memory usage 150MB exceeds limit of 100MB
```

**Solutions:**

1. **Account for debug builds:**
```cpp
#ifdef NDEBUG
  size_t memory_limit = 100 * 1024 * 1024;  // 100MB for release
#else
  size_t memory_limit = 200 * 1024 * 1024;  // 200MB for debug
#endif

assert_memory_usage_less_than([&]() {
  operation();
}, memory_limit);
```

2. **Measure peak memory, not current:**
```cpp
// Use memory monitoring that tracks peak usage
MemoryMonitor monitor;
monitor.start();
operation();
monitor.stop();

size_t peak_memory = monitor.peak_usage();
assert_less_than(peak_memory, limit, "Peak memory should be within limit");
```

3. **Clean up before measuring:**
```cpp
// Force garbage collection/cleanup
std::vector<LargeObject>().swap(large_objects);  // Clear and deallocate
force_garbage_collection();  // If applicable

// Now measure clean memory usage
assert_memory_usage_less_than([&]() {
  operation();
}, limit);
```

## CI/CD Integration Issues

### Problem: Tests Pass Locally But Fail in CI

**Symptoms:**
- All tests pass on developer machine
- Same tests fail in CI environment
- Different error messages or behaviors

**Diagnosis:**
```bash
# Compare environments
echo "Local environment:"
uname -a
gcc --version
cmake --version
ldd ./test_executable

echo "CI environment:"
# Check CI logs for same information
```

**Solutions:**

1. **Use same compiler and flags:**
```cmake
# In CMakeLists.txt, be explicit about compiler flags
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

# Don't rely on system defaults
```

2. **Handle missing dependencies:**
```cpp
// Check for required resources
if (!file_exists("test_data.json")) {
  skip_test("Test data file not found");
  return;
}

if (!network_available()) {
  skip_test("Network not available for integration test");
  return;
}
```

3. **Use container for consistent environment:**
```dockerfile
# Dockerfile.test
FROM ubuntu:22.04
RUN apt-get update && apt-get install -y build-essential cmake
COPY . /app
WORKDIR /app
RUN cmake -B build -DENABLE_TESTING=ON
RUN cmake --build build
CMD ["ctest", "--output-on-failure"]
```

### Problem: CI Timeouts

**Symptoms:**
```bash
CI job cancelled due to timeout after 30 minutes
```

**Solutions:**

1. **Optimize test execution:**
```cmake
# Run tests in parallel
set_tests_properties(TestName PROPERTIES
    PARALLEL_LEVEL 4
)

# Set appropriate timeouts
set_tests_properties(TestName PROPERTIES
    TIMEOUT 300  # 5 minutes instead of default
)
```

2. **Split long-running tests:**
```bash
# Instead of one long test suite
ctest --timeout 1800  # 30 minutes

# Split into categories
ctest -L "unit" --timeout 300      # 5 minutes
ctest -L "integration" --timeout 600  # 10 minutes
ctest -L "benchmark" --timeout 900    # 15 minutes
```

3. **Use test sharding:**
```bash
# Split tests across multiple CI jobs
# Job 1:
ctest -I 1,10,3  # Tests 1, 4, 7, 10

# Job 2:
ctest -I 2,10,3  # Tests 2, 5, 8

# Job 3:
ctest -I 3,10,3  # Tests 3, 6, 9
```

## Memory and Resource Issues

### Problem: Memory Leaks in Tests

**Symptoms:**
```bash
$ valgrind ./test_executable
==12345== LEAK SUMMARY:
==12345==    definitely lost: 1,024 bytes in 1 blocks
```

**Diagnosis:**
```bash
# Run with Valgrind
valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./test_executable

# Use AddressSanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address -g" ..
./test_executable
```

**Solutions:**

1. **Use RAII and smart pointers:**
```cpp
// Bad
void test_method() {
  Object* obj = new Object();
  obj->process();
  // Forgot to delete obj - memory leak
}

// Good
void test_method() {
  auto obj = std::make_unique<Object>();
  obj->process();
  // Automatic cleanup when obj goes out of scope
}
```

2. **Proper test fixture cleanup:**
```cpp
class TestFixture : public SolarSystem::Testing::TestCase {
  void setup() override {
    resource_ = std::make_unique<Resource>();
  }

  void teardown() override {
    // Explicit cleanup if needed
    resource_->cleanup();
    resource_.reset();
  }

private:
  std::unique_ptr<Resource> resource_;
};
```

3. **Clean up static/global state:**
```cpp
void teardown() override {
  // Clean up singletons
  SingletonManager::reset_all();

  // Clear global caches
  GlobalCache::clear();

  // Reset static variables
  reset_static_state();
}
```

### Problem: File Handle Exhaustion

**Symptoms:**
```bash
Error: Too many open files
```

**Solutions:**

1. **Use RAII for file handles:**
```cpp
// Bad
FILE* file = fopen("test.txt", "r");
process_file(file);
// Forgot to close file

// Good
{
  std::ifstream file("test.txt");
  process_file(file);
  // File automatically closed when going out of scope
}
```

2. **Clean up temporary files:**
```cpp
class TemporaryFile {
public:
  TemporaryFile(const std::string& path) : path_(path) {}
  ~TemporaryFile() {
    std::remove(path_.c_str());  // Clean up on destruction
  }

private:
  std::string path_;
};

void test_method() {
  TemporaryFile temp_file("test_temp.txt");
  // Use file
  // Automatic cleanup when temp_file goes out of scope
}
```

## Build and Compilation Problems

### Problem: Linking Errors

**Symptoms:**
```bash
undefined reference to `SolarSystem::Testing::TestCase::TestCase()'
```

**Solutions:**

1. **Check library linking:**
```cmake
target_link_libraries(test_executable
    PRIVATE
        solar_test      # Make sure this is included
        solar_core
        solar_jpl
        solar_utils
)
```

2. **Verify library build:**
```bash
# Check if libraries were built
ls -la build/lib/
nm build/lib/libsolar_test.a | grep TestCase  # Check symbols
```

3. **Check include paths:**
```cmake
target_include_directories(test_executable
    PRIVATE
        ${CMAKE_SOURCE_DIR}/lib/solar_test/include
)
```

### Problem: Header Not Found

**Symptoms:**
```bash
fatal error: solar_test/solar_test.hpp: No such file or directory
```

**Solutions:**

1. **Check include directories:**
```cmake
# In CMakeLists.txt
target_include_directories(test_executable
    PRIVATE
        ${CMAKE_SOURCE_DIR}/lib/solar_test/include
)
```

2. **Verify header exists:**
```bash
find . -name "solar_test.hpp"
ls -la lib/solar_test/include/solar_test/
```

3. **Check relative paths:**
```cpp
// Make sure include path is correct
#include <solar_test/solar_test.hpp>  // Not solar_test.hpp
```

## Common Error Messages

### "Test executable not found"

**Cause:** Test wasn't built or is in wrong location

**Solution:**
```bash
# Check if executable exists
ls -la build/tests/unit/test_name

# Rebuild if missing
cmake --build build --target test_name
```

### "Assertion failed: Expected X but got Y"

**Cause:** Test logic error or incorrect expected values

**Solution:**
```cpp
// Add debug output to understand the issue
std::cout << "Expected: " << expected << std::endl;
std::cout << "Actual: " << actual << std::endl;
std::cout << "Difference: " << (actual - expected) << std::endl;

// Check if values are within reasonable tolerance
assert_near(expected, actual, tolerance, "Values should be approximately equal");
```

### "Mock was not called"

**Cause:** Component not using mock or mock not configured correctly

**Solution:**
```cpp
// Verify mock is passed to component
Component component(mock);  // Make sure mock is passed

// Check if component method is called
component.method_that_should_use_mock();

// Verify mock configuration
assert_true(mock->is_configured(), "Mock should be configured");
```

### "Timeout exceeded"

**Cause:** Test taking too long or hanging

**Solution:**
```cpp
// Add timeout to test
class MyTest : public SolarSystem::Testing::TestCase {
public:
  MyTest() : TestCase({
    "MyTest",
    "Description",
    {"unit"},
    std::chrono::seconds(30)  // 30 second timeout
  }) {}
};

// Or check for infinite loops
int max_iterations = 1000;
for (int i = 0; i < max_iterations && condition; ++i) {
  // Loop body
}
assert_less_than(i, max_iterations, "Loop should terminate");
```

### "Segmentation fault"

**Cause:** Memory access violation

**Solution:**
```bash
# Run with debugger
gdb ./test_executable
(gdb) run
(gdb) bt  # Get backtrace

# Run with AddressSanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address -g" ..
./test_executable
```

## Getting Additional Help

### Debug Output

Enable verbose test output:
```bash
ctest --verbose
ctest --output-on-failure
ctest --extra-verbose
```

### Logging

Add logging to tests:
```cpp
#include <iostream>

SOLAR_TEST_CASE(MyTest, "Test with logging") {
  std::cout << "Starting test..." << std::endl;

  auto result = operation();
  std::cout << "Operation result: " << result << std::endl;

  assert_equals(expected, result, "Result should match expected");
  std::cout << "Test completed successfully" << std::endl;
}
```

### Profiling

Profile slow tests:
```bash
# Time execution
time ./test_executable

# Profile with gprof
g++ -pg test.cpp -o test_executable
./test_executable
gprof test_executable gmon.out > profile.txt

# Profile with perf
perf record ./test_executable
perf report
```

### Community Resources

- Check project documentation in `docs/`
- Review existing tests for examples
- Look at CI logs for similar issues
- Search for error messages in project issues

Remember: Good debugging starts with understanding what the test is supposed to do and what it's actually doing. Add logging, use debuggers, and don't hesitate to simplify tests to isolate problems.
