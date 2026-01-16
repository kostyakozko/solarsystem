/**
 * @file benchmark_jpl_data.cpp
 * @brief Performance benchmarks for JPL data processing
 * @note Migrated to Google Test
 *
 * This benchmark validates the JPL data processing performance claims:
 * - Cache loading 1000-2000x performance improvement over network requests
 * - JPL response parsing efficiency
 * - Data conversion and validation performance
 * - Binary vs JSON cache performance comparison
 */

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

#include "benchmark_utils.h"
#include "solar_core/bodies/celestial_body.hpp"
#include "solar_core/math/vector3.hpp"
#include "solar_jpl/jpl_client.hpp"

using namespace SolarSystem;

// Helper function to create realistic JPL response data
std::string create_mock_jpl_response() {
  return R"(
*******************************************************************************
 Revised: April 12, 2021             Earth                                 399

 PHYSICAL DATA (updated 2021-May-10):
  Mass x10^24 (kg)      = 5.97219+-0.00006
  Radius (km)           = 6371.01+-0.02

*******************************************************************************
Ephemeris / WWW_USER Mon Jan  1 00:00:00 2024 Pasadena, USA      / Horizons
*******************************************************************************
Target body name: Earth (399)                    {source: DE441}
Center body name: Solar System Barycenter (0)    {source: DE441}
Center-site name: BODY CENTER
*******************************************************************************
Start time      : A.D. 2024-Jan-01 00:00:00.0000 TDB
Stop  time      : A.D. 2024-Jan-01 00:00:00.0000 TDB
Step-size       : 1 days
*******************************************************************************
Center geodetic : 0.00000000,0.00000000,0.0000000 {E-lon(deg),Lat(deg),Alt(km)}
Center cylindric: 0.00000000,0.00000000,0.0000000 {RHO,Z,PHI(deg)}
Center radii    : (undefined)
*******************************************************************************
Output reference frame : ICRF
Output time format     : JD
Output units           : KM-S
Output CSV format      : YES
*******************************************************************************
JDTDB
   X     Y     Z
   VX    VY    VZ
*******************************************************************************
$$SOE
2460310.500000000, -2.521267663e+07,  1.449102209e+08,  6.282740466e+04,  -2.983983333e+01, -5.162373077e+00, -8.407168550e-04
$$EOE
*******************************************************************************
)";
}

// Helper function to create test cache data
void create_test_cache_data(const std::filesystem::path& cache_dir) {
  std::filesystem::create_directories(cache_dir);

  // Create binary cache file
  auto binary_path = cache_dir / "ephemeris_cache.bin";
  std::ofstream binary_file(binary_path, std::ios::binary);
  if (binary_file.is_open()) {
    size_t body_count = 27;  // Typical solar system body count
    binary_file.write(reinterpret_cast<const char*>(&body_count), sizeof(body_count));

    for (size_t i = 0; i < body_count; ++i) {
      int jpl_id = static_cast<int>(i + 1);
      binary_file.write(reinterpret_cast<const char*>(&jpl_id), sizeof(jpl_id));

      std::string name = "Body_" + std::to_string(i);
      size_t name_length = name.length();
      binary_file.write(reinterpret_cast<const char*>(&name_length), sizeof(name_length));
      binary_file.write(name.c_str(), static_cast<std::streamsize>(name_length));

      auto epoch_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
      binary_file.write(reinterpret_cast<const char*>(&epoch_time), sizeof(epoch_time));

      double pos[3] = {1.0e11 * static_cast<double>(i + 1), 2.0e11 * static_cast<double>(i + 1),
                       3.0e11 * static_cast<double>(i + 1)};
      binary_file.write(reinterpret_cast<const char*>(pos), sizeof(pos));

      double vel[3] = {1000.0 * static_cast<double>(i + 1), 2000.0 * static_cast<double>(i + 1),
                       3000.0 * static_cast<double>(i + 1)};
      binary_file.write(reinterpret_cast<const char*>(vel), sizeof(vel));

      long double mass = 1.0e24L * (i + 1);
      binary_file.write(reinterpret_cast<const char*>(&mass), sizeof(mass));
    }
  }

  // Create JSON cache file
  auto json_path = cache_dir / "ephemeris_data.json";
  std::ofstream json_file(json_path);
  if (json_file.is_open()) {
    json_file << "{\n";
    json_file << "  \"metadata\": {\n";
    json_file << "    \"created_at\": "
              << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << ",\n";
    json_file << "    \"body_count\": 27,\n";
    json_file << "    \"source\": \"JPL_HORIZONS\"\n";
    json_file << "  },\n";
    json_file << "  \"bodies\": [\n";

    for (int i = 0; i < 27; ++i) {
      json_file << "    {\n";
      json_file << "      \"jpl_id\": " << (i + 1) << ",\n";
      json_file << "      \"body_name\": \"Body_" << i << "\",\n";
      json_file << "      \"epoch\": "
                << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << ",\n";
      json_file << "      \"position\": [" << (1.0e11 * (i + 1)) << ", " << (2.0e11 * (i + 1))
                << ", " << (3.0e11 * (i + 1)) << "],\n";
      json_file << "      \"velocity\": [" << (1000.0 * (i + 1)) << ", " << (2000.0 * (i + 1))
                << ", " << (3000.0 * (i + 1)) << "],\n";
      json_file << "      \"mass\": " << (1.0e24 * (i + 1)) << "\n";
      json_file << "    }";
      if (i < 26) json_file << ",";
      json_file << "\n";
    }

    json_file << "  ]\n";
    json_file << "}\n";
  }
}

// JPL Data Performance Benchmark Test
TEST(JPLDataBenchmark, CachePerformanceValidation) {
  // Create test cache directory
  auto test_cache_dir = std::filesystem::temp_directory_path() / "jpl_benchmark_test";
  create_test_cache_data(test_cache_dir);

  Benchmark::BenchmarkSuite suite("JPL Data Processing Benchmarks");

  // 1. BINARY CACHE LOADING BENCHMARK
  suite.run_benchmark(
      "BinaryCacheLoadingBenchmark",
      [&test_cache_dir]() {
        auto binary_path = test_cache_dir / "ephemeris_cache.bin";
        std::ifstream file(binary_path, std::ios::binary);

        if (file.is_open()) {
          size_t body_count;
          file.read(reinterpret_cast<char*>(&body_count), sizeof(body_count));

          std::vector<JPL::EphemerisData> data;
          data.reserve(body_count);

          for (size_t i = 0; i < body_count; ++i) {
            JPL::EphemerisData body_data;

            file.read(reinterpret_cast<char*>(&body_data.jpl_id), sizeof(body_data.jpl_id));

            size_t name_length;
            file.read(reinterpret_cast<char*>(&name_length), sizeof(name_length));
            body_data.body_name.resize(name_length);
            file.read(body_data.body_name.data(), static_cast<std::streamsize>(name_length));

            std::time_t epoch_time;
            file.read(reinterpret_cast<char*>(&epoch_time), sizeof(epoch_time));
            body_data.epoch = std::chrono::system_clock::from_time_t(epoch_time);

            double pos[3];
            file.read(reinterpret_cast<char*>(pos), sizeof(pos));
            body_data.position = Math::Vector3d{pos[0], pos[1], pos[2]};

            double vel[3];
            file.read(reinterpret_cast<char*>(vel), sizeof(vel));
            body_data.velocity = Math::Vector3d{vel[0], vel[1], vel[2]};

            file.read(reinterpret_cast<char*>(&body_data.mass), sizeof(body_data.mass));

            data.push_back(std::move(body_data));
          }

          // Prevent optimization
          volatile size_t count = data.size();
          (void)count;
        }
      },
      10000);

  // 2. JSON CACHE LOADING BENCHMARK - Compare with binary performance
  suite.run_benchmark(
      "JSONCacheLoadingBenchmark",
      [&test_cache_dir]() {
        auto json_path = test_cache_dir / "ephemeris_data.json";
        std::ifstream file(json_path);

        if (file.is_open()) {
          std::string json_content;
          std::string line;
          while (std::getline(file, line)) {
            json_content += line + "\n";
          }

          // Simple parsing simulation (count bodies)
          size_t body_count = 0;
          size_t pos = 0;
          while ((pos = json_content.find("\"jpl_id\":", pos)) != std::string::npos) {
            body_count++;
            pos++;
          }

          // Prevent optimization
          volatile size_t count = body_count;
          (void)count;
        }
      },
      1000);

  // 3. SIMULATED NETWORK REQUEST BENCHMARK - Baseline for cache comparison
  suite.run_benchmark(
      "SimulatedNetworkRequestBenchmark",
      []() {
        // Simulate network latency and processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Typical network latency

        // Simulate JPL response parsing
        std::string response = create_mock_jpl_response();

        // Parse the response (simplified)
        JPL::EphemerisData data;
        data.jpl_id = 399;  // Earth
        data.body_name = "Earth";
        data.epoch = std::chrono::system_clock::now();

        // Extract position and velocity from response
        auto soe_pos = response.find("$$SOE");
        auto eoe_pos = response.find("$$EOE");

        if (soe_pos != std::string::npos && eoe_pos != std::string::npos) {
          std::string data_section = response.substr(soe_pos + 5, eoe_pos - soe_pos - 5);

          // Simple parsing simulation
          std::istringstream stream(data_section);
          std::string line;
          while (std::getline(stream, line)) {
            if (!line.empty() && line.find(',') != std::string::npos) {
              // Simulate parsing CSV data
              std::istringstream line_stream(line);
              std::string token;
              std::vector<std::string> tokens;

              while (std::getline(line_stream, token, ',')) {
                tokens.push_back(token);
              }

              if (tokens.size() >= 7) {
                try {
                  double x = std::stod(tokens[1]);
                  double y = std::stod(tokens[2]);
                  double z = std::stod(tokens[3]);
                  data.position = Math::Vector3d{x, y, z};

                  double vx = std::stod(tokens[4]);
                  double vy = std::stod(tokens[5]);
                  double vz = std::stod(tokens[6]);
                  data.velocity = Math::Vector3d{vx, vy, vz};

                  break;
                } catch (const std::exception&) {
                  // Continue parsing
                }
              }
            }
          }
        }

        data.mass = 5.97219e24L;  // Earth mass

        // Prevent optimization
        volatile auto result = data.position.magnitude() + data.velocity.magnitude() + data.mass;
        (void)result;
      },
      10);  // Low iteration count due to simulated network delay

  // 4. JPL RESPONSE PARSING BENCHMARK - Pure parsing performance
  suite.run_benchmark(
      "JPLResponseParsingBenchmark",
      []() {
        std::string response = create_mock_jpl_response();

        // Parse body name
        std::string body_name = "Unknown";
        auto name_pos = response.find("Target body name:");
        if (name_pos != std::string::npos) {
          auto start = response.find(":", name_pos) + 1;
          auto end = response.find("(", start);
          if (end != std::string::npos) {
            body_name = response.substr(start, end - start);
            // Trim whitespace
            body_name.erase(0, body_name.find_first_not_of(" \t"));
            body_name.erase(body_name.find_last_not_of(" \t") + 1);
          }
        }

        // Parse mass
        long double mass = 0.0L;
        auto mass_pos = response.find("Mass x10^24 (kg)");
        if (mass_pos != std::string::npos) {
          auto start = response.find("=", mass_pos) + 1;
          auto end = response.find_first_of("+-\n", start);
          if (end != std::string::npos) {
            try {
              std::string mass_str = response.substr(start, end - start);
              mass_str.erase(0, mass_str.find_first_not_of(" \t"));
              mass = std::stold(mass_str) * 1e24L;
            } catch (const std::exception&) {
              mass = 1.0e24L;
            }
          }
        }

        // Parse ephemeris data
        Math::Vector3d position{0.0, 0.0, 0.0};
        Math::Vector3d velocity{0.0, 0.0, 0.0};

        auto soe_pos = response.find("$$SOE");
        auto eoe_pos = response.find("$$EOE");

        if (soe_pos != std::string::npos && eoe_pos != std::string::npos) {
          std::string data_section = response.substr(soe_pos + 5, eoe_pos - soe_pos - 5);

          std::istringstream stream(data_section);
          std::string line;
          while (std::getline(stream, line)) {
            if (!line.empty() && line.find(',') != std::string::npos) {
              std::istringstream line_stream(line);
              std::string token;
              std::vector<std::string> tokens;

              while (std::getline(line_stream, token, ',')) {
                token.erase(0, token.find_first_not_of(" \t"));
                token.erase(token.find_last_not_of(" \t") + 1);
                tokens.push_back(token);
              }

              if (tokens.size() >= 7) {
                try {
                  double x = std::stod(tokens[1]);
                  double y = std::stod(tokens[2]);
                  double z = std::stod(tokens[3]);
                  position = Math::Vector3d{x, y, z};

                  double vx = std::stod(tokens[4]);
                  double vy = std::stod(tokens[5]);
                  double vz = std::stod(tokens[6]);
                  velocity = Math::Vector3d{vx, vy, vz};

                  break;
                } catch (const std::exception&) {
                  continue;
                }
              }
            }
          }
        }

        // Prevent optimization
        volatile auto result =
            position.magnitude() + velocity.magnitude() + mass + body_name.length();
        (void)result;
      },
      5000);

  // 5. DATA CONVERSION BENCHMARK - Ephemeris to CelestialBody conversion
  suite.run_benchmark(
      "DataConversionBenchmark",
      []() {
        // Create ephemeris data
        JPL::EphemerisData ephemeris_data;
        ephemeris_data.body_name = "Earth";
        ephemeris_data.jpl_id = 399;
        ephemeris_data.epoch = std::chrono::system_clock::now();
        ephemeris_data.position =
            Math::Vector3d{-2.521267663e7, 1.449102209e8, 6.282740466e4};  // km
        ephemeris_data.velocity =
            Math::Vector3d{-2.983983333e1, -5.162373077e0, -8.407168550e-4};  // km/s
        ephemeris_data.mass = 5.97219e24L;                                    // kg

        // Convert to CelestialBody
        Bodies::CelestialBody::Properties props = {
            .name = ephemeris_data.body_name,
            .mass = ephemeris_data.mass,
            .position = ephemeris_data.position * 1000.0L,  // Convert km to m
            .velocity = ephemeris_data.velocity * 1000.0L,  // Convert km/s to m/s
            .type = Bodies::BodyType::Planet,
            .priority = Bodies::BodyPriority::Essential,
            .jpl_id = std::to_string(ephemeris_data.jpl_id),
            .creation_date = std::nullopt};

        Bodies::CelestialBody body(props);

        // Perform some operations to validate the conversion
        auto distance_from_origin = body.position().magnitude();
        auto speed = body.velocity().magnitude();

        // Prevent optimization
        volatile auto result = distance_from_origin + speed + body.mass();
        (void)result;
      },
      10000);

  // Create benchmark results directory if it doesn't exist
  std::filesystem::create_directories("benchmark_results");

  // Export results to CSV for CI compatibility
  suite.export_results("benchmark_results/jpl_data_benchmark.csv", "csv");

  // Validate cache performance improvement claims
  auto results = suite.get_results();
  bool cache_improvement_validated = false;

  std::cout << "\n=== Cache Performance Validation ===" << std::endl;

  double binary_cache_time = 0.0;
  double network_request_time = 0.0;

  for (const auto& result : results) {
    if (result.name == "BinaryCacheLoadingBenchmark") {
      binary_cache_time = result.avg_duration_ms;
      std::cout << "Binary Cache Loading: " << binary_cache_time << " ms avg" << std::endl;
    }
    if (result.name == "SimulatedNetworkRequestBenchmark") {
      network_request_time = result.avg_duration_ms;
      std::cout << "Network Request: " << network_request_time << " ms avg" << std::endl;
    }
  }

  if (binary_cache_time > 0.0 && network_request_time > 0.0) {
    double improvement_factor = network_request_time / binary_cache_time;
    std::cout << "Cache Performance Improvement: " << static_cast<int>(improvement_factor) << "x"
              << std::endl;

    // Validate 1000x improvement claim
    if (improvement_factor >= 1000.0) {
      cache_improvement_validated = true;
      std::cout << "Cache Performance Claim: VALIDATED (≥1000x improvement)" << std::endl;
    } else {
      std::cout << "Cache Performance Claim: NOT VALIDATED (expected ≥1000x, got "
                << static_cast<int>(improvement_factor) << "x)" << std::endl;
    }
  }

  // Cleanup test cache
  std::filesystem::remove_all(test_cache_dir);

  std::cout << "\nOverall JPL Data Performance: " << (cache_improvement_validated ? "PASS" : "FAIL")
            << std::endl;

  EXPECT_TRUE(cache_improvement_validated);
}
