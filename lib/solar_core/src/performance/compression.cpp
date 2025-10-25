/**
 * @file compression.cpp
 * @brief Implementation of response compression
 */

#include "solar_core/performance/compression.hpp"

#include <algorithm>
#include <zlib.h>

namespace SolarSystem::Performance {

std::string to_string(CompressionAlgorithm algorithm) {
  switch (algorithm) {
    case CompressionAlgorithm::NONE: return "none";
    case CompressionAlgorithm::GZIP: return "gzip";
    case CompressionAlgorithm::DEFLATE: return "deflate";
    default: return "unknown";
  }
}

ResponseCompressor::ResponseCompressor(CompressionConfig config)
    : config_(std::move(config)) {}

std::optional<std::string> ResponseCompressor::compress(
    const std::string& data,
    CompressionAlgorithm algorithm) const {
  if (!config_.enabled || algorithm == CompressionAlgorithm::NONE) {
    return data;
  }

  // Simple gzip compression using zlib
  z_stream stream;
  stream.zalloc = Z_NULL;
  stream.zfree = Z_NULL;
  stream.opaque = Z_NULL;

  int window_bits = (algorithm == CompressionAlgorithm::GZIP) ? 15 + 16 : 15;

  if (deflateInit2(&stream, config_.compression_level, Z_DEFLATED,
                   window_bits, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
    return std::nullopt;
  }

  stream.avail_in = static_cast<uInt>(data.size());
  stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));

  std::string compressed;
  compressed.resize(deflateBound(&stream, static_cast<uLong>(data.size())));

  stream.avail_out = static_cast<uInt>(compressed.size());
  stream.next_out = reinterpret_cast<Bytef*>(&compressed[0]);

  if (deflate(&stream, Z_FINISH) != Z_STREAM_END) {
    deflateEnd(&stream);
    return std::nullopt;
  }

  compressed.resize(stream.total_out);
  deflateEnd(&stream);

  return compressed;
}

std::optional<std::string> ResponseCompressor::decompress(
    const std::string& data,
    CompressionAlgorithm algorithm) const {
  if (algorithm == CompressionAlgorithm::NONE) {
    return data;
  }

  z_stream stream;
  stream.zalloc = Z_NULL;
  stream.zfree = Z_NULL;
  stream.opaque = Z_NULL;

  int window_bits = (algorithm == CompressionAlgorithm::GZIP) ? 15 + 16 : 15;

  if (inflateInit2(&stream, window_bits) != Z_OK) {
    return std::nullopt;
  }

  stream.avail_in = static_cast<uInt>(data.size());
  stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(data.data()));

  std::string decompressed;
  decompressed.resize(data.size() * 4);  // Estimate

  stream.avail_out = static_cast<uInt>(decompressed.size());
  stream.next_out = reinterpret_cast<Bytef*>(&decompressed[0]);

  if (inflate(&stream, Z_FINISH) != Z_STREAM_END) {
    inflateEnd(&stream);
    return std::nullopt;
  }

  decompressed.resize(stream.total_out);
  inflateEnd(&stream);

  return decompressed;
}

bool ResponseCompressor::should_compress(
    const std::string& content_type,
    size_t content_length) const {
  if (!config_.enabled || content_length < config_.min_size_bytes) {
    return false;
  }

  return std::find(config_.compressible_types.begin(),
                   config_.compressible_types.end(),
                   content_type) != config_.compressible_types.end();
}

double ResponseCompressor::get_compression_ratio(
    size_t original_size,
    size_t compressed_size) const {
  if (original_size == 0) return 0.0;
  return static_cast<double>(compressed_size) / static_cast<double>(original_size);
}

}  // namespace SolarSystem::Performance
