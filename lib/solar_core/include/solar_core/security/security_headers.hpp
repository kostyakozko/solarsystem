/**
 * @file security_headers.hpp
 * @brief HTTP security headers management
 *
 * Provides security headers for:
 * - Content Security Policy (CSP)
 * - HTTP Strict Transport Security (HSTS)
 * - X-Frame-Options
 * - X-Content-Type-Options
 * - X-XSS-Protection
 * - Referrer-Policy
 */

#pragma once

#include <map>
#include <string>
#include <vector>

#include "solar_core/export.hpp"

namespace SolarSystem::Security {

/**
 * @brief Security headers configuration
 */
struct SecurityHeadersConfig {
  bool enable_csp = true;
  bool enable_hsts = true;
  bool enable_frame_options = true;
  bool enable_content_type_options = true;
  bool enable_xss_protection = true;
  bool enable_referrer_policy = true;

  // CSP directives
  std::vector<std::string> csp_default_src = {"'self'"};
  std::vector<std::string> csp_script_src = {"'self'", "'unsafe-inline'"};
  std::vector<std::string> csp_style_src = {"'self'", "'unsafe-inline'"};
  std::vector<std::string> csp_img_src = {"'self'", "data:"};

  // HSTS settings
  int hsts_max_age = 31536000;  // 1 year
  bool hsts_include_subdomains = true;
  bool hsts_preload = false;

  // Frame options
  std::string frame_options = "DENY";  // DENY, SAMEORIGIN, or ALLOW-FROM

  // Referrer policy
  std::string referrer_policy = "strict-origin-when-cross-origin";
};

/**
 * @brief Security headers manager
 */
class SOLAR_CORE_API SecurityHeaders {
 public:
  /**
   * @brief Construct with configuration
   */
  explicit SecurityHeaders(SecurityHeadersConfig config = {});

  /**
   * @brief Get all security headers
   */
  [[nodiscard]] std::map<std::string, std::string> get_headers() const;

  /**
   * @brief Get Content-Security-Policy header
   */
  [[nodiscard]] std::string get_csp_header() const;

  /**
   * @brief Get Strict-Transport-Security header
   */
  [[nodiscard]] std::string get_hsts_header() const;

  /**
   * @brief Get X-Frame-Options header
   */
  [[nodiscard]] std::string get_frame_options_header() const;

  /**
   * @brief Get X-Content-Type-Options header
   */
  [[nodiscard]] std::string get_content_type_options_header() const;

  /**
   * @brief Get X-XSS-Protection header
   */
  [[nodiscard]] std::string get_xss_protection_header() const;

  /**
   * @brief Get Referrer-Policy header
   */
  [[nodiscard]] std::string get_referrer_policy_header() const;

  /**
   * @brief Update configuration
   */
  void update_config(const SecurityHeadersConfig& config);

 private:
  SecurityHeadersConfig config_;
};

/**
 * @brief CORS (Cross-Origin Resource Sharing) manager
 */
class CORSManager {
 public:
  /**
   * @brief CORS configuration
   */
  struct Config {
    bool enabled = true;
    std::vector<std::string> allowed_origins = {"*"};
    std::vector<std::string> allowed_methods = {"GET", "POST", "PUT", "DELETE", "OPTIONS"};
    std::vector<std::string> allowed_headers = {"Content-Type", "Authorization"};
    std::vector<std::string> exposed_headers;
    bool allow_credentials = false;
    int max_age = 86400;  // 24 hours
  };

  /**
   * @brief Construct with configuration
   */
  explicit CORSManager(Config config);

  /**
   * @brief Get CORS headers for request
   */
  [[nodiscard]] std::map<std::string, std::string> get_cors_headers(
      const std::string& origin, const std::string& method) const;

  /**
   * @brief Check if origin is allowed
   */
  [[nodiscard]] bool is_origin_allowed(const std::string& origin) const;

  /**
   * @brief Check if method is allowed
   */
  [[nodiscard]] bool is_method_allowed(const std::string& method) const;

 private:
  Config config_;
};

}  // namespace SolarSystem::Security
