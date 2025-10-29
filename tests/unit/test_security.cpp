/**
 * @file test_security.cpp
 * @brief Unit tests for security hardening system (Task 13)
 */

#include "../utils/test_framework.h"

#include "solar_core/security/input_sanitizer.hpp"
#include "solar_core/security/rate_limiter.hpp"
#include "solar_core/security/security_headers.hpp"
#include "solar_core/security/session_manager.hpp"

using namespace SolarSystem::Security;

int main() {
  TestSuite suite("Security System Tests");

  suite.run_test("Input Sanitizer - HTML Escaping", []() {
    InputSanitizer sanitizer;

    auto result = sanitizer.sanitize_html("<script>alert('xss')</script>");
    if (result.find("<script>") != std::string::npos)
      throw std::runtime_error("Script tags should be escaped");
  });

  suite.run_test("Input Sanitizer - SQL Injection Prevention", []() {
    InputSanitizer sanitizer;

    auto result = sanitizer.sanitize_sql("'; DROP TABLE users; --");
    // Result should be sanitized (escaped or modified)
    if (result == "'; DROP TABLE users; --")
      throw std::runtime_error("SQL should be sanitized");
  });

  suite.run_test("Input Sanitizer - Path Traversal Prevention", []() {
    InputSanitizer sanitizer;

    auto result = sanitizer.sanitize_path("../../etc/passwd");
    if (result.find("..") != std::string::npos)
      throw std::runtime_error("Path traversal should be prevented");
  });

  suite.run_test("Rate Limiter - Basic Limiting", []() {
    RateLimitConfig config;
    config.max_requests = 5;
    config.window = std::chrono::seconds(1);

    RateLimiter limiter(config);

    // First 5 requests should succeed
    for (int i = 0; i < 5; i++) {
      auto result = limiter.check_limit("client1");
      if (!result.allowed) throw std::runtime_error("Request should be allowed");
    }

    // 6th request should be blocked
    auto result = limiter.check_limit("client1");
    if (result.allowed) throw std::runtime_error("Request should be rate limited");
  });

  suite.run_test("Rate Limiter - Multiple Clients", []() {
    RateLimitConfig config;
    config.max_requests = 2;
    config.window = std::chrono::seconds(1);

    RateLimiter limiter(config);

    auto r1 = limiter.check_limit("client1");
    if (!r1.allowed) throw std::runtime_error("Client1 should be allowed");
    auto r2 = limiter.check_limit("client2");
    if (!r2.allowed) throw std::runtime_error("Client2 should be allowed");
    auto r3 = limiter.check_limit("client1");
    if (!r3.allowed) throw std::runtime_error("Client1 should be allowed");
    auto r4 = limiter.check_limit("client2");
    if (!r4.allowed) throw std::runtime_error("Client2 should be allowed");

    // Both should now be limited
    auto r5 = limiter.check_limit("client1");
    if (r5.allowed) throw std::runtime_error("Client1 should be limited");
    auto r6 = limiter.check_limit("client2");
    if (r6.allowed) throw std::runtime_error("Client2 should be limited");
  });

  suite.run_test("Security Headers - Initialization", []() {
    SecurityHeadersConfig config;
    config.enable_hsts = true;

    SecurityHeaders headers(config);
    auto header_map = headers.get_headers();

    // Headers should be generated
    if (header_map.empty()) throw std::runtime_error("Headers should not be empty");
  });

  suite.run_test("Session Manager - Create and Validate", []() {
    SessionManager manager;

    auto session_id = manager.create_session("user123");
    if (session_id.empty()) throw std::runtime_error("Session ID should not be empty");

    if (!manager.validate_session(session_id))
      throw std::runtime_error("Session should be valid");

    auto session = manager.get_session(session_id);
    if (!session.has_value()) throw std::runtime_error("Session should exist");
    if (session->user_id != "user123") throw std::runtime_error("Wrong user ID");
  });

  suite.run_test("Session Manager - Destroy Session", []() {
    SessionManager manager;

    auto session_id = manager.create_session("user456");
    manager.destroy_session(session_id);

    if (manager.validate_session(session_id))
      throw std::runtime_error("Destroyed session should be invalid");
  });

  suite.print_summary();
  return suite.all_passed() ? 0 : 1;
}
