# Development Workflow Guidelines

## Git Commit Strategy

### Automatic Commits After Task Completion
- **ALWAYS** commit changes after successfully completing any task
- Use descriptive commit messages that reference the task being completed
- Commit format: `feat: [task description] - [brief summary of changes]`
- Include relevant files in each commit (don't leave uncommitted changes)

### Commit Message Conventions
- **feat**: New features or functionality
- **fix**: Bug fixes
- **refactor**: Code refactoring without functional changes
- **test**: Adding or updating tests
- **docs**: Documentation updates
- **build**: Build system or dependency changes
- **style**: Code formatting or style changes

### Examples of Good Commit Messages
```
feat: implement core simulation engine - add N-body physics calculations
fix: resolve memory leak in JPL data caching - proper cleanup in destructor
test: add unit tests for celestial body factory - cover edge cases
refactor: modernize vector operations - use C++20 concepts and ranges
docs: update API documentation - add examples for new simulation methods
```

### Workflow Steps
1. Complete the assigned task
2. Verify the implementation works correctly
3. **Run comprehensive test suite** (see Testing Requirements below)
4. **Mark task as completed** (update task status to completed)
5. Format code if needed (`make format`)
6. Stage all relevant changes (`git add`)
7. Commit with descriptive message
8. Confirm commit was successful

### What to Commit
- All source code changes (`.cpp`, `.h` files)
- Build configuration updates (`CMakeLists.txt`)
- Documentation updates (`.md` files)
- Test files and test data
- Configuration files when modified
- **Task status updates** (tasks.md files with completed status)

### What NOT to Commit
- Build artifacts (`build/` directory contents)
- Cache files (`cache/` directory contents)
- Generated documentation (`docs/api/html/`, `docs/api/latex/`)
- Temporary files or IDE-specific files

## Task Completion Protocol

### After Every Successful Task:
1. **Verify**: Ensure the task requirements are fully met
2. **Test**: Run comprehensive test suite (MANDATORY - see Testing Requirements)
3. **Mark Complete**: Update task status to completed (so it's included in commit)
4. **Format**: Apply code formatting if source files were modified
5. **Stage**: Add all relevant changes to git staging (including task status updates)
6. **Commit**: Create a descriptive commit message
7. **Confirm**: Verify the commit was created successfully
8. **CLEAR ALL**: Clear all previous context so new task could be done with fresh session

### Commit Timing
- Mark task as completed BEFORE committing (so task status is included in commit)
- Commit immediately after marking task complete
- Don't wait for multiple tasks to accumulate
- Each task should result in at least one commit
- Break large tasks into smaller commits if they involve multiple logical changes

## Testing Requirements

### MANDATORY: 100% Test Success Before Commit
**ALL tests must pass with 100% success rate before any commit is allowed.**

### Test Execution Commands
```bash
# Build with testing enabled (if not already done)
cmake -B build -DENABLE_TESTING=ON
cmake --build build -j$(nproc)

# Run ALL tests - MUST be 100% successful
cd build && ctest --output-on-failure

# Alternative: Run comprehensive test suite
make test-all
```

### Required Test Categories
1. **Unit Tests**: All library component tests must pass
   ```bash
   ctest -L "unit" --output-on-failure
   ```

2. **Integration Tests**: All workflow and end-to-end tests must pass
   ```bash
   ctest -L "integration" --output-on-failure
   ```

3. **Performance Benchmarks**: All performance tests must pass (no regressions > 10%)
   ```bash
   ctest -L "benchmark" --output-on-failure
   ```

### Test Failure Protocol
- **If ANY test fails**: DO NOT COMMIT
- Fix the failing test(s) or the underlying issue
- Re-run the complete test suite
- Only commit when ALL tests pass (100% success rate)

### Test Coverage Requirements
- New functionality must include corresponding unit tests
- Modified code must not break existing tests
- Integration tests must validate end-to-end workflows
- Performance tests must not show regressions > 10%

### Quick Test Validation
```bash
# Quick validation script for pre-commit
./tests/scripts/run_all_tests.sh --quick

# Full validation with benchmarks
./tests/scripts/run_all_tests.sh --comprehensive
```

### Test Environment Setup
- Ensure build directory exists: `mkdir -p build`
- Configure with testing: `cmake -B build -DENABLE_TESTING=ON`
- Build all targets: `cmake --build build -j$(nproc)`
- Verify test utilities are available

### Continuous Integration Alignment
This testing requirement aligns with the CI/CD pipeline:
- Same test suite runs in GitHub Actions
- Same success criteria (100% pass rate)
- Same timeout settings and test categories
- Prevents CI failures by catching issues locally

## Quality Standards

### Before Committing
- **ALL TESTS PASS**: 100% success rate required (see Testing Requirements above)
- Code compiles without errors or warnings
- Follows project coding standards (Google style, 100 column limit)
- Includes appropriate error handling
- Has reasonable test coverage for new functionality
- Documentation is updated for public APIs

### Commit Quality
- Commit messages are clear and descriptive
- Changes are logically grouped
- No unrelated changes mixed together
- All files needed for the change are included
- No debugging code or temporary changes left in

## Integration with Existing Workflow

This workflow integrates with the existing Solar System Suite development process:
- Follows the modular library architecture
- Respects the C++20 coding standards
- Maintains the CMake build system requirements
- Supports the testing framework structure
- Preserves the documentation generation process
