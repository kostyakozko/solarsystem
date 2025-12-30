# JSON Migration Analysis

## Overview

This document catalogs all custom JSON implementations and usage in the Solar System Suite codebase. These files have been migrated to use nlohmann/json in Phase 5 (Tasks 11.1-11.4).

**Analysis Date**: December 30, 2025
**nlohmann/json Status**: Integrated via FetchContent (Task 5.1 ✓)
**Migration Status**: COMPLETED (December 31, 2025)

---

## Summary Statistics

| Category | File Count | Status |
|----------|------------|--------|
| Manual JSON Building (ostringstream) | 10 | ✓ MIGRATED |
| Custom JSON Validator | 1 | KEPT (provides security features) |
| Custom JSON Reporter | 1 | ✓ MIGRATED |
| to_json()/from_json() Methods | 8 | ✓ MIGRATED |
| Files Already Using nlohmann/json | 2 | N/A |

---

## Migration Completed

These files build JSON using `std::ostringstream` and string concatenation. They should be migrated to use `nlohmann::json` objects.

### 1.1 lib/solar_jpl/src/cache_manager.cpp
- **Location**: Lines 277-302
- **Function**: `save_cache()` - JSON cache serialization
- **Pattern**: Manual `json_file << "{ ... }"` building
- **Migration**: Use `nlohmann::json` for EphemerisData serialization
- **Task**: 11.2 (Migrate JSON in solar_jpl)

### 1.2 lib/solar_jpl/src/cache_recovery.cpp
- **Location**: Line 264-270
- **Function**: Metadata file writing
- **Pattern**: Manual `metadata_file << "{ ... }"` building
- **Task**: 11.2 (Migrate JSON in solar_jpl)

### 1.3 lib/solar_jpl/src/data_validator.cpp
- **Location**: Lines 113-139
- **Function**: `ValidationReport::to_json()`
- **Pattern**: Manual ostringstream JSON building
- **Task**: 11.2 (Migrate JSON in solar_jpl)

### 1.4 apps/solar_system_web/src/web_server.cpp
- **Location**: Multiple locations (lines 269, 793-1262)
- **Functions**:
  - `create_error_response()` - Error JSON responses
  - `handle_status()` - Status endpoint
  - `handle_health()` - Health endpoint
  - `handle_bodies()` - Bodies list endpoint
  - `handle_state()` - State endpoint
  - `handle_simulate()` - Simulation endpoint
- **Pattern**: Extensive manual `json << "{ ... }"` building
- **Task**: 11.3 (Migrate JSON in web server)

### 1.5 lib/solar_core/src/output/output_formatter.cpp
- **Location**: Lines 16-45
- **Functions**: `OutputMetadata::to_json()`, `OutputMetadata::from_json()`
- **Pattern**: Manual ostringstream JSON building
- **Task**: 11.1 (Migrate JSON in solar_core)

### 1.6 lib/solar_core/src/bodies/body_collection.cpp
- **Location**: Lines 200-220
- **Function**: `BodyCollection::to_json()`
- **Pattern**: Manual ostringstream JSON building
- **Task**: 11.1 (Migrate JSON in solar_core)

### 1.7 lib/solar_core/src/diagnostics/diagnostic_system.cpp
- **Location**: Lines 92-120
- **Function**: `DiagnosticReport::to_json()`
- **Pattern**: Manual ostringstream JSON building
- **Task**: 11.1 (Migrate JSON in solar_core)

### 1.8 lib/solar_core/src/builders/simulation_builder.cpp
- **Location**: Lines 842-912
- **Functions**: `ConfigurationBuilder::to_json()`, `ConfigurationBuilder::from_json()`
- **Pattern**: Manual ostringstream JSON building and basic string parsing
- **Task**: 11.1 (Migrate JSON in solar_core)

### 1.9 lib/solar_core/src/visualization/visualization_modes.cpp
- **Location**: Lines 268-289
- **Function**: `export_visualization()` with ExportFormat::JSON
- **Pattern**: Manual ostringstream JSON building
- **Task**: 11.1 (Migrate JSON in solar_core)

### 1.10 lib/solar_utils/src/logging.cpp
- **Location**: Lines 37-55
- **Function**: `LogEntry::to_json()`
- **Pattern**: Manual ostringstream JSON building
- **Task**: 11.1 (Migrate JSON in solar_core) or new task for solar_utils

---

## Category 2: Custom JSON Validator (MEDIUM Priority)

### 2.1 lib/solar_utils/include/solar_utils/validation/json_validator.hpp
- **Class**: `JSONValidator`
- **Features**:
  - JSON syntax validation
  - JSON schema validation
  - JSON structure/type checking
  - Security validation (injection detection)
  - Pretty print / minify
- **Decision**: Could be replaced with nlohmann/json's built-in validation, or kept as a wrapper
- **Task**: 11.1 or new task

---

## Category 3: Custom JSON Reporter (MEDIUM Priority)

### 3.1 lib/solar_test/include/solar_test/reporters/json_reporter.hpp
- **Class**: `JsonReporter`
- **Features**:
  - Test result JSON serialization
  - Custom `json_escape()` method
  - Manual JSON building methods
- **Implementation**: lib/solar_test/src/reporters/json_reporter.cpp (if exists)
- **Task**: 11.4 (Migrate JSON in test framework)

---

## Category 4: to_json()/from_json() Methods (HIGH Priority)

These methods need to be migrated to use nlohmann/json's ADL serialization pattern.

| File | Class/Struct | Methods |
|------|--------------|---------|
| lib/solar_core/src/output/output_formatter.cpp | OutputMetadata | to_json(), from_json() |
| lib/solar_core/src/bodies/body_collection.cpp | BodyCollection | to_json() |
| lib/solar_core/src/diagnostics/diagnostic_system.cpp | DiagnosticReport | to_json() |
| lib/solar_core/src/builders/simulation_builder.cpp | ConfigurationBuilder | to_json(), from_json() |
| lib/solar_jpl/src/data_validator.cpp | ValidationReport | to_json() |
| lib/solar_utils/src/logging.cpp | LogEntry | to_json() |
| lib/solar_test/src/benchmarks/regression_detector.cpp | PerformanceBaseline | to_json(), from_json() |
| lib/solar_test/src/benchmarks/regression_detector.cpp | RegressionAnalysis | to_json() |

---

## Category 5: Files Already Using nlohmann/json (No Migration Needed)

### 5.1 lib/solar_core/src/data/shared_data_manager.cpp
- **Status**: ✓ Already uses `#include <nlohmann/json.hpp>`
- **Usage**: Template serialization

### 5.2 lib/solar_core/src/communication/message.cpp
- **Status**: ✓ Already uses `#include <nlohmann/json.hpp>`
- **Usage**: JsonMessageSerializer (simplified implementation)

---

## Migration Priority Order

### Phase 5 Task Mapping

| Task | Files | Estimated Effort |
|------|-------|------------------|
| **11.1** Migrate JSON in solar_core | output_formatter.cpp, body_collection.cpp, diagnostic_system.cpp, simulation_builder.cpp, visualization_modes.cpp | HIGH |
| **11.2** Migrate JSON in solar_jpl | cache_manager.cpp, cache_recovery.cpp, data_validator.cpp | MEDIUM |
| **11.3** Migrate JSON in web server | web_server.cpp | HIGH (many endpoints) |
| **11.4** Migrate JSON in test framework | json_reporter.hpp/cpp, regression_detector.cpp | MEDIUM |

### Additional Considerations

1. **lib/solar_utils/src/logging.cpp** - Not covered by existing tasks, may need to add to 11.1 or create new task
2. **json_validator.hpp** - Evaluate if nlohmann/json's validation is sufficient or if wrapper is needed
3. **launcher.cpp** - Has `load_from_json()` method that needs migration

---

## Migration Patterns

### Pattern 1: Simple to_json() Migration

**Before (manual):**
```cpp
std::string MyClass::to_json() const {
  std::ostringstream oss;
  oss << "{\n";
  oss << "  \"field\": " << field_ << "\n";
  oss << "}";
  return oss.str();
}
```

**After (nlohmann/json):**
```cpp
std::string MyClass::to_json() const {
  nlohmann::json j;
  j["field"] = field_;
  return j.dump(2);  // 2-space indent
}
```

### Pattern 2: ADL Serialization (Recommended)

```cpp
// In header
void to_json(nlohmann::json& j, const MyClass& obj);
void from_json(const nlohmann::json& j, MyClass& obj);

// In source
void to_json(nlohmann::json& j, const MyClass& obj) {
  j = nlohmann::json{{"field", obj.field_}};
}

void from_json(const nlohmann::json& j, MyClass& obj) {
  j.at("field").get_to(obj.field_);
}
```

### Pattern 3: Web Server Response Building

**Before:**
```cpp
std::ostringstream json;
json << "{\n";
json << "  \"status\": \"ok\"\n";
json << "}";
return json.str();
```

**After:**
```cpp
nlohmann::json response;
response["status"] = "ok";
return response.dump();
```

---

## Notes

- All files listed have been verified to exist in the codebase
- Priority is based on code complexity and usage frequency
- Web server has the most extensive manual JSON building
- Consider creating helper functions for common patterns during migration
