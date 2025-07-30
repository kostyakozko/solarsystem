# Test Data Repository

This directory contains structured test data for the Solar System Suite testing framework.

## Directory Structure

```
tests/data/
├── jpl_responses/          # JPL HORIZONS API response samples
│   ├── valid/              # Valid JPL responses for different bodies
│   ├── invalid/            # Invalid/malformed responses for error testing
│   └── scenarios/          # Specific test scenario responses
├── ephemeris/              # Ephemeris data samples
│   ├── time_periods/       # Data for different time periods
│   ├── bodies/             # Data for specific celestial bodies
│   └── formats/            # Different data formats (JSON, binary, CSV)
├── cache/                  # Cache file samples
│   ├── valid/              # Valid cache files
│   ├── corrupted/          # Corrupted cache files for error testing
│   └── formats/            # Different cache formats
├── validation/             # Reference data for validation
│   ├── orbital_mechanics/  # Known orbital mechanics solutions
│   ├── physical_constants/ # Physical constants and reference values
│   └── benchmarks/         # Performance benchmark reference data
└── scenarios/              # Complete test scenarios
    ├── unit_tests/         # Data for unit test scenarios
    ├── integration_tests/  # Data for integration test scenarios
    └── performance_tests/  # Data for performance test scenarios
```

## Data Categories

### JPL Response Samples
- **Valid responses**: Realistic JPL HORIZONS API responses for major celestial bodies
- **Invalid responses**: Malformed, incomplete, or error responses for error handling tests
- **Edge cases**: Responses for edge cases like distant dates, unusual bodies, etc.

### Ephemeris Data
- **Time periods**: J2000.0 epoch, current epoch, historical dates, future projections
- **Bodies**: All 27 supported celestial bodies with varying data quality
- **Formats**: JSON, binary, CSV formats for compatibility testing

### Cache Samples
- **Valid caches**: Properly formatted cache files with known good data
- **Corrupted caches**: Files with various types of corruption for error handling
- **Format variations**: Different cache format versions and structures

### Validation Data
- **Reference solutions**: Known correct solutions for orbital mechanics problems
- **Physical constants**: Authoritative values for masses, radii, orbital parameters
- **Benchmark data**: Reference performance metrics and expected results

## Usage

Test data files are organized by category and can be loaded using the TestDataManager class:

```cpp
// Load JPL response samples
auto jpl_data = TestDataManager::load_jpl_responses("earth_2024");

// Load ephemeris data for specific time period
auto ephemeris = TestDataManager::load_ephemeris_data("j2000_epoch");

// Load cache samples
auto cache_data = TestDataManager::load_cache_samples("valid_binary");

// Validate data integrity
bool valid = TestDataManager::validate_jpl_response(jpl_response);
```

## Data Validation

All test data includes:
- **Metadata**: Creation date, source, format version, description
- **Checksums**: Data integrity verification
- **Validation rules**: Expected ranges, formats, and constraints
- **Test scenarios**: Associated test cases and expected outcomes

## Maintenance

Test data should be:
- **Version controlled**: All data files are tracked in git
- **Documented**: Each data set includes metadata and usage notes
- **Validated**: Regular validation against known good sources
- **Updated**: Periodic updates to reflect current ephemeris data
