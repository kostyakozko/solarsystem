/**
 * @file compression.hpp
 * @brief Response compression for bandwidth optimization
 *
 * Provides compression with:
 * - gzip compression
 * - Automatic compression based on content type
 * - Compression level configuration
 */

#pragma once

#include <optional>
#include <string>
#include <vector>

namespace SolarSystem::Performance {

/**
 * @brief Compression algorithm
 */
enum class CompressionAlgorithm {
  NONE,
  GZIP,
  DEFLATE
};

/**
 * @brief Convert compression algorithm to string
 */
[[nodiscard]] std::string to_string(CompressionAlgorithm algorithm);

/**
 * @brief Compression configuration
 */
struct CompressionConfig {
  bool enabled = true;
  CompressionAlgorithm algorithm = CompressionAlgorithm::GZIP;
  int compression_level = 6;  // 1-9, higher = better compression but slower
  size_t min_size_bytes = 1024;  // Don't compress responses smaller than this
  std::vector<std::string> compressible_types = {
      "text/html",
      "text/css",
      "text/javascript",
      "application/javascript",
      "application/json",
      "text/plain"
  };
};

/**
 * @brief Response compressor
 */
class ResponseCompressor {
 public:
  /**
   * @brief Construct with configuration
   */
  explicit ResponseCompressor(CompressionConfig config = {});

  /**
   * @brief Compress data
   */
  [[nodiscard]] std::optional<std::string> compress(
      const std::string& data,
      CompressionAlgorithm algorithm = CompressionAlgorithm::GZIP) const;

  /**
   * @brief Decompress data
   */
  [[nodiscard]] std::optional<std::string> decompress(
      const std::string& data,
      CompressionAlgorithm algorithm = CompressionAlgorithm::GZIP) const;

  /**
   * @brief Check if content type should be compressed
   */
  [[nodiscard]] bool should_compress(
      const std::string& content_type,
      size_t content_length) const;

  /**
   * @brief Get compression ratio
   */
  [[nodiscard]] double get_compression_ratio(
      size_t original_size,
      size_t compressed_size) const;

 private:
  CompressionConfig config_;
};

}  // namespace SolarSystem::Performance
