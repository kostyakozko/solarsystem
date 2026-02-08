#pragma once

/**
 * @file batch_processor.hpp
 * @brief Batch processing engine for automated analysis workflows
 */

#include <chrono>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <solar_analysis/data_exporter.hpp>
#include <solar_analysis/data_processor.hpp>
#include <solar_analysis/export.hpp>
#include <solar_analysis/orbital_calculator.hpp>
#include <solar_analysis/statistical_analyzer.hpp>
#include <string>
#include <vector>

namespace SolarSystem::Analysis {

/**
 * @brief Job status
 */
enum class JobStatus { Pending, Running, Completed, Failed, Cancelled };

/**
 * @brief Job priority
 */
enum class JobPriority { Low, Normal, High, Critical };

/**
 * @brief Analysis job definition
 */
struct SOLAR_ANALYSIS_API AnalysisJob {
  std::string id;
  std::string name;
  std::string description;
  JobPriority priority = JobPriority::Normal;
  std::vector<std::string> body_names;
  std::optional<TimeRange> time_range;
  std::vector<std::string> analyses;  // "orbital", "statistical", "export"
  ExportConfig export_config;
  std::filesystem::path output_path;
};

/**
 * @brief Job result
 */
struct SOLAR_ANALYSIS_API JobResult {
  std::string job_id;
  JobStatus status = JobStatus::Pending;
  std::chrono::system_clock::time_point start_time;
  std::chrono::system_clock::time_point end_time;
  std::chrono::milliseconds duration{0};
  size_t records_processed = 0;
  std::vector<std::string> output_files;
  std::optional<std::string> error_message;
  std::map<std::string, std::string> results;  // Key-value results
};

/**
 * @brief Progress callback for batch processing
 */
using BatchProgressCallback =
    std::function<void(const std::string& job_id, double progress, const std::string& message)>;

/**
 * @brief Batch processor configuration
 */
struct SOLAR_ANALYSIS_API BatchConfig {
  size_t max_concurrent_jobs = 4;
  bool stop_on_error = false;
  bool save_intermediate_results = true;
  std::chrono::seconds job_timeout{3600};
  std::filesystem::path working_directory = ".";
};

/**
 * @brief Batch processor for automated analysis workflows
 */
class SOLAR_ANALYSIS_API BatchProcessor {
 public:
  BatchProcessor();
  explicit BatchProcessor(const BatchConfig& config);
  ~BatchProcessor();

  BatchProcessor(const BatchProcessor&) = delete;
  BatchProcessor& operator=(const BatchProcessor&) = delete;

  // Configuration
  void set_config(const BatchConfig& config);
  void set_progress_callback(BatchProgressCallback callback);

  // Job management
  std::string submit_job(const AnalysisJob& job);
  bool cancel_job(const std::string& job_id);
  [[nodiscard]] std::optional<JobResult> get_result(const std::string& job_id) const;
  [[nodiscard]] JobStatus get_status(const std::string& job_id) const;

  // Batch operations
  std::vector<std::string> submit_batch(const std::vector<AnalysisJob>& jobs);
  void process_all();
  void wait_for_completion();

  // Queue management
  [[nodiscard]] size_t pending_count() const;
  [[nodiscard]] size_t running_count() const;
  [[nodiscard]] size_t completed_count() const;
  void clear_completed();

  // Workflow templates
  [[nodiscard]] static AnalysisJob create_orbital_analysis_job(
      const std::string& name, const std::vector<std::string>& bodies,
      const std::filesystem::path& output);

  [[nodiscard]] static AnalysisJob create_statistical_analysis_job(
      const std::string& name, const std::vector<std::string>& bodies,
      const std::filesystem::path& output);

  [[nodiscard]] static AnalysisJob create_full_analysis_job(const std::string& name,
                                                            const std::vector<std::string>& bodies,
                                                            const std::filesystem::path& output);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  JobResult execute_job(const AnalysisJob& job);
};

/**
 * @brief Batch job configuration loader
 */
class SOLAR_ANALYSIS_API BatchConfigLoader {
 public:
  [[nodiscard]] static std::vector<AnalysisJob> load_from_json(const std::string& json);
  [[nodiscard]] static std::string save_to_json(const std::vector<AnalysisJob>& jobs);
};

}  // namespace SolarSystem::Analysis
