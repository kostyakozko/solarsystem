# Test Troubleshooting Reference

Quick reference guide for common testing issues and solutions.

## Quick Diagnostics

```bash
# Check test build status
cmake --build build --target test_name 2>&1 | grep -i error

# Run single test with verbose output
ctest --test-dir build -R "TestName" --verbose --output-on-failure

# Check test labels
ctest --test-dir build --print-labels

# List all tests
ctest --test-dir build -N
```

## Common Error Messages

### "Test not found"

**Error**: `No tests were found!!!`

**Cause**: Test not registered in CMakeLists.txt

**Solution**:
```cmake
add_test(NAME UnitTest_YourTest COMMAND test_your_test)
```

### "Undefined reference"

**Error**: `undefined reference to 'function_name'`

**Cause**: Missing library linkage

**Solution**:
```cmake
target_link_libraries(test_name test_utils solar_core solar_utils Threads::Threads)
```

### "Assertion failed"

**Error**: `Assertion failed: expected X but got Y`

**Cause**: Test logic error or actual bug

**Solution**:
1. Check test expectations
2. Debug the code under test
3. Verify test data is correct

### "Timeout"

**Error**: `Test timeout exceeded`

**Cause**: Test runs too long or hangs

**Solution**:
```cmake
set_tests_properties(TestName PROPERTIES TIMEOUT 60)  # Increase timeout
```

Or fix the hanging code.

### "Segmentation fault"

**Error**: `Segmentation fault (core dumped)`

**Cause**: Memory access error

**Solution**:
```bash
# Run with address sanitizer
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address -g" ..
cmake --build . && ctest
```

## Platform-Specific Issues

### macOS

**Issue**: Tests fail with "dylib not found"

**Solution**:
```bash
export DYLD_LIBRARY_PATH=/path/to/libs:$DYLD_LIBRARY_PATH
```

### Linux

**Issue**: Permission denied

**Solution**:
```bash
chmod +x build/tests/unit/test_name
```

### Windows

**Issue**: DLL not found

**Solution**: Add DLL directory to PATH or copy DLLs to test directory.

## Performance Issues

### Tests Run Slowly

**Solutions**:
- Run in parallel: `ctest -j$(nproc)`
- Reduce test data size
- Use mocks for expensive operations
- Profile slow tests

### Memory Usage

**Check memory**:
```bash
# Monitor during test run
top -p $(pgrep test_name)

# Or use valgrind
valgrind --leak-check=full ./build/tests/unit/test_name
```

## Flaky Tests

### Symptoms
- Test passes sometimes, fails other times
- Different results on different runs
- Timing-dependent failures

### Solutions
1. Add proper synchronization
2. Increase timeouts
3. Use deterministic test data
4. Fix race conditions

## CI/CD Issues

### Tests Pass Locally, Fail in CI

**Common causes**:
- Different environment
- Missing dependencies
- Timing differences
- Resource constraints

**Solutions**:
- Match CI environment locally
- Check CI logs carefully
- Add debug output
- Increase timeouts for CI

## Getting More Help

1. Check test output carefully
2. Run with `--verbose` flag
3. Use debugger (lldb/gdb)
4. Review recent code changes
5. Ask team for help

---

**Last Updated**: 2025-11-13
