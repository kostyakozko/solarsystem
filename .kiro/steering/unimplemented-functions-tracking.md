# Unimplemented Functions Tracking Guidelines

## Purpose

This steering rule ensures that all simplified, stub, mock, or placeholder implementations are properly documented and tracked in the `unimplemented-functions-completion` spec for future implementation.

## When to Apply

Apply these guidelines when:
- Implementing new features or functionality
- Creating test infrastructure
- Adding placeholder implementations for future work
- Completing tasks from any spec

## Detection Keywords

Watch for these patterns in code that indicate unimplemented/simplified functions:

### Code Comments
- `// Simplified`
- `// TODO`
- `// FIXME`
- `// Stub`
- `// Mock implementation`
- `// Placeholder`
- `// Not implemented`
- `// Temporary`

### Function Implementations
- Functions that return hardcoded values
- Functions that return empty containers
- Functions that return `nullopt` or `nullptr` without logic
- Functions with empty bodies or minimal logic
- Functions that throw "not implemented" exceptions

### Common Patterns
```cpp
// Pattern 1: Hardcoded return values
double calculate_something() {
    return 0.0;  // Simplified
}

// Pattern 2: Empty implementations
void process_data() {
    // TODO: Implement actual processing
}

// Pattern 3: Mock/stub implementations
std::string serialize() {
    return "{\"type\":\"message\"}";  // Simplified JSON
}

// Pattern 4: Always returns empty/null
std::optional<Data> get_data() {
    return std::nullopt;  // Simplified mock
}
```

## Documentation Process

### Step 1: Identify During Implementation

When implementing a task, if you create any simplified/stub implementations:

1. **Mark clearly in code** with comment indicating it's simplified
2. **Document why** it's simplified (testing, future work, etc.)
3. **Note in commit message** that simplified implementations were added

Example:
```cpp
template <typename T>
std::optional<T> DistributedCache::get(const std::string& /* key */) {
    // Simplified mock implementation for testing
    // TODO: Implement proper cache retrieval with TTL checking
    // Tracked in: unimplemented-functions-completion spec
    return std::nullopt;
}
```

### Step 2: Create Documentation File

After completing a task or phase, create a documentation file in the spec directory:

**File**: `.kiro/specs/[spec-name]/UNIMPLEMENTED_FUNCTIONS.md`

**Template**:
```markdown
# Unimplemented Functions in [Spec Name]

## Overview
Brief description of what was implemented and what remains simplified.

## Status: [DOCUMENTED/IN_PROGRESS/COMPLETED]

---

## Task [Number]: [Task Name]

### File: `path/to/file.cpp`

**Status**: Simplified/Mock/Stub implementation

#### Function Name (Line X)

1. **`function_name()`** (Line XXX)
   - **Current**: Description of current simplified implementation
   - **Needed**: Description of what full implementation requires
   - **Impact**: Impact of not having full implementation
   - **Requirements**: Reference to requirements
   - **Priority**: HIGH/MEDIUM/LOW

**Recommendation**: Implementation strategy or approach

---

## Summary Statistics

- **Total Functions**: X identified
- **Priority Breakdown**: HIGH: X, MEDIUM: X, LOW: X
```

### Step 3: Integrate with Unimplemented Functions Spec

After documenting, integrate findings into the main spec:

1. **Add to Requirements** (`.kiro/specs/unimplemented-functions-completion/requirements.md`)
   - Create new requirement or sub-requirement
   - Document acceptance criteria

2. **Add to Design** (`.kiro/specs/unimplemented-functions-completion/design.md`)
   - Add design section with implementation approach
   - Include code examples and strategies

3. **Add to Tasks** (`.kiro/specs/unimplemented-functions-completion/tasks.md`)
   - Create new phase or add to existing phase
   - Break down into specific implementation tasks
   - Prioritize by impact

### Step 4: Commit Documentation

Commit the documentation separately from the implementation:

```bash
git add .kiro/specs/[spec-name]/UNIMPLEMENTED_FUNCTIONS.md
git add .kiro/specs/unimplemented-functions-completion/
git commit -m "docs: catalog unimplemented functions from [spec-name]

- Identified X unimplemented functions across Y files
- Documented in UNIMPLEMENTED_FUNCTIONS.md
- Integrated into unimplemented-functions-completion spec
- Prioritized by impact: HIGH (X), MEDIUM (X), LOW (X)"
```

## Audit Process

### During Task Completion

Before marking a task as complete:

1. **Search for keywords** in all modified files:
   ```bash
   grep -r "Simplified\|TODO\|FIXME\|Stub\|Mock implementation" [modified-files]
   ```

2. **Review each occurrence** to determine if it needs tracking

3. **Document findings** if any simplified implementations exist

4. **Update task notes** to reference the documentation

### Periodic Audits

Perform periodic audits to catch any missed simplified implementations:

```bash
# Search entire codebase
grep -r "Simplified\|TODO\|FIXME\|Stub\|Mock implementation" \
  lib/solar_core/src/ \
  lib/solar_core/include/ \
  lib/solar_jpl/src/ \
  lib/solar_utils/src/ \
  --include="*.cpp" --include="*.hpp"
```

## Priority Guidelines

### HIGH Priority
- Functions that block production use
- Serialization/deserialization functions
- Network communication functions
- Security-critical functions
- Data integrity functions

### MEDIUM Priority
- Functions that reduce functionality
- Quality assessment functions
- Statistical calculation functions
- Performance monitoring functions
- Diagnostic functions

### LOW Priority
- Functions that are nice-to-have
- Visualization enhancements
- Configuration enhancements
- Advanced features
- Optional optimizations

## Integration with Development Workflow

### Task Completion Checklist

Add to the standard task completion checklist:

- [ ] Implementation complete and tested
- [ ] All tests passing (100% success rate)
- [ ] **Check for simplified/stub implementations**
- [ ] **Document any simplified implementations**
- [ ] **Integrate with unimplemented-functions-completion spec if needed**
- [ ] Mark task as completed
- [ ] Commit changes with descriptive message

### Code Review Checklist

During code review, check for:

- [ ] Are there any simplified/stub implementations?
- [ ] Are they properly documented with comments?
- [ ] Are they tracked in the unimplemented-functions-completion spec?
- [ ] Is the priority appropriate?
- [ ] Is there a clear path to full implementation?

## Examples

### Example 1: Data Sharing Templates (Task 19)

**During Implementation**:
```cpp
template <typename T>
SolarSystem::Utils::Expected<DataVersion, std::string>
SharedDataManager::store(const std::string& /* key */, const T& /* value */,
                        const std::string& owner) {
    // Simplified mock implementation for testing
    // TODO: Implement proper serialization using JSON/MessagePack
    // Tracked in: unimplemented-functions-completion spec, Phase 3.5, Task 10.4
    DataVersion version;
    version.version = 1;
    version.timestamp = std::chrono::system_clock::now();
    version.modified_by = owner;
    return SolarSystem::Utils::Expected<DataVersion, std::string>(version);
}
```

**Documentation Created**:
- `.kiro/specs/application-enhancements/UNIMPLEMENTED_FUNCTIONS.md`
- Integrated into `.kiro/specs/unimplemented-functions-completion/`

**Result**: 5 template methods documented and tracked for future implementation

### Example 2: Message Serialization (Task 18)

**During Implementation**:
```cpp
SolarSystem::Utils::Expected<std::vector<uint8_t>, SerializationError>
JsonMessageSerializer::serialize(const Message& /* message */) const {
    // Simplified JSON serialization for testing
    // TODO: Implement proper JSON serialization using nlohmann/json
    // Tracked in: unimplemented-functions-completion spec, Phase 3.5, Task 10.5
    std::string json = "{\"type\":\"message\"}";
    return std::vector<uint8_t>(json.begin(), json.end());
}
```

**Documentation Created**:
- Documented 7 serialization functions
- Added to unimplemented-functions-completion spec with HIGH priority

**Result**: Clear path to implementation with proper JSON library integration

## Benefits

### For Current Development
- **Transparency**: Everyone knows what's simplified vs. fully implemented
- **Testing**: Tests can be written knowing the limitations
- **Planning**: Future work is clearly documented

### For Future Development
- **Prioritization**: Clear priority levels guide implementation order
- **Context**: Documentation provides context for why simplifications exist
- **Efficiency**: No need to rediscover what needs implementation

### For Project Management
- **Tracking**: All unimplemented functions tracked in one place
- **Progress**: Can measure progress on completing implementations
- **Risk Management**: Understand what's not production-ready

## Anti-Patterns to Avoid

### ❌ Don't Do This
```cpp
// Bad: No documentation, no tracking
double calculate() {
    return 0.0;  // TODO
}
```

### ✅ Do This Instead
```cpp
// Good: Clear documentation and tracking
double calculate() {
    // Simplified implementation for testing
    // TODO: Implement proper calculation algorithm
    // Tracked in: unimplemented-functions-completion spec, Phase X, Task Y
    // Impact: Results will be inaccurate until implemented
    return 0.0;
}
```

### ❌ Don't Do This
```cpp
// Bad: Silently returns mock data
std::string serialize() {
    return "{}";
}
```

### ✅ Do This Instead
```cpp
// Good: Clearly marked as simplified
std::string serialize() {
    // Simplified mock serialization for testing
    // TODO: Implement proper JSON serialization
    // Tracked in: unimplemented-functions-completion spec
    // Impact: Cannot serialize actual data structures
    return "{}";
}
```

## Maintenance

### Regular Reviews

- **Monthly**: Review unimplemented-functions-completion spec for progress
- **Quarterly**: Audit codebase for any undocumented simplified implementations
- **Before Release**: Ensure all HIGH priority functions are implemented

### Updates

- Update this steering rule as new patterns emerge
- Add examples from actual implementations
- Refine priority guidelines based on experience

---

**Last Updated**: 2025-10-29
**Status**: Active
**Applies To**: All specs and implementations
