# Solar System Analysis Library

Comprehensive data analysis tools for scientific research, orbital mechanics analysis, and statistical studies of celestial body data.

## Overview

The `solar_analysis` library provides:
- **Orbital Mechanics**: Kepler equation solving, orbital elements calculation, trajectory analysis
- **Statistical Analysis**: Summary statistics, correlation, regression, anomaly detection
- **Data Processing**: Loading, validation, interpolation, caching
- **Mission Planning**: Transfer trajectories, launch windows, encounter analysis
- **Advanced Algorithms**: Clustering, optimization, curve fitting

## Quick Start

### Basic Orbital Analysis

```cpp
#include <solar_analysis/orbital_calculator.hpp>
#include <solar_analysis/data_processor.hpp>

using namespace SolarSystem::Analysis;

// Load data for a celestial body
DataProcessor processor;
processor.load_body_data("Earth");

// Get state vector data
auto data = processor.get_data("Earth");

// Calculate orbital elements
OrbitalCalculator calc;
auto elements = calc.calculate_elements(data.front());

std::cout << "Semi-major axis: " << elements.semi_major_axis << " km\n";
std::cout << "Eccentricity: " << elements.eccentricity << "\n";
std::cout << "Orbital period: " << elements.orbital_period << " s\n";
```

### Statistical Analysis

```cpp
#include <solar_analysis/statistical_analyzer.hpp>

StatisticalAnalyzer stats;

std::vector<double> distances = {1.47e8, 1.49e8, 1.52e8, 1.50e8};

// Summary statistics
auto summary = stats.calculate_statistics(distances);
std::cout << "Mean: " << summary.mean << "\n";
std::cout << "Std Dev: " << summary.std_dev << "\n";

// Anomaly detection
auto anomalies = stats.detect_anomalies(distances, 2.0);

// Linear regression
std::vector<double> x = {1, 2, 3, 4, 5};
std::vector<double> y = {2.1, 4.0, 5.9, 8.1, 10.0};
auto reg = stats.linear_regression(x, y);
std::cout << "Slope: " << reg.slope << ", R²: " << reg.r_squared << "\n";
```

### Data Export

```cpp
#include <solar_analysis/data_exporter.hpp>

DataExporter exporter;

// Configure export
ExportConfig config;
config.format = ExportFormat::CSV;
config.precision = 10;
exporter.set_config(config);

// Add metadata
ExportMetadata metadata;
metadata.title = "Earth Orbital Data";
metadata.reference_frame = "J2000_Ecliptic";
exporter.set_metadata(metadata);

// Export data
auto result = exporter.export_data(data, "earth_data.csv");
```

## API Reference

### Core Classes

| Class | Description |
|-------|-------------|
| `DataProcessor` | Load, validate, and cache ephemeris data |
| `OrbitalCalculator` | Orbital mechanics calculations |
| `StatisticalAnalyzer` | Statistical analysis tools |
| `DataExporter` | Multi-format data export |
| `BatchProcessor` | Automated batch analysis |
| `AnalysisCLI` | Interactive command-line interface |
| `HistoricalAnalyzer` | Long-term trend analysis |
| `MissionAnalyzer` | Mission planning tools |
| `AdvancedAnalyzer` | ML and optimization algorithms |

### Data Structures

| Structure | Description |
|-----------|-------------|
| `StateVector` | Position and velocity at a timestamp |
| `OrbitalElements` | Keplerian orbital parameters |
| `TimeRange` | Time interval specification |
| `DataQuality` | Data validation results |
| `SummaryStatistics` | Statistical summary |

## Command-Line Interface

The `solar_system_analyzer` application provides interactive analysis:

```bash
# Show help
./solar_system_analyzer --help

# Analyze specific bodies
./solar_system_analyzer -b Earth -b Mars

# Export to CSV
./solar_system_analyzer -b Jupiter --format csv -o jupiter.csv
```

### Interactive Mode

```
analysis> load Earth
Loaded data for Earth

analysis> orbital Earth
Orbital elements for Earth:
  Semi-major axis: 149597870.7 km
  Eccentricity: 0.0167
  Period: 31558149.5 s

analysis> stats Earth
Statistics for Earth distance:
  Mean: 149597870.7 km
  Std Dev: 2499647.8 km

analysis> export Earth earth_data.csv
Exported to earth_data.csv
```

## Batch Processing

```cpp
#include <solar_analysis/batch_processor.hpp>

BatchProcessor batch;

// Create analysis job
auto job = BatchProcessor::create_full_analysis_job(
    "Inner Planets Analysis",
    {"Mercury", "Venus", "Earth", "Mars"},
    "./output"
);

// Submit and process
batch.submit_job(job);
batch.process_all();

// Get results
auto result = batch.get_result(job.id);
```

## Mission Planning

```cpp
#include <solar_analysis/mission_analyzer.hpp>

MissionAnalyzer mission;

// Calculate Earth-Mars transfer
auto transfer = mission.calculate_transfer(
    1.496e8,  // Earth orbit radius (km)
    2.279e8,  // Mars orbit radius (km)
    "Earth", "Mars"
);

std::cout << "Total delta-V: " << transfer.total_delta_v << " km/s\n";
std::cout << "Flight time: " << transfer.flight_time.count() / 86400 << " days\n";

// Find launch windows
auto windows = mission.find_launch_windows(
    1.496e8, 2.279e8,
    start_time, end_time, 5
);
```

## Advanced Analysis

### Clustering

```cpp
#include <solar_analysis/advanced_analyzer.hpp>

AdvancedAnalyzer advanced;

std::vector<std::vector<double>> data = {
    {0, 0}, {1, 0}, {0, 1},      // Cluster 1
    {10, 10}, {11, 10}, {10, 11} // Cluster 2
};

auto result = advanced.kmeans_cluster(data, 2);
// result.clusters contains 2 clusters
```

### Optimization

```cpp
// Minimize f(x) = (x-3)²
auto objective = [](const std::vector<double>& p) {
    return (p[0] - 3) * (p[0] - 3);
};

auto result = advanced.minimize(objective, {0.0});
// result.optimal_params ≈ {3.0}
```

## Coordinate Systems

The library supports multiple coordinate systems and reference frames:

- **Cartesian** (x, y, z)
- **Spherical** (r, θ, φ)
- **J2000 Ecliptic** (default)
- **J2000 Equatorial**

```cpp
#include <solar_analysis/data_models.hpp>

// Convert coordinates
auto spherical = CoordinateConverter::cartesian_to_spherical(cartesian);
auto equatorial = CoordinateConverter::ecliptic_to_equatorial(ecliptic);
```

## Error Handling

All operations return result types indicating success/failure:

```cpp
auto result = exporter.export_data(data, path);
if (result.success) {
    std::cout << "Exported " << result.records_exported << " records\n";
} else {
    std::cerr << "Error: " << result.message << "\n";
}
```

## Performance Tips

1. **Use caching**: `DataProcessor` caches loaded data automatically
2. **Batch operations**: Use `BatchProcessor` for multiple analyses
3. **Appropriate precision**: Set `ExportConfig::precision` based on needs
4. **Time range filtering**: Load only needed data with `TimeRange`

## See Also

- [User Guide](../user-guide/USER_GUIDE.md)
- [API Documentation](../api/html/index.html)
- [Examples](ANALYSIS_EXAMPLES.md)
