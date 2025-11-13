# Solar System Suite Testing Tutorial

## Introduction

Welcome to the Solar System Suite testing tutorial! This guide will walk you through creating, running, and maintaining tests for the project.

## Prerequisites

- C++20 compiler (GCC 7+, Clang 5+, or MSVC 2019+)
- CMake 3.15+
- Basic understanding of C++ and testing concepts

---

## Tutorial 1: Your First Test

### Step 1: Create a Test File

Create a new file `tests/unit/test_my_feature.cpp`:

```cpp
#include "test_framework.h"

int main() {
    TEST_SUITE("My Feature Tests");

    TEST_CASE("Basic Functionality") {
     // Your test code here
        ASSERT_TRUE(true);
    }

    return current_suite->all_passed() ? 0 : 1;
}
```

### Step 2: Register the Test

Add to `tests/unit/CMakeLists.txt`:

```cmake
add_executable(test_my_feature test_my_feature.cpp)
target_link_libraries(test_my_feature test_utils solar_core solar_utils Threads::Threads)
add_test(NAME UnitTest_MyFeature COMMAND test_my_feature)
set_tests_properties(UnitTest_MyFeature PROPERTIES LABELS "unit" TIMEOUT ${TEST_TIMEOUT})
```

### Step 3: Build and Run

```bash
cmake --build build --target test_my_feature
ctest --test-dir build -R "UnitTest_MyFeature" --output-on-failure
```

**Expected Output**:
```
Test project /path/to/build
    Start 1: UnitTest_MyFeature
1/1 Test #1: UnitTest_MyFeature ...............   Passed    0.01 sec

100% tests passed, 0 tests failed out of 1
```

---

## Tutorial 2: Testing a Class

Let's test a simple `Calculator` class.

### The Class

```cpp
// calculator.h
class Calculator {
public:
    int add(int a, int b) { return a + b; }
    int subtract(int a, int b) { return a - b; }
    int multiply(int a, int b) { return a * b; }
    double divide(int a, int b) {
        if (b == 0) throw std::invalid_argument("Division by zero");
        return static_cast<double>(a) / b;
    }
};
```

### The Test

```cpp
#include "test_framework.h"
#include "calculator.h"

int main() {
    TEST_SUITE("Calculator Tests");

    TEST_CASE("Addition") {
        Calculator calc;
        ASSERT_EQ(calc.add(2, 3), 5);
        ASSERT_EQ(calc.add(-1, 1), 0);
        ASSERT_EQ(calc.add(0, 0), 0);
    }

    TEST_CASE("Subtraction") {
        Calculator calc;
        ASSERT_EQ(calc.subtract(5, 3), 2);
        ASSERT_EQ(calc.subtract(0, 5), -5);
    }

    TEST_CASE("Multiplication") {
        Calculator calc;
        ASSERT_EQ(calc.multiply(3, 4), 12);
        ASSERT_EQ(calc.multiply(-2, 3), -6);
    }

    TEST_CASE("Division") {
        Calculator calc;
        ASSERT_EQ(calc.divide(10, 2), 5.0);
        ASSERT_EQ(calc.divide(7, 2), 3.5);
    }

    TEST_CASE("Division by Zero") {
        Calculator calc;
        bool exception_thrown = false;
        try {
            calc.divide(10, 0);
        } catch (const std::invalid_argument&) {
            exception_thrown = true;
        }
        ASSERT_TRUE(exception_thrown);
    }

    return current_suite->all_passed() ? 0 : 1;
}
```

---

## Tutorial 3: Testing with Mock Objects

### Scenario

Testing a `DataProcessor` that depends on a `DataSource`:

```cpp
// data_source.h
class DataSource {
public:
    virtual std::vector<int> fetch_data() = 0;
    virtual ~DataSource() = default;
};

// data_processor.h
class DataProcessor {
    DataSource& source_;
public:
    explicit DataProcessor(DataSource& source) : source_(source) {}

    int sum_data() {
        auto data = source_.fetch_data();
        return std::accumulate(data.begin(), data.end(), 0);
    }
};
```

### Mock Implementation

```cpp
class MockDataSource : public DataSource {
    std::vector<int> test_data_;
public:
    void set_test_data(std::vector<int> data) {
        test_data_ = std::move(data);
    }

    std::vector<int> fetch_data() override {
        return test_data_;
    }
};
```

### The Test

```cpp
TEST_CASE("Data Processing with Mock") {
    MockDataSource mock_source;
    mock_source.set_test_data({1, 2, 3, 4, 5});

    DataProcessor processor(mock_source);
    int result = processor.sum_data();

    ASSERT_EQ(result, 15);
}
```

---

## Tutorial 4: Performance Testing

### Measuring Execution Time

```cpp
TEST_CASE("Performance Benchmark") {
    const int iterations = 1000000;

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < iterations; ++i) {
        // Operation to benchmark
        expensive_operation();
    }

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Assert performance requirement
    ASSERT_LT(duration.count(), 1000);  // Must complete in < 1 second

    // Log performance metric
    double ops_per_sec = iterations / (duration.count() / 1000.0);
    // Note: In real tests, you'd log this properly
}
```

---

## Tutorial 5: Thread Safety Testing

### Testing Concurrent Access

```cpp
TEST_CASE("Thread-Safe Counter") {
    ThreadSafeCounter counter;
    const int num_threads = 10;
    const int increments_per_thread = 1000;

    std::vector<std::thread> threads;

    // Launch threads
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&counter, increments_per_thread]() {
            for (int j = 0; j < increments_per_thread; ++j) {
                counter.increment();
            }
        });
    }

    // Wait for completion
    for (auto& t : threads) {
        t.join();
    }

    // Verify result
    ASSERT_EQ(counter.get(), num_threads * increments_per_thread);
}
```

---

## Tutorial 6: Integration Testing

### Testing Complete Workflows

```cpp
TEST_CASE("End-to-End Simulation Workflow") {
    // Step 1: Initialize system
    SolarSystem system;
    system.add_body(create_test_body("Sun", 1.989e30, 696340000.0));
    system.add_body(create_test_body("Earth", 5.972e24, 6371000.0));

    // Step 2: Configure simulation
    SimulationConfig config;
    config.time_step = 3600.0;  // 1 hour
    config.duration = 86400.0;   // 1 day

    // Step 3: Run simulation
    SimulationEngine engine(system, config);
    auto results = engine.run();

    // Step 4: Verify results
    ASSERT_FALSE(results.empty());
    ASSERT_EQ(results.size(), 24);  // 24 hours

    // Step 5: Validate physics
    for (const auto& state : results) {
        ASSERT_TRUE(state.is_valid());
        ASSERT_GT(state.total_energy(), 0.0);
    }
}
```

---

## Tutorial 7: Debugging Failing Tests

### Using Print Statements

```cpp
TEST_CASE("Debug Example") {
    int value = compute_value();

    // Temporary debug output
    std::cout << "Computed value: " << value << std::endl;

    ASSERT_EQ(value, 42);
}
```

### Using a Debugger

```bash
# Build with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . -j$(nproc)

# Run under debugger
lldb ./build/tests/unit/test_name

# Set breakpoint
(lldb) b test_file.cpp:50

# Run
(lldb) run

# Step through
(lldb) n  # next line
(lldb) s  # step into

# Inspect
(lldb) p variable_name
(lldb) bt  # backtrace
```

---

## Tutorial 8: Test-Driven Development (TDD)

### The TDD Cycle

1. **Red**: Write a failing test
2. **Green**: Write minimal code to pass
3. **Refactor**: Improve the code

### Example: Implementing a Stack

#### Step 1: Write the Test (Red)

```cpp
TEST_CASE("Stack Push and Pop") {
    Stack<int> stack;
    stack.push(42);
    ASSERT_EQ(stack.pop(), 42);
}
```

This will fail because `Stack` doesn't exist yet.

#### Step 2: Implement Minimal Code (Green)

```cpp
template<typename T>
class Stack {
    std::vector<T> data_;
public:
    void push(const T& value) {
        data_.push_back(value);
    }

    T pop() {
        T value = data_.back();
        data_.pop_back();
        return value;
    }
};
```

Now the test passes!

#### Step 3: Refactor

Add error handling, optimize, improve design while keeping tests green.

---

## Common Patterns

### Setup and Teardown

```cpp
class TestFixture {
    Database* db_;
public:
    TestFixture() {
        // Setup
        db_ = new Database("test.db");
        db_->initialize();
    }

    ~TestFixture() {
        // Teardown
        db_->cleanup();
        delete db_;
    }

    Database* get_db() { return db_; }
};

TEST_CASE("Using Fixture") {
    TestFixture fixture;
    auto db = fixture.get_db();

    // Use db in test
    ASSERT_TRUE(db->is_connected());
}
```

### Parameterized Tests

```cpp
void test_addition(int a, int b, int expected) {
    Calculator calc;
    ASSERT_EQ(calc.add(a, b), expected);
}

TEST_CASE("Parameterized Addition Tests") {
    test_addition(1, 2, 3);
    test_addition(0, 0, 0);
    test_addition(-1, 1, 0);
    test_addition(100, 200, 300);
}
```

---

## Exercises

### Exercise 1: Basic Testing

Create tests for a `Temperature` class that converts between Celsius and Fahrenheit.

**Hint**: Test edge cases like absolute zero, freezing point, boiling point.

### Exercise 2: Mock Testing

Create a mock for a `WeatherService` and test a `WeatherReport` class.

### Exercise 3: Performance Testing

Benchmark different sorting algorithms and verify they meet performance requirements.

### Exercise 4: Thread Safety

Test a thread-safe queue implementation with multiple producers and consumers.

---

## Next Steps

1. Read the [Testing Guide](TESTING_GUIDE.md) for detailed documentation
2. Explore existing tests in `tests/unit/` for examples
3. Practice TDD on new features
4. Contribute tests for untested code
5. Help improve test coverage

---

## Getting Help

- Check the [Troubleshooting Guide](TESTING_GUIDE.md#troubleshooting-guide)
- Review existing tests for patterns
- Ask questions in team discussions
- Consult the test framework source code

---

**Happy Testing!** 🧪

---

**Last Updated**: 2025-11-13
**Version**: 4.0.0
