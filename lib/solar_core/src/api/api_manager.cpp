/**
 * @file api_manager.cpp
 * @brief Implementation of API management
 */

#include "solar_core/api/api_manager.hpp"

#include <algorithm>
#include <mutex>
#include <sstream>

namespace SolarSystem::API {

// APIVersion implementation
std::string APIVersion::to_string() const {
  return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
}

std::optional<APIVersion> APIVersion::parse(const std::string& str) {
  APIVersion version;
  std::istringstream iss(str);
  char dot;

  if (!(iss >> version.major >> dot >> version.minor >> dot >> version.patch)) {
    return std::nullopt;
  }

  return version;
}

// APIManager implementation
struct APIManager::Impl {
  std::vector<APIEndpoint> endpoints;
  std::shared_ptr<Security::RateLimiter> rate_limiter;
  APIStatistics stats;
  bool monitoring_enabled = true;
  mutable std::mutex mutex;

  std::optional<APIEndpoint> find_endpoint(const std::string& path,
                                           Performance::HttpMethod method,
                                           const APIVersion& version) {
    for (const auto& endpoint : endpoints) {
      if (endpoint.path == path && endpoint.method == method &&
          endpoint.version.major == version.major) {
        return endpoint;
      }
    }
    return std::nullopt;
  }

  void record_request(const APIRequestMetadata& metadata) {
    if (!monitoring_enabled) return;

    stats.total_requests++;

    if (metadata.status_code >= 200 && metadata.status_code < 300) {
      stats.successful_requests++;
    } else {
      stats.failed_requests++;
    }

    stats.endpoint_usage[metadata.endpoint_path]++;
    stats.status_code_distribution[metadata.status_code]++;

    // Update average response time
    auto total_time = static_cast<long long>(stats.avg_response_time.count()) *
                      static_cast<long long>(stats.total_requests - 1) +
                      static_cast<long long>(metadata.response_time.count());
    stats.avg_response_time = std::chrono::milliseconds(total_time / static_cast<long long>(stats.total_requests));
  }
};

APIManager::APIManager() : impl_(std::make_unique<Impl>()) {}

APIManager::~APIManager() = default;

APIManager::APIManager(APIManager&&) noexcept = default;
APIManager& APIManager::operator=(APIManager&&) noexcept = default;

void APIManager::register_endpoint(const APIEndpoint& endpoint) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->endpoints.push_back(endpoint);
}

Performance::HttpResponse APIManager::handle_request(
    const Performance::HttpRequest& request) {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  auto start_time = std::chrono::steady_clock::now();

  // Extract API version from path or header
  APIVersion version{1, 0, 0};
  if (request.headers.count("API-Version")) {
    auto parsed = APIVersion::parse(request.headers.at("API-Version"));
    if (parsed) {
      version = *parsed;
    }
  }

  // Find matching endpoint
  auto endpoint = impl_->find_endpoint(request.path, request.method, version);

  if (!endpoint) {
    Performance::HttpResponseBuilder builder;
    auto response = builder.status(404, "Not Found")
                        .json("{\"error\": \"Endpoint not found\"}")
                        .build();

    APIRequestMetadata metadata;
    metadata.endpoint_path = request.path;
    metadata.method = request.method;
    metadata.version = version;
    metadata.timestamp = std::chrono::system_clock::now();
    metadata.client_ip = request.client_ip;
    metadata.status_code = 404;
    metadata.response_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time);

    impl_->record_request(metadata);

    return response;
  }

  // Check rate limiting
  if (endpoint->rate_limited && impl_->rate_limiter) {
    auto rate_result = impl_->rate_limiter->check_limit(request.client_ip);
    if (!rate_result.allowed) {
      impl_->stats.rate_limited_requests++;

      Performance::HttpResponseBuilder builder;
      auto response = builder.status(429, "Too Many Requests")
                          .header("Retry-After", std::to_string(rate_result.retry_after.count()))
                          .json("{\"error\": \"Rate limit exceeded\"}")
                          .build();

      APIRequestMetadata metadata;
      metadata.endpoint_path = request.path;
      metadata.method = request.method;
      metadata.version = version;
      metadata.timestamp = std::chrono::system_clock::now();
      metadata.client_ip = request.client_ip;
      metadata.status_code = 429;
      metadata.response_time = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - start_time);

      impl_->record_request(metadata);

      return response;
    }
  }

  // Validate request
  std::string validation_error;
  if (!APIValidator::validate_request(request, *endpoint, &validation_error)) {
    Performance::HttpResponseBuilder builder;
    auto response = builder.status(400, "Bad Request")
                        .json("{\"error\": \"" + validation_error + "\"}")
                        .build();

    APIRequestMetadata metadata;
    metadata.endpoint_path = request.path;
    metadata.method = request.method;
    metadata.version = version;
    metadata.timestamp = std::chrono::system_clock::now();
    metadata.client_ip = request.client_ip;
    metadata.status_code = 400;
    metadata.response_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time);

    impl_->record_request(metadata);

    return response;
  }

  // Handle request
  auto response = endpoint->handler(request);

  // Record metrics
  APIRequestMetadata metadata;
  metadata.endpoint_path = request.path;
  metadata.method = request.method;
  metadata.version = version;
  metadata.timestamp = std::chrono::system_clock::now();
  metadata.client_ip = request.client_ip;
  metadata.status_code = response.status_code;
  metadata.request_size = request.body.size();
  metadata.response_size = response.body.size();
  metadata.response_time = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start_time);

  impl_->record_request(metadata);

  return response;
}

std::string APIManager::get_documentation(const std::string& format) const {
  std::lock_guard<std::mutex> lock(impl_->mutex);

  if (format == "openapi" || format == "json") {
    return APIDocumentationGenerator::generate_openapi(impl_->endpoints);
  } else if (format == "markdown" || format == "md") {
    return APIDocumentationGenerator::generate_markdown(impl_->endpoints);
  } else if (format == "html") {
    return APIDocumentationGenerator::generate_html(impl_->endpoints);
  }

  return "{}";
}

APIStatistics APIManager::get_statistics() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->stats;
}

std::map<std::string, size_t> APIManager::get_endpoint_usage() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->stats.endpoint_usage;
}

void APIManager::set_rate_limiter(std::shared_ptr<Security::RateLimiter> limiter) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->rate_limiter = std::move(limiter);
}

void APIManager::set_monitoring_enabled(bool enabled) {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  impl_->monitoring_enabled = enabled;
}

std::vector<APIEndpoint> APIManager::get_endpoints() const {
  std::lock_guard<std::mutex> lock(impl_->mutex);
  return impl_->endpoints;
}

// APIDocumentationGenerator implementation
std::string APIDocumentationGenerator::generate_openapi(
    const std::vector<APIEndpoint>& endpoints,
    const std::string& title,
    const APIVersion& version) {
  std::ostringstream oss;

  oss << "{\n";
  oss << "  \"openapi\": \"3.0.0\",\n";
  oss << "  \"info\": {\n";
  oss << "    \"title\": \"" << title << "\",\n";
  oss << "    \"version\": \"" << version.to_string() << "\"\n";
  oss << "  },\n";
  oss << "  \"paths\": {\n";

  for (size_t i = 0; i < endpoints.size(); ++i) {
    const auto& endpoint = endpoints[i];

    oss << "    \"" << endpoint.path << "\": {\n";
    oss << "      \"" << Performance::to_string(endpoint.method) << "\": {\n";
    oss << "        \"summary\": \"" << endpoint.description << "\",\n";
    oss << "        \"parameters\": [";

    for (size_t j = 0; j < endpoint.parameters.size(); ++j) {
      if (j > 0) oss << ", ";
      oss << "\"" << endpoint.parameters[j] << "\"";
    }

    oss << "],\n";
    oss << "        \"responses\": {\n";
    oss << "          \"200\": { \"description\": \"Success\" }\n";
    oss << "        }\n";
    oss << "      }\n";
    oss << "    }";

    if (i < endpoints.size() - 1) oss << ",";
    oss << "\n";
  }

  oss << "  }\n";
  oss << "}\n";

  return oss.str();
}

std::string APIDocumentationGenerator::generate_markdown(
    const std::vector<APIEndpoint>& endpoints) {
  std::ostringstream oss;

  oss << "# API Documentation\n\n";

  for (const auto& endpoint : endpoints) {
    oss << "## " << Performance::to_string(endpoint.method) << " " << endpoint.path << "\n\n";
    oss << endpoint.description << "\n\n";

    if (!endpoint.parameters.empty()) {
      oss << "**Parameters:**\n";
      for (const auto& param : endpoint.parameters) {
        oss << "- " << param << "\n";
      }
      oss << "\n";
    }

    if (endpoint.requires_auth) {
      oss << "**Authentication:** Required\n\n";
    }

    oss << "---\n\n";
  }

  return oss.str();
}

std::string APIDocumentationGenerator::generate_html(
    const std::vector<APIEndpoint>& endpoints) {
  std::ostringstream oss;

  oss << "<!DOCTYPE html>\n";
  oss << "<html>\n<head>\n";
  oss << "<title>API Documentation</title>\n";
  oss << "<style>body { font-family: Arial, sans-serif; margin: 20px; }</style>\n";
  oss << "</head>\n<body>\n";
  oss << "<h1>API Documentation</h1>\n";

  for (const auto& endpoint : endpoints) {
    oss << "<div class='endpoint'>\n";
    oss << "<h2>" << Performance::to_string(endpoint.method) << " " << endpoint.path << "</h2>\n";
    oss << "<p>" << endpoint.description << "</p>\n";

    if (!endpoint.parameters.empty()) {
      oss << "<h3>Parameters:</h3>\n<ul>\n";
      for (const auto& param : endpoint.parameters) {
        oss << "<li>" << param << "</li>\n";
      }
      oss << "</ul>\n";
    }

    oss << "</div>\n<hr>\n";
  }

  oss << "</body>\n</html>\n";

  return oss.str();
}

// APIValidator implementation
bool APIValidator::validate_request(
    const Performance::HttpRequest& request,
    const APIEndpoint& endpoint,
    std::string* error) {
  // Validate parameters
  if (!validate_parameters(request.query_params, endpoint.parameters)) {
    if (error) *error = "Missing required parameters";
    return false;
  }

  return true;
}

bool APIValidator::validate_version(
    const APIVersion& requested,
    const APIVersion& supported) {
  return requested.major == supported.major;
}

bool APIValidator::validate_parameters(
    const std::map<std::string, std::string>& params,
    const std::vector<std::string>& required) {
  for (const auto& req : required) {
    if (params.find(req) == params.end()) {
      return false;
    }
  }
  return true;
}

}  // namespace SolarSystem::API
