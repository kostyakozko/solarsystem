/**
 * @file streaming_output_manager.cpp
 * @brief Implementation of streaming output manager for large result sets
 */

#include <algorithm>
#include <iostream>

#include "solar_test/formatters/output_formatter.hpp"

namespace SolarSystem::Testing::Formatters {

// StreamingOutputManager implementation
StreamingOutputManager::StreamingOutputManager(std::unique_ptr<OutputFormatter> formatter,
                                               const StreamingConfig& config)
    : formatter_(std::move(formatter)), config_(config) {
  buffer_.str().reserve(config_.buffer_size);
}

void StreamingOutputManager::start_output(std::ostream& output) {
  output_stream_ = &output;
  start_time_ = std::chrono::steady_clock::now();
  stats_ = StreamingStats{};

  formatter_->start_streaming(output);
}

void StreamingOutputManager::add_test_result(const TestResult& result) {
  if (!output_stream_) {
    throw std::runtime_error("Output not started");
  }

  // Check memory limits before processing
  check_memory_limits();

  // Stream the result
  formatter_->stream_test_result(result, *output_stream_);

  stats_.total_results_processed++;

  // Update memory usage estimate
  size_t estimated_size =
      result.test_name.size() + result.error_message.size() + 200;  // Base overhead
  stats_.bytes_written += estimated_size;

  // Flush if needed
  if (config_.auto_flush && stats_.total_results_processed % 100 == 0) {
    flush_buffer();
  }

  // Optimize memory usage periodically
  if (config_.enable_chunked_processing &&
      stats_.total_results_processed % config_.chunk_size == 0) {
    optimize_memory_usage();
  }
}

void StreamingOutputManager::add_test_suite(const TestSuiteResult& suite) {
  if (!output_stream_) {
    throw std::runtime_error("Output not started");
  }

  // For streaming, we process individual test results
  for (const auto& result : suite.test_results) {
    add_test_result(result);
  }
}

void StreamingOutputManager::finish_output() {
  if (output_stream_) {
    formatter_->end_streaming(*output_stream_);
    flush_buffer();

    // Calculate final statistics
    auto end_time = std::chrono::steady_clock::now();
    stats_.total_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_);

    output_stream_ = nullptr;
  }
}

void StreamingOutputManager::flush_buffer() {
  if (output_stream_) {
    output_stream_->flush();
    stats_.flush_operations++;
  }
}

void StreamingOutputManager::optimize_memory_usage() {
  // Clear oversized buffers and reclaim memory

  // Force garbage collection of any cached data
  if (static_cast<size_t>(buffer_.tellp()) > config_.buffer_size) {
    buffer_.str("");
    buffer_.clear();
  }

  // Shrink internal buffer to fit actual content
  buffer_.str(buffer_.str());

  // Update peak memory usage
  size_t current_usage = get_memory_usage();
  if (current_usage > stats_.peak_memory_usage) {
    stats_.peak_memory_usage = current_usage;
  }
}

size_t StreamingOutputManager::get_memory_usage() const {
  // Estimate current memory usage
  size_t usage = buffer_.str().size();
  usage += stats_.total_results_processed * 100;  // Rough estimate per result
  return usage;
}

void StreamingOutputManager::check_memory_limits() {
  size_t current_usage = get_memory_usage();

  if (current_usage > config_.max_memory_usage) {
    // Force optimization
    optimize_memory_usage();

    // If still over limit, warn but continue
    current_usage = get_memory_usage();
    if (current_usage > config_.max_memory_usage) {
      std::cerr << "Warning: Memory usage (" << current_usage << " bytes) exceeds limit ("
                << config_.max_memory_usage << " bytes)" << std::endl;
    }
  }
}

std::string StreamingOutputManager::compress_data(const std::string& data) {
  // Lightweight run-length encoding for test output compression.
  // External compression libraries (zlib, lz4, zstd) are available in the
  // main build but avoided here to keep the test framework dependency-free.

  if (data.size() < config_.compression_threshold) {
    return data;  // Don't compress small data
  }

  // Simple run-length encoding as a placeholder
  std::string compressed;
  compressed.reserve(data.size());

  for (size_t i = 0; i < data.size();) {
    char current = data[i];
    size_t count = 1;

    while (i + count < data.size() && data[i + count] == current && count < 255) {
      count++;
    }

    if (count > 3) {
      compressed += "RLE";
      compressed += static_cast<char>(count);
      compressed += current;
    } else {
      compressed.append(data, i, count);
    }

    i += count;
  }

  // Update compression ratio
  if (!data.empty()) {
    stats_.compression_ratio = static_cast<size_t>(
        (static_cast<double>(compressed.size()) / static_cast<double>(data.size())) * 100);
  }

  return compressed;
}

}  // namespace SolarSystem::Testing::Formatters
