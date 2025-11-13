# Testing Quick Reference Card

## Common Commands

```bash
# Build all tests
cmake --build build -j$(nproc)

# Run all tests
ctest --test-dir build --output-on-failure

# Run specific test
ctest --test-dir build -R "TestName"

# Run tests with label
ctest --test-dir build -L "unit"

# Run tests in parallel
ctest --test-dir build -j$(nproc)

# Verbose output
ctest --test-dir build --verbose

# List all tests
ctest --test-dir build -N
```

## Test Structure

```cpp
#include "test_framework.h"

int main() {
    TEST_SUITE("Suite Name");

    TEST_CASE("Test Name") {
        // Arrange
        MyClass obj;

        // Act
        auto result = obj.method();

        // Assert
        ASSERT_TRUE(result);
    }

    return current_suite->all_passed() ? 0 : 1;
}
```

## Assertions

```cpp
ASSERT_TRUE(condition);
ASSERT_FALSE(condition);
ASSERT_EQ(actual, expected);
ASSERT_NE(a, b);
ASSERT_LT(a, b);  // a < b
ASSERT_LE(a, b);  // a <= b
ASSERT_GT(a, b);  // a > b
ASSERT_GE(a, b);  // a >= b
```

## CMake Registration

```cmake
add_executable(test_name test_name.cpp)
target_link_libraries(test_name test_utils solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_Name COMMAND test_name)
set_tests_properties(UnitTest_Name PROPERTIES
    LABELS "unit;feature"
    TIMEOUT ${TEST_TIMEOUT})
```

## Common Patterns

### Mock Object
```cpp
class MockService : public Service {
public:
    void set_response(Data data) { data_ = data; }
    Data get_data() override { return data_; }
private:
    Data data_;
};
```

### Performance Test
```cpp
auto start = std::chrono::steady_clock::now();
// ... code to benchmark ...
auto end = std::chrono::steady_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
ASSERT_LT(duration.count(), 1000);
```

### Thread Safety Test
```cpp
std::vector<std::thread> threads;
for (int i = 0; i < 10; ++i) {
    threads.emplace_back([&obj]() {
        obj.thread_safe_method();
    });
}
for (auto& t : threads) t.join();
```

## Debugging

```bash
# Run with address sanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address" ..

# Run under debugger
lldb ./build/tests/unit/test_name

# Set breakpoint
(lldb) b file.cpp:123

# Run and inspect
(lldb) run
(lldb) p variable
```

## Test Labels

- `unit` - Unit tests
- `integration` - Integration tests
- `benchmark` - Performance tests
- `security` - Security tests
- `concurrency` - Thread safety tests
- `phase1-12` - Test phases
- `task1-35` - Specific tasks

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Test not found | Add to CMakeLists.txt |
| Undefined reference | Check target_link_libraries |
| Timeout | Increase timeout or fix code |
| Segfault | Use address sanitizer |
| Flaky test | Add synchronization |

## Resources

- [Full Guide](TESTING_GUIDE.md)
- [Tutorial](TESTING_TUTORIAL.md)
- [Troubleshooting](TROUBLESHOOTING.md)
- [Onboarding](ONBOARDING.md)
