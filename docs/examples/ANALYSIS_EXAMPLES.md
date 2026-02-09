# Analysis Examples

Practical examples for common analysis workflows using the Solar System Analysis Library.

## Example 1: Orbital Parameter Analysis

Analyze orbital parameters for inner planets:

```cpp
#include <solar_analysis/data_processor.hpp>
#include <solar_analysis/orbital_calculator.hpp>
#include <solar_analysis/data_exporter.hpp>
#include <iostream>

int main() {
    using namespace SolarSystem::Analysis;
    
    DataProcessor processor;
    OrbitalCalculator calc;
    
    std::vector<std::string> planets = {"Mercury", "Venus", "Earth", "Mars"};
    
    std::cout << "Inner Planets Orbital Analysis\n";
    std::cout << "==============================\n\n";
    
    for (const auto& planet : planets) {
        processor.load_body_data(planet);
        auto data = processor.get_data(planet);
        
        if (!data.empty()) {
            auto elements = calc.calculate_elements(data.front());
            
            std::cout << planet << ":\n";
            std::cout << "  Semi-major axis: " << elements.semi_major_axis / 1e6 << " million km\n";
            std::cout << "  Eccentricity: " << elements.eccentricity << "\n";
            std::cout << "  Period: " << elements.orbital_period / 86400 << " days\n";
            std::cout << "  Perihelion: " << elements.periapsis / 1e6 << " million km\n";
            std::cout << "  Aphelion: " << elements.apoapsis / 1e6 << " million km\n\n";
        }
    }
    
    return 0;
}
```

## Example 2: Statistical Analysis of Distance Data

Analyze distance variations over time:

```cpp
#include <solar_analysis/data_processor.hpp>
#include <solar_analysis/statistical_analyzer.hpp>
#include <iostream>

int main() {
    using namespace SolarSystem::Analysis;
    
    DataProcessor processor;
    StatisticalAnalyzer stats;
    
    processor.load_body_data("Earth");
    auto data = processor.get_data("Earth");
    
    // Extract distances
    std::vector<double> distances;
    for (const auto& sv : data) {
        distances.push_back(static_cast<double>(sv.position.magnitude()));
    }
    
    // Calculate statistics
    auto summary = stats.calculate_statistics(distances);
    
    std::cout << "Earth Distance Statistics\n";
    std::cout << "=========================\n";
    std::cout << "Mean distance: " << summary.mean / 1e6 << " million km\n";
    std::cout << "Std deviation: " << summary.std_dev / 1e6 << " million km\n";
    std::cout << "Min distance: " << summary.min / 1e6 << " million km\n";
    std::cout << "Max distance: " << summary.max / 1e6 << " million km\n";
    std::cout << "Variation: " << (summary.max - summary.min) / 1e6 << " million km\n";
    
    // Detect anomalies
    auto anomalies = stats.detect_anomalies(distances, 2.5);
    std::cout << "\nAnomalies detected: " << anomalies.size() << "\n";
    
    return 0;
}
```

## Example 3: Mission Planning - Earth to Mars Transfer

Calculate Hohmann transfer trajectory:

```cpp
#include <solar_analysis/mission_analyzer.hpp>
#include <iostream>
#include <chrono>

int main() {
    using namespace SolarSystem::Analysis;
    
    MissionAnalyzer mission;
    
    // Orbital radii (km)
    double earth_radius = 1.496e8;  // 1 AU
    double mars_radius = 2.279e8;   // 1.52 AU
    
    // Calculate transfer
    auto transfer = mission.calculate_transfer(
        earth_radius, mars_radius, "Earth", "Mars"
    );
    
    std::cout << "Earth-Mars Hohmann Transfer\n";
    std::cout << "===========================\n";
    std::cout << "Departure delta-V: " << transfer.departure_delta_v << " km/s\n";
    std::cout << "Arrival delta-V: " << transfer.arrival_delta_v << " km/s\n";
    std::cout << "Total delta-V: " << transfer.total_delta_v << " km/s\n";
    std::cout << "Flight time: " << transfer.flight_time.count() / 86400 << " days\n";
    
    // Estimate fuel requirements
    double dry_mass = 1000;  // kg
    double isp = 300;        // seconds (typical chemical rocket)
    double fuel_mass = mission.estimate_fuel_mass(transfer.total_delta_v, dry_mass, isp);
    
    std::cout << "\nFuel Requirements (Isp=" << isp << "s):\n";
    std::cout << "Dry mass: " << dry_mass << " kg\n";
    std::cout << "Fuel mass: " << fuel_mass << " kg\n";
    std::cout << "Mass ratio: " << (dry_mass + fuel_mass) / dry_mass << "\n";
    
    return 0;
}
```

## Example 4: Batch Analysis with Export

Process multiple bodies and export results:

```cpp
#include <solar_analysis/batch_processor.hpp>
#include <iostream>

int main() {
    using namespace SolarSystem::Analysis;
    
    BatchProcessor batch;
    
    // Set progress callback
    batch.set_progress_callback([](const std::string& job_id, double progress, 
                                   const std::string& message) {
        std::cout << "[" << job_id << "] " << (progress * 100) << "% - " << message << "\n";
    });
    
    // Create jobs for different planet groups
    auto inner = BatchProcessor::create_full_analysis_job(
        "Inner Planets", {"Mercury", "Venus", "Earth", "Mars"}, "./output/inner"
    );
    
    auto outer = BatchProcessor::create_full_analysis_job(
        "Outer Planets", {"Jupiter", "Saturn", "Uranus", "Neptune"}, "./output/outer"
    );
    
    // Submit and process
    batch.submit_job(inner);
    batch.submit_job(outer);
    batch.process_all();
    
    // Report results
    std::cout << "\nCompleted: " << batch.completed_count() << " jobs\n";
    
    return 0;
}
```

## Example 5: Historical Trend Analysis

Analyze long-term orbital evolution:

```cpp
#include <solar_analysis/data_processor.hpp>
#include <solar_analysis/historical_analyzer.hpp>
#include <iostream>

int main() {
    using namespace SolarSystem::Analysis;
    
    DataProcessor processor;
    HistoricalAnalyzer historical;
    
    processor.load_body_data("Earth");
    auto data = processor.get_data("Earth");
    
    // Analyze distance trend
    auto trend = historical.analyze_long_term_trend(data, "distance");
    
    std::cout << "Earth Distance Trend Analysis\n";
    std::cout << "=============================\n";
    std::cout << "Trend direction: " << (trend.trend.direction > 0 ? "increasing" : 
                                         trend.trend.direction < 0 ? "decreasing" : "stable") << "\n";
    std::cout << "Trend strength: " << trend.trend.strength << "\n";
    std::cout << "Annual change: " << trend.annual_change << " km/year\n";
    
    return 0;
}
```

## Example 6: Clustering Celestial Bodies

Group bodies by orbital characteristics:

```cpp
#include <solar_analysis/data_processor.hpp>
#include <solar_analysis/orbital_calculator.hpp>
#include <solar_analysis/advanced_analyzer.hpp>
#include <iostream>

int main() {
    using namespace SolarSystem::Analysis;
    
    DataProcessor processor;
    OrbitalCalculator calc;
    AdvancedAnalyzer advanced;
    
    std::vector<std::string> bodies = {
        "Mercury", "Venus", "Earth", "Mars",
        "Jupiter", "Saturn", "Uranus", "Neptune"
    };
    
    // Extract orbital features
    std::vector<std::vector<double>> features;
    for (const auto& body : bodies) {
        processor.load_body_data(body);
        auto data = processor.get_data(body);
        if (!data.empty()) {
            auto elem = calc.calculate_elements(data.front());
            // Normalize features
            features.push_back({
                std::log10(elem.semi_major_axis),
                elem.eccentricity * 10
            });
        }
    }
    
    // Cluster into 2 groups (inner/outer)
    auto result = advanced.kmeans_cluster(features, 2);
    
    std::cout << "Celestial Body Clustering\n";
    std::cout << "=========================\n";
    for (size_t i = 0; i < bodies.size(); ++i) {
        std::cout << bodies[i] << " -> Cluster " << result.assignments[i] << "\n";
    }
    
    return 0;
}
```

## Example 7: Interactive CLI Script

Create an analysis script for the CLI:

```bash
# analysis_script.txt
# Load planets
load Mercury
load Venus
load Earth
load Mars

# Analyze each
orbital Mercury
orbital Venus
orbital Earth
orbital Mars

# Export data
export Earth earth_analysis.csv
export Mars mars_analysis.csv
```

Run with:
```bash
./solar_system_analyzer < analysis_script.txt
```

## Example 8: Data Export in Multiple Formats

```cpp
#include <solar_analysis/data_processor.hpp>
#include <solar_analysis/data_exporter.hpp>

int main() {
    using namespace SolarSystem::Analysis;
    
    DataProcessor processor;
    processor.load_body_data("Jupiter");
    auto data = processor.get_data("Jupiter");
    
    DataExporter exporter;
    
    // CSV export
    ExportConfig csv_config;
    csv_config.format = ExportFormat::CSV;
    csv_config.precision = 10;
    exporter.set_config(csv_config);
    exporter.export_data(data, "jupiter.csv");
    
    // JSON export
    ExportConfig json_config;
    json_config.format = ExportFormat::JSON;
    exporter.set_config(json_config);
    exporter.export_data(data, "jupiter.json");
    
    // XML export
    ExportConfig xml_config;
    xml_config.format = ExportFormat::XML;
    exporter.set_config(xml_config);
    exporter.export_data(data, "jupiter.xml");
    
    return 0;
}
```

## See Also

- [Analysis Library Documentation](../ANALYSIS_LIBRARY.md)
- [User Guide](../user-guide/USER_GUIDE.md)
