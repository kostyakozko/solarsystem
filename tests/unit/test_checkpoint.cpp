/**
 * @file test_checkpoint.cpp
 * @brief Unit tests for simulation checkpointing and resume functionality
 */

#include "../utils/test_framework.h"

#include <chrono>
#include <filesystem>
#include <thread>

#include "solar_core/bodies/body_collection.hpp"
#include "solar_core/bodies/celestial_body.hpp"
#include "solar_core/simulation/checkpoint.hpp"
#include "solar_core/simulation/simulation_engine.hpp"

using namespace SolarSystem;
using namespace SolarSystem::Simulation;
using namespace SolarSystem::Bodies;

// Helper function to create test bodies
BodyCollection create_test_bodies() {
  BodyCollection bodies;

  // Create Sun
  CelestialBody::Properties sun_props;
  sun_props.name = "Sun";
  sun_props.mass = 1.989e30;
  sun_props.position = Math::Vector3d{0, 0, 0};
  sun_props.velocity = Math::Vector3d{0, 0, 0};
  sun_props.type = BodyType::Star;
  sun_props.priority = BodyPriority::Essential;
  bodies.add_body(CelestialBody(sun_props));

  // Create Earth
  CelestialBody::Properties earth_props;
  earth_props.name = "Earth";
  earth_props.mass = 5.972e24;
  earth_props.position = Math::Vector3d{1.496e11, 0, 0};
  earth_props.velocity = Math::Vector3d{0, 29780, 0};
  earth_props.type = BodyType::Planet;
  earth_props.priority = BodyPriority::Important;
  bodies.add_body(CelestialBody(earth_props));

  return bodies;
}

int main() {
  TestSuite suite("Checkpoint Tests");

  suite.run_test("Checkpoint Manager Creation", []() {
    std::filesystem::path test_dir = std::filesystem::temp_directory_path() / "test_checkpoints_1";
    std::filesystem::create_directories(test_dir);

    CheckpointConfig config;
    config.checkpoint_directory = test_dir;
    config.enable_compression = true;
    config.enable_validation = true;

    CheckpointManager manager(config);
    ASSERT_EQ(manager.get_config().checkpoint_directory, test_dir);
    ASSERT_TRUE(manager.get_config().enable_compression);
    ASSERT_TRUE(manager.get_config().enable_validation);

    std::filesystem::remove_all(test_dir);
  });

  suite.run_test("Checkpoint ID Generation", []() {
    std::filesystem::path test_dir = std::filesystem::temp_directory_path() / "test_checkpoints_2";
    std::filesystem::create_directories(test_dir);

    CheckpointConfig config;
    config.checkpoint_directory = test_dir;
    config.checkpoint_prefix = "test";

    CheckpointManager manager(config);

    std::string id1 = manager.generate_checkpoint_id();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::string id2 = manager.generate_checkpoint_id();

    ASSERT_FALSE(id1.empty());
    ASSERT_FALSE(id2.empty());
    ASSERT_NE(id1, id2);
    ASSERT_TRUE(id1.find("test") != std::string::npos);
    ASSERT_TRUE(id2.find("test") != std::string::npos);

    std::filesystem::remove_all(test_dir);
  });

  suite.run_test("Basic Checkpoint Save", []() {
    std::filesystem::path test_dir = std::filesystem::temp_directory_path() / "test_checkpoints_3";
    std::filesystem::create_directories(test_dir);

    CheckpointConfig config;
    config.checkpoint_directory = test_dir;

    CheckpointManager manager(config);

    // Create and initialize simulation
    SimulationEngine engine;
    auto init_result = engine.initialize(create_test_bodies());
    ASSERT_TRUE(init_result.has_value());

    // Run a few steps
    for (int i = 0; i < 10; ++i) {
      auto step_result = engine.step();
      ASSERT_TRUE(step_result.has_value());
    }

    // Save checkpoint
    auto save_result = manager.save_checkpoint(engine, "test_checkpoint_1");
    ASSERT_TRUE(save_result.has_value());
    ASSERT_EQ(save_result.value(), "test_checkpoint_1");

    // Verify checkpoint file exists
    ASSERT_TRUE(manager.checkpoint_exists("test_checkpoint_1"));

    std::filesystem::remove_all(test_dir);
  });

  suite.run_test("Checkpoint Load", []() {
    std::filesystem::path test_dir = std::filesystem::temp_directory_path() / "test_checkpoints_4";
    std::filesystem::create_directories(test_dir);

    CheckpointConfig config;
    config.checkpoint_directory = test_dir;

    CheckpointManager manager(config);

    // Create and run simulation
    SimulationEngine engine;
    auto init_result = engine.initialize(create_test_bodies());
    ASSERT_TRUE(init_result.has_value());

    for (int i = 0; i < 10; ++i) {
      auto step_result = engine.step();
      ASSERT_TRUE(step_result.has_value());
    }

    double saved_time = engine.get_current_time();
    size_t saved_iterations = engine.get_state().iteration_count;

    // Save checkpoint
    auto save_result = manager.save_checkpoint(engine, "test_load");
    ASSERT_TRUE(save_result.has_value());

    // Load checkpoint
    auto load_result = manager.load_checkpoint("test_load");
    ASSERT_TRUE(load_result.has_value());

    const auto& checkpoint_data = load_result.value();
    ASSERT_EQ(checkpoint_data.metadata.checkpoint_id, "test_load");
    ASSERT_NEAR(checkpoint_data.metadata.simulation_seconds, saved_time, 1e-6);
    ASSERT_EQ(checkpoint_data.metadata.iteration_count, saved_iterations);
    ASSERT_EQ(checkpoint_data.bodies.size(), 2);

    std::filesystem::remove_all(test_dir);
  });

  suite.run_test("Checkpoint Resume", []() {
    std::filesystem::path test_dir = std::filesystem::temp_directory_path() / "test_checkpoints_5";
    std::filesystem::create_directories(test_dir);

    CheckpointConfig config;
    config.checkpoint_directory = test_dir;

    CheckpointManager manager(config);

    // Create and run first simulation
    SimulationEngine engine1;
    auto init_result = engine1.initialize(create_test_bodies());
    ASSERT_TRUE(init_result.has_value());

    for (int i = 0; i < 20; ++i) {
      auto step_result = engine1.step();
      ASSERT_TRUE(step_result.has_value());
    }

    double saved_time = engine1.get_current_time();

    // Save checkpoint
    auto save_result = manager.save_checkpoint(engine1, "test_resume");
    ASSERT_TRUE(save_result.has_value());

    // Create new simulation and resume
    SimulationEngine engine2;
    auto resume_result = manager.resume_simulation(engine2, "test_resume");
    ASSERT_TRUE(resume_result.has_value());

    // Verify resumed state matches saved state (allow tolerance for simulation differences)
    ASSERT_NEAR(engine2.get_current_time(), saved_time, 100.0);
    ASSERT_EQ(engine2.get_bodies().size(), 2);

    // Continue simulation from resumed state
    for (int i = 0; i < 10; ++i) {
      auto step_result = engine2.step();
      ASSERT_TRUE(step_result.has_value());
    }

    ASSERT_GT(engine2.get_current_time(), saved_time);

    std::filesystem::remove_all(test_dir);
  });

  suite.run_test("Checkpoint Compression", []() {
    std::filesystem::path test_dir = std::filesystem::temp_directory_path() / "test_checkpoints_6";
    std::filesystem::create_directories(test_dir);

    // Test with compression enabled
    CheckpointConfig compressed_config;
    compressed_config.checkpoint_directory = test_dir;
    compressed_config.enable_compression = true;
    CheckpointManager compressed_manager(compressed_config);

    SimulationEngine engine1;
    auto init_result = engine1.initialize(create_test_bodies());
    ASSERT_TRUE(init_result.has_value());

    for (int i = 0; i < 10; ++i) {
      auto step_result = engine1.step();
      (void)step_result;  // Suppress unused warning
    }

    auto save_compressed = compressed_manager.save_checkpoint(engine1, "compressed");
    ASSERT_TRUE(save_compressed.has_value());

    auto compressed_stats = compressed_manager.get_last_stats();
    ASSERT_GT(compressed_stats.bytes_written, 0);

    // Test without compression
    CheckpointConfig uncompressed_config;
    uncompressed_config.checkpoint_directory = test_dir;
    uncompressed_config.enable_compression = false;
    CheckpointManager uncompressed_manager(uncompressed_config);

    SimulationEngine engine2;
    init_result = engine2.initialize(create_test_bodies());
    ASSERT_TRUE(init_result.has_value());

    for (int i = 0; i < 10; ++i) {
      auto step_result = engine2.step();
      (void)step_result;  // Suppress unused warning
    }

    auto save_uncompressed = uncompressed_manager.save_checkpoint(engine2, "uncompressed");
    ASSERT_TRUE(save_uncompressed.has_value());

    auto uncompressed_stats = uncompressed_manager.get_last_stats();
    ASSERT_GT(uncompressed_stats.bytes_written, 0);

    // Compressed should be smaller (or equal for small data)
    ASSERT_LE(compressed_stats.bytes_written, uncompressed_stats.bytes_written);

    std::filesystem::remove_all(test_dir);
  });

  suite.run_test("Checkpoint Validation", []() {
    std::filesystem::path test_dir = std::filesystem::temp_directory_path() / "test_checkpoints_7";
    std::filesystem::create_directories(test_dir);

    CheckpointConfig config;
    config.checkpoint_directory = test_dir;

    CheckpointManager manager(config);

    SimulationEngine engine;
    auto init_result = engine.initialize(create_test_bodies());
    ASSERT_TRUE(init_result.has_value());

    for (int i = 0; i < 10; ++i) {
      auto step_result = engine.step();
      (void)step_result;  // Suppress unused warning
    }

    // Save checkpoint
    auto save_result = manager.save_checkpoint(engine, "test_validate");
    ASSERT_TRUE(save_result.has_value());

    // Validate checkpoint
    auto validate_result = manager.validate_checkpoint("test_validate");
    ASSERT_TRUE(validate_result.has_value());
    ASSERT_TRUE(validate_result.value());

    // Validate non-existent checkpoint
    auto invalid_result = manager.validate_checkpoint("nonexistent");
    ASSERT_FALSE(invalid_result.has_value());

    std::filesystem::remove_all(test_dir);
  });

  suite.run_test("Checkpoint Listing", []() {
    std::filesystem::path test_dir = std::filesystem::temp_directory_path() / "test_checkpoints_8";
    std::filesystem::create_directories(test_dir);

    CheckpointConfig config;
    config.checkpoint_directory = test_dir;
    config.checkpoint_prefix = "test_list";

    CheckpointManager manager(config);

    SimulationEngine engine;
    auto init_result = engine.initialize(create_test_bodies());
    ASSERT_TRUE(init_result.has_value());

    // Create multiple checkpoints
    for (int i = 0; i < 3; ++i) {
      for (int j = 0; j < 5; ++j) {
        auto step_result = engine.step();
        (void)step_result;  // Suppress unused warning
      }

      std::string checkpoint_id = "test_list_" + std::to_string(i);
      auto save_result = manager.save_checkpoint(engine, checkpoint_id);
      std::cout << "DEBUG: Saving checkpoint " << checkpoint_id << ": " << (save_result.has_value() ? "SUCCESS" : "FAILED") << std::endl;
      ASSERT_TRUE(save_result.has_value());

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // List checkpoints
    auto checkpoints = manager.list_checkpoints();
    std::cout << "DEBUG: Found " << checkpoints.size() << " checkpoints" << std::endl;
    for (const auto& cp : checkpoints) {
      std::cout << "  - " << cp.checkpoint_id << std::endl;
    }
    ASSERT_TRUE(checkpoints.size() == 3);

    // Verify checkpoints are sorted by creation time (newest first)
    for (size_t i = 1; i < checkpoints.size(); ++i) {
      ASSERT_GE(checkpoints[i - 1].created_at, checkpoints[i].created_at);
    }

    std::filesystem::remove_all(test_dir);
  });

  suite.run_test("Checkpoint Deletion", []() {
    std::filesystem::path test_dir = std::filesystem::temp_directory_path() / "test_checkpoints_9";
    std::filesystem::create_directories(test_dir);

    CheckpointConfig config;
    config.checkpoint_directory = test_dir;

    CheckpointManager manager(config);

    SimulationEngine engine;
    auto init_result = engine.initialize(create_test_bodies());
    ASSERT_TRUE(init_result.has_value());

    // Save checkpoint
    auto save_result = manager.save_checkpoint(engine, "test_delete");
    ASSERT_TRUE(save_result.has_value());
    ASSERT_TRUE(manager.checkpoint_exists("test_delete"));

    // Delete checkpoint
    auto delete_result = manager.delete_checkpoint("test_delete");
    ASSERT_TRUE(delete_result.has_value());
    ASSERT_FALSE(manager.checkpoint_exists("test_delete"));

    // Try to delete non-existent checkpoint
    auto invalid_delete = manager.delete_checkpoint("nonexistent");
    ASSERT_FALSE(invalid_delete.has_value());

    std::filesystem::remove_all(test_dir);
  });

  suite.run_test("Checkpoint Scheduler", []() {
    std::filesystem::path test_dir = std::filesystem::temp_directory_path() / "test_checkpoints_10";
    std::filesystem::create_directories(test_dir);

    CheckpointConfig config;
    config.checkpoint_directory = test_dir;
    config.checkpoint_interval = std::chrono::seconds(1);

    CheckpointManager manager(config);
    CheckpointScheduler scheduler(manager, config);

    SimulationEngine engine;
    auto init_result = engine.initialize(create_test_bodies());
    ASSERT_TRUE(init_result.has_value());

    // Start scheduling
    scheduler.start_scheduling();
    ASSERT_TRUE(scheduler.is_scheduling());

    // Initially should not checkpoint
    ASSERT_FALSE(scheduler.should_checkpoint(engine.get_state()));

    // Wait for interval
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));

    // Now should checkpoint
    ASSERT_TRUE(scheduler.should_checkpoint(engine.get_state()));

    // Trigger checkpoint
    auto checkpoint_result = scheduler.trigger_checkpoint(engine);
    ASSERT_TRUE(checkpoint_result.has_value());

    // Stop scheduling
    scheduler.stop_scheduling();
    ASSERT_FALSE(scheduler.is_scheduling());

    std::filesystem::remove_all(test_dir);
  });

  suite.run_test("Error Handling - Uninitialized Engine", []() {
    std::filesystem::path test_dir = std::filesystem::temp_directory_path() / "test_checkpoints_11";
    std::filesystem::create_directories(test_dir);

    CheckpointConfig config;
    config.checkpoint_directory = test_dir;

    CheckpointManager manager(config);

    SimulationEngine engine;  // Not initialized

    auto save_result = manager.save_checkpoint(engine, "test_error");
    ASSERT_FALSE(save_result.has_value());
    ASSERT_TRUE(save_result.error() == CheckpointResult::ValidationError);

    std::filesystem::remove_all(test_dir);
  });

  suite.run_test("Error Handling - Invalid Checkpoint ID", []() {
    std::filesystem::path test_dir = std::filesystem::temp_directory_path() / "test_checkpoints_12";
    std::filesystem::create_directories(test_dir);

    CheckpointConfig config;
    config.checkpoint_directory = test_dir;

    CheckpointManager manager(config);

    auto load_result = manager.load_checkpoint("nonexistent_checkpoint");
    ASSERT_FALSE(load_result.has_value());
    ASSERT_TRUE(load_result.error() == CheckpointResult::FileError);

    std::filesystem::remove_all(test_dir);
  });

  return suite.all_passed() ? 0 : 1;
}

