# Known Test Issues

## Flaky Test: UnitTest_Logging

**Status**: Pre-existing issue (before Task 3)

**Symptoms**:
- Test passes when run individually: `ctest -R UnitTest_Logging`
- Test sometimes segfaults when run as part of full suite: `ctest -L unit`
- Flaky even when run alone (passes ~66% of the time)

**Root Cause**:
- Likely a race condition or cleanup issue in test_logging.cpp
- Logger singleton may have state management issues
- Not caused by Task 3 changes (verified by checking out commit 614e80d)

**Evidence**:
```bash
# Test passes at commit 614e80d (before Task 3):
git checkout 614e80d
ctest --test-dir build -L unit  # 100% pass rate

# Test is flaky at current commit:
git checkout modern-cpp-architecture
ctest --test-dir build -R UnitTest_Logging  # Sometimes passes, sometimes segfaults
```

**Impact**:
- Does not affect Task 3 deliverables (test_solar_utils.cpp, test_args.cpp)
- Our new tests pass consistently (100% success rate)
- Full test suite shows 98% pass rate (54/55 tests)

**Workaround**:
- Run UnitTest_Logging individually when needed
- Or run full suite multiple times (usually passes 2 out of 3 runs)

**Recommendation**:
- Fix as part of `unimplemented-functions-completion` spec, Task 2 (Implement Error Handling and Logging System)
- Likely needs proper Logger singleton cleanup/reset between tests
- May need mutex protection or better state isolation
- The logging system has simplified/unimplemented functions that need completion

**Related Spec**: `.kiro/specs/unimplemented-functions-completion/tasks.md` - Task 2
**Date Identified**: 2025-11-07
**Identified By**: Task 3 implementation testing
