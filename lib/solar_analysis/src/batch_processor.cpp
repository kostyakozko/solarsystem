/**
 * @file batch_processor.cpp
 * @brief Implementation of batch processing engine
 */

#include "solar_analysis/batch_processor.hpp"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <sstream>
#include <thread>

namespace SolarSystem::Analysis {

struct BatchProcessor::Impl {
  BatchConfig config;
  BatchProgressCallback progress_callback;

  std::mutex mutex;
  std::map<std::string, AnalysisJob> jobs;
  std::map<std::string, JobResult> results;
  std::queue<std::string> pending_queue;
  std::atomic<size_t> running_count{0};
  std::atomic<size_t> job_counter{0};

  DataProcessor data_processor;
  OrbitalCalculator orbital_calculator;
  StatisticalAnalyzer statistical_analyzer;
  DataExporter data_exporter;
};

BatchProcessor::BatchProcessor() : impl_(std::make_unique<Impl>()) {}

BatchProcessor::BatchProcessor(const BatchConfig& config) : impl_(std::make_unique<Impl>()) {
  impl_->config = config;
}

BatchProcessor::~BatchProcessor() = default;

void BatchProcessor::set_config(const BatchConfig& config) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->config = config;
}

void BatchProcessor::set_progress_callback(BatchProgressCallback callback) {
  impl_->progress_callback = std::move(callback);
}

std::string BatchProcessor::submit_job(const AnalysisJob& job) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  std::string job_id = job.id.empty() ? "job_" + std::to_string(++impl_->job_counter) : job.id;

  AnalysisJob job_copy = job;
  job_copy.id = job_id;

  impl_->jobs[job_id] = job_copy;
  JobResult initial_result;
  initial_result.job_id = job_id;
  initial_result.status = JobStatus::Pending;
  impl_->results[job_id] = initial_result;
  impl_->pending_queue.push(job_id);

  return job_id;
}

bool BatchProcessor::cancel_job(const std::string& job_id) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto it = impl_->results.find(job_id);
  if (it == impl_->results.end()) return false;

  if (it->second.status == JobStatus::Pending) {
    it->second.status = JobStatus::Cancelled;
    return true;
  }
  return false;
}

std::optional<JobResult> BatchProcessor::get_result(const std::string& job_id) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto it = impl_->results.find(job_id);
  if (it != impl_->results.end()) {
    return it->second;
  }
  return std::nullopt;
}

JobStatus BatchProcessor::get_status(const std::string& job_id) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto it = impl_->results.find(job_id);
  return it != impl_->results.end() ? it->second.status : JobStatus::Pending;
}

std::vector<std::string> BatchProcessor::submit_batch(const std::vector<AnalysisJob>& jobs) {
  std::vector<std::string> job_ids;
  job_ids.reserve(jobs.size());

  for (const auto& job : jobs) {
    job_ids.push_back(submit_job(job));
  }

  return job_ids;
}

void BatchProcessor::process_all() {
  while (!impl_->pending_queue.empty()) {
    std::string job_id;
    {
      std::lock_guard<std::mutex> lock(impl_->mutex);
      if (impl_->pending_queue.empty()) break;

      job_id = impl_->pending_queue.front();
      impl_->pending_queue.pop();

      if (impl_->results[job_id].status == JobStatus::Cancelled) {
        continue;
      }
    }

    auto job_it = impl_->jobs.find(job_id);
    if (job_it == impl_->jobs.end()) continue;

    ++impl_->running_count;
    auto result = execute_job(job_it->second);
    --impl_->running_count;

    {
      std::lock_guard<std::mutex> lock(impl_->mutex);
      impl_->results[job_id] = result;
    }

    if (impl_->config.stop_on_error && result.status == JobStatus::Failed) {
      break;
    }
  }
}

void BatchProcessor::wait_for_completion() { process_all(); }

size_t BatchProcessor::pending_count() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->pending_queue.size();
}

size_t BatchProcessor::running_count() const { return impl_->running_count.load(); }

size_t BatchProcessor::completed_count() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  size_t count = 0;
  for (const auto& [_, result] : impl_->results) {
    if (result.status == JobStatus::Completed || result.status == JobStatus::Failed) {
      ++count;
    }
  }
  return count;
}

void BatchProcessor::clear_completed() {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  std::vector<std::string> to_remove;
  for (const auto& [id, result] : impl_->results) {
    if (result.status == JobStatus::Completed || result.status == JobStatus::Failed ||
        result.status == JobStatus::Cancelled) {
      to_remove.push_back(id);
    }
  }

  for (const auto& id : to_remove) {
    impl_->results.erase(id);
    impl_->jobs.erase(id);
  }
}

JobResult BatchProcessor::execute_job(const AnalysisJob& job) {
  JobResult result;
  result.job_id = job.id;
  result.status = JobStatus::Running;
  result.start_time = std::chrono::system_clock::now();

  if (impl_->progress_callback) {
    impl_->progress_callback(job.id, 0.0, "Starting job: " + job.name);
  }

  try {
    // Load data for all bodies
    size_t total_records = 0;
    for (const auto& body : job.body_names) {
      bool loaded = false;
      if (job.time_range) {
        loaded = impl_->data_processor.load_body_data(body, *job.time_range);
      } else {
        loaded = impl_->data_processor.load_body_data(body);
      }
      if (loaded) {
        total_records += impl_->data_processor.data_point_count(body);
      }
    }

    if (impl_->progress_callback) {
      impl_->progress_callback(job.id, 0.3, "Data loaded");
    }

    // Run requested analyses
    for (const auto& analysis : job.analyses) {
      if (analysis == "orbital") {
        for (const auto& body : job.body_names) {
          auto data = impl_->data_processor.get_data(body);
          if (!data.empty()) {
            auto elements = impl_->orbital_calculator.calculate_elements(data.front());
            result.results[body + "_semi_major_axis"] = std::to_string(elements.semi_major_axis);
            result.results[body + "_eccentricity"] = std::to_string(elements.eccentricity);
            result.results[body + "_period"] = std::to_string(elements.orbital_period);
          }
        }
      } else if (analysis == "statistical") {
        for (const auto& body : job.body_names) {
          auto data = impl_->data_processor.get_data(body);
          std::vector<double> distances;
          for (const auto& sv : data) {
            distances.push_back(static_cast<double>(sv.position.magnitude()));
          }
          if (!distances.empty()) {
            auto stats = impl_->statistical_analyzer.calculate_statistics(distances);
            result.results[body + "_mean_distance"] = std::to_string(stats.mean);
            result.results[body + "_std_distance"] = std::to_string(stats.std_dev);
          }
        }
      }
    }

    if (impl_->progress_callback) {
      impl_->progress_callback(job.id, 0.7, "Analysis complete");
    }

    // Export if requested
    if (std::find(job.analyses.begin(), job.analyses.end(), "export") != job.analyses.end()) {
      impl_->data_exporter.set_config(job.export_config);

      for (const auto& body : job.body_names) {
        auto data = impl_->data_processor.get_data(body);
        std::string ext = (job.export_config.format == ExportFormat::JSON) ? ".json" : ".csv";
        auto path = job.output_path / (body + ext);
        auto export_result = impl_->data_exporter.export_data(data, path);
        if (export_result.success) {
          result.output_files.push_back(path.string());
        }
      }
    }

    result.records_processed = total_records;
    result.status = JobStatus::Completed;

    if (impl_->progress_callback) {
      impl_->progress_callback(job.id, 1.0, "Job completed");
    }

  } catch (const std::exception& e) {
    result.status = JobStatus::Failed;
    result.error_message = e.what();

    if (impl_->progress_callback) {
      impl_->progress_callback(job.id, 1.0, "Job failed: " + std::string(e.what()));
    }
  }

  result.end_time = std::chrono::system_clock::now();
  result.duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(result.end_time - result.start_time);

  return result;
}

AnalysisJob BatchProcessor::create_orbital_analysis_job(const std::string& name,
                                                        const std::vector<std::string>& bodies,
                                                        const std::filesystem::path& output) {
  AnalysisJob job;
  job.name = name;
  job.body_names = bodies;
  job.analyses = {"orbital", "export"};
  job.output_path = output;
  return job;
}

AnalysisJob BatchProcessor::create_statistical_analysis_job(const std::string& name,
                                                            const std::vector<std::string>& bodies,
                                                            const std::filesystem::path& output) {
  AnalysisJob job;
  job.name = name;
  job.body_names = bodies;
  job.analyses = {"statistical", "export"};
  job.output_path = output;
  return job;
}

AnalysisJob BatchProcessor::create_full_analysis_job(const std::string& name,
                                                     const std::vector<std::string>& bodies,
                                                     const std::filesystem::path& output) {
  AnalysisJob job;
  job.name = name;
  job.body_names = bodies;
  job.analyses = {"orbital", "statistical", "export"};
  job.output_path = output;
  return job;
}

// BatchConfigLoader implementation

std::vector<AnalysisJob> BatchConfigLoader::load_from_json(const std::string& json) {
  std::vector<AnalysisJob> jobs;

  // Simple JSON parsing - find job objects
  size_t pos = 0;
  while ((pos = json.find("\"name\"", pos)) != std::string::npos) {
    AnalysisJob job;

    // Extract name
    size_t start = json.find(':', pos) + 2;
    size_t end = json.find('"', start);
    if (start < json.size() && end != std::string::npos) {
      job.name = json.substr(start, end - start);
    }

    jobs.push_back(job);
    pos = end + 1;
  }

  return jobs;
}

std::string BatchConfigLoader::save_to_json(const std::vector<AnalysisJob>& jobs) {
  std::ostringstream oss;
  oss << "{\"jobs\":[";

  for (size_t i = 0; i < jobs.size(); ++i) {
    if (i > 0) oss << ",";
    const auto& job = jobs[i];
    oss << "{\"id\":\"" << job.id << "\",\"name\":\"" << job.name << "\",\"bodies\":[";

    for (size_t j = 0; j < job.body_names.size(); ++j) {
      if (j > 0) oss << ",";
      oss << "\"" << job.body_names[j] << "\"";
    }

    oss << "],\"analyses\":[";
    for (size_t j = 0; j < job.analyses.size(); ++j) {
      if (j > 0) oss << ",";
      oss << "\"" << job.analyses[j] << "\"";
    }
    oss << "]}";
  }

  oss << "]}";
  return oss.str();
}

}  // namespace SolarSystem::Analysis
