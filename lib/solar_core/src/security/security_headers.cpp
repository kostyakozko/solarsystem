/**
 * @file security_headers.cpp
 * @brief Implementation of security headers
 */

#include "solar_core/security/security_headers.hpp"

#include <algorithm>
#include <sstream>

namespace SolarSystem::Security {

SecurityHeaders::SecurityHeaders(SecurityHeadersConfig config)
    : config_(std::move(config)) {}

std::map<std::string, std::string> SecurityHeaders::get_headers() const {
  std::map<std::string, std::string> headers;

  if (config_.enable_csp) {
    headers["Content-Security-Policy"] = get_csp_header();
  }

  if (config_.enable_hsts) {
    headers["Strict-Transport-Security"] = get_hsts_header();
  }

  if (config_.enable_frame_options) {
    headers["X-Frame-Options"] = get_frame_options_header();
  }

  if (config_.enable_content_type_options) {
    headers["X-Content-Type-Options"] = get_content_type_options_header();
  }

  if (config_.enable_xss_protection) {
    headers["X-XSS-Protection"] = get_xss_protection_header();
  }

  if (config_.enable_referrer_policy) {
    headers["Referrer-Policy"] = get_referrer_policy_header();
  }

  return headers;
}

std::string SecurityHeaders::get_csp_header() const {
  std::ostringstream oss;

  oss << "default-src";
  for (const auto& src : config_.csp_default_src) {
    oss << " " << src;
  }
  oss << "; ";

  oss << "script-src";
  for (const auto& src : config_.csp_script_src) {
    oss << " " << src;
  }
  oss << "; ";

  oss << "style-src";
  for (const auto& src : config_.csp_style_src) {
    oss << " " << src;
  }
  oss << "; ";

  oss << "img-src";
  for (const auto& src : config_.csp_img_src) {
    oss << " " << src;
  }

  return oss.str();
}

std::string SecurityHeaders::get_hsts_header() const {
  std::ostringstream oss;
  oss << "max-age=" << config_.hsts_max_age;

  if (config_.hsts_include_subdomains) {
    oss << "; includeSubDomains";
  }

  if (config_.hsts_preload) {
    oss << "; preload";
  }

  return oss.str();
}

std::string SecurityHeaders::get_frame_options_header() const {
  return config_.frame_options;
}

std::string SecurityHeaders::get_content_type_options_header() const {
  return "nosniff";
}

std::string SecurityHeaders::get_xss_protection_header() const {
  return "1; mode=block";
}

std::string SecurityHeaders::get_referrer_policy_header() const {
  return config_.referrer_policy;
}

void SecurityHeaders::update_config(const SecurityHeadersConfig& config) {
  config_ = config;
}

// CORSManager implementation
CORSManager::CORSManager(Config config) : config_(std::move(config)) {}

std::map<std::string, std::string> CORSManager::get_cors_headers(
    const std::string& origin,
    const std::string& method) const {
  std::map<std::string, std::string> headers;

  if (!config_.enabled) {
    return headers;
  }

  if (is_origin_allowed(origin)) {
    headers["Access-Control-Allow-Origin"] = origin;
  }

  if (is_method_allowed(method)) {
    std::ostringstream methods;
    for (size_t i = 0; i < config_.allowed_methods.size(); ++i) {
      if (i > 0) methods << ", ";
      methods << config_.allowed_methods[i];
    }
    headers["Access-Control-Allow-Methods"] = methods.str();
  }

  if (!config_.allowed_headers.empty()) {
    std::ostringstream headers_str;
    for (size_t i = 0; i < config_.allowed_headers.size(); ++i) {
      if (i > 0) headers_str << ", ";
      headers_str << config_.allowed_headers[i];
    }
    headers["Access-Control-Allow-Headers"] = headers_str.str();
  }

  if (config_.allow_credentials) {
    headers["Access-Control-Allow-Credentials"] = "true";
  }

  headers["Access-Control-Max-Age"] = std::to_string(config_.max_age);

  return headers;
}

bool CORSManager::is_origin_allowed(const std::string& origin) const {
  if (config_.allowed_origins.empty()) {
    return false;
  }

  if (std::find(config_.allowed_origins.begin(), config_.allowed_origins.end(), "*") !=
      config_.allowed_origins.end()) {
    return true;
  }

  return std::find(config_.allowed_origins.begin(), config_.allowed_origins.end(), origin) !=
         config_.allowed_origins.end();
}

bool CORSManager::is_method_allowed(const std::string& method) const {
  return std::find(config_.allowed_methods.begin(), config_.allowed_methods.end(), method) !=
         config_.allowed_methods.end();
}

}  // namespace SolarSystem::Security
