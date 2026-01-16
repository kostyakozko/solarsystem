/**
 * @file request_handler.cpp
 * @brief Implementation of HTTP request handling
 */

#include "solar_core/performance/request_handler.hpp"

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>

namespace SolarSystem::Performance {

std::string to_string(HttpMethod method) {
  switch (method) {
    case HttpMethod::GET:
      return "GET";
    case HttpMethod::POST:
      return "POST";
    case HttpMethod::PUT:
      return "PUT";
    case HttpMethod::DELETE:
      return "DELETE";
    case HttpMethod::PATCH:
      return "PATCH";
    case HttpMethod::HEAD:
      return "HEAD";
    case HttpMethod::OPTIONS:
      return "OPTIONS";
    default:
      return "UNKNOWN";
  }
}

std::optional<HttpMethod> parse_http_method(const std::string& str) {
  if (str == "GET") return HttpMethod::GET;
  if (str == "POST") return HttpMethod::POST;
  if (str == "PUT") return HttpMethod::PUT;
  if (str == "DELETE") return HttpMethod::DELETE;
  if (str == "PATCH") return HttpMethod::PATCH;
  if (str == "HEAD") return HttpMethod::HEAD;
  if (str == "OPTIONS") return HttpMethod::OPTIONS;
  return std::nullopt;
}

// HttpRouter implementation
void HttpRouter::add_route(HttpMethod method, const std::string& path, RequestHandler handler) {
  add_route(method, path, std::move(handler), false);
}

void HttpRouter::add_route(HttpMethod method, const std::string& path, RequestHandler handler,
                           bool requires_auth) {
  Route route;
  route.method = method;
  route.path = path;
  route.handler = std::move(handler);
  route.requires_auth = requires_auth;
  routes_.push_back(std::move(route));
}

std::optional<RequestHandler> HttpRouter::find_handler(HttpMethod method,
                                                       const std::string& path) const {
  for (const auto& route : routes_) {
    if (route.method == method && route.path == path) {
      return route.handler;
    }
  }
  return std::nullopt;
}

bool HttpRouter::requires_auth(HttpMethod method, const std::string& path) const {
  for (const auto& route : routes_) {
    if (route.method == method && route.path == path) {
      return route.requires_auth;
    }
  }
  return false;
}

std::vector<Route> HttpRouter::get_routes() const { return routes_; }

// RequestProcessor implementation
struct RequestProcessor::Impl {
  size_t thread_pool_size;
  std::vector<std::thread> workers;
  std::queue<std::function<void()>> tasks;
  std::mutex queue_mutex;
  std::condition_variable condition;
  std::atomic<bool> stop{false};
  std::atomic<size_t> active_count{0};
  std::atomic<size_t> total_processed{0};

  explicit Impl(size_t size) : thread_pool_size(size) {
    for (size_t i = 0; i < thread_pool_size; ++i) {
      workers.emplace_back([this] {
        while (true) {
          std::function<void()> task;
          {
            std::unique_lock<std::mutex> lock(queue_mutex);
            condition.wait(lock, [this] { return stop.load() || !tasks.empty(); });

            if (stop.load() && tasks.empty()) {
              return;
            }

            task = std::move(tasks.front());
            tasks.pop();
          }

          active_count.fetch_add(1);
          task();
          active_count.fetch_sub(1);
          total_processed.fetch_add(1);
        }
      });
    }
  }

  ~Impl() {
    stop.store(true);
    condition.notify_all();
    for (auto& worker : workers) {
      if (worker.joinable()) {
        worker.join();
      }
    }
  }
};

RequestProcessor::RequestProcessor(size_t thread_pool_size)
    : impl_(std::make_unique<Impl>(thread_pool_size)) {}

RequestProcessor::~RequestProcessor() = default;

RequestProcessor::RequestProcessor(RequestProcessor&&) noexcept = default;
RequestProcessor& RequestProcessor::operator=(RequestProcessor&&) noexcept = default;

void RequestProcessor::process_async(const HttpRequest& request, RequestHandler handler,
                                     std::function<void(const HttpResponse&)> callback) {
  {
    std::lock_guard<std::mutex> lock(impl_->queue_mutex);
    impl_->tasks.emplace([request, handler, callback]() {
      auto response = handler(request);
      callback(response);
    });
  }
  impl_->condition.notify_one();
}

HttpResponse RequestProcessor::process_sync(const HttpRequest& request, RequestHandler handler) {
  return handler(request);
}

size_t RequestProcessor::get_active_count() const { return impl_->active_count.load(); }

size_t RequestProcessor::get_total_processed() const { return impl_->total_processed.load(); }

// HttpRequestParser implementation
std::optional<HttpRequest> HttpRequestParser::parse(const std::string& raw_request) {
  HttpRequest request;

  std::istringstream stream(raw_request);
  std::string line;

  // Parse request line
  if (!std::getline(stream, line)) {
    return std::nullopt;
  }

  std::istringstream request_line(line);
  std::string method_str, path_and_query, version;
  request_line >> method_str >> path_and_query >> version;

  auto method = parse_http_method(method_str);
  if (!method) {
    return std::nullopt;
  }
  request.method = *method;

  // Split path and query string
  auto query_pos = path_and_query.find('?');
  if (query_pos != std::string::npos) {
    request.path = path_and_query.substr(0, query_pos);
    request.query_string = path_and_query.substr(query_pos + 1);
    request.query_params = parse_query_string(request.query_string);
  } else {
    request.path = path_and_query;
  }

  // Parse headers
  std::string header_section;
  while (std::getline(stream, line) && line != "\r" && !line.empty()) {
    header_section += line + "\n";
  }
  request.headers = parse_headers(header_section);

  // Parse body
  std::string body_line;
  while (std::getline(stream, body_line)) {
    request.body += body_line;
  }

  return request;
}

std::map<std::string, std::string> HttpRequestParser::parse_query_string(const std::string& query) {
  std::map<std::string, std::string> params;

  std::istringstream stream(query);
  std::string pair;

  while (std::getline(stream, pair, '&')) {
    auto eq_pos = pair.find('=');
    if (eq_pos != std::string::npos) {
      auto key = pair.substr(0, eq_pos);
      auto value = pair.substr(eq_pos + 1);
      params[key] = value;
    }
  }

  return params;
}

std::map<std::string, std::string> HttpRequestParser::parse_headers(
    const std::string& header_section) {
  std::map<std::string, std::string> headers;

  std::istringstream stream(header_section);
  std::string line;

  while (std::getline(stream, line)) {
    auto colon_pos = line.find(':');
    if (colon_pos != std::string::npos) {
      auto name = line.substr(0, colon_pos);
      auto value = line.substr(colon_pos + 1);

      // Trim whitespace
      value.erase(0, value.find_first_not_of(" \t"));
      value.erase(value.find_last_not_of(" \t\r\n") + 1);

      headers[name] = value;
    }
  }

  return headers;
}

// HttpResponseBuilder implementation
HttpResponseBuilder& HttpResponseBuilder::status(int code, const std::string& message) {
  response_.status_code = code;
  if (!message.empty()) {
    response_.status_message = message;
  } else {
    // Set default message based on code
    switch (code) {
      case 200:
        response_.status_message = "OK";
        break;
      case 201:
        response_.status_message = "Created";
        break;
      case 204:
        response_.status_message = "No Content";
        break;
      case 400:
        response_.status_message = "Bad Request";
        break;
      case 401:
        response_.status_message = "Unauthorized";
        break;
      case 403:
        response_.status_message = "Forbidden";
        break;
      case 404:
        response_.status_message = "Not Found";
        break;
      case 500:
        response_.status_message = "Internal Server Error";
        break;
      default:
        response_.status_message = "Unknown";
        break;
    }
  }
  return *this;
}

HttpResponseBuilder& HttpResponseBuilder::header(const std::string& name,
                                                 const std::string& value) {
  response_.headers[name] = value;
  return *this;
}

HttpResponseBuilder& HttpResponseBuilder::body(const std::string& content) {
  response_.body = content;
  return *this;
}

HttpResponseBuilder& HttpResponseBuilder::json(const std::string& json_content) {
  response_.headers["Content-Type"] = "application/json";
  response_.body = json_content;
  return *this;
}

HttpResponseBuilder& HttpResponseBuilder::html(const std::string& html_content) {
  response_.headers["Content-Type"] = "text/html";
  response_.body = html_content;
  return *this;
}

HttpResponse HttpResponseBuilder::build() const { return response_; }

std::string HttpResponseBuilder::serialize(const HttpResponse& response) {
  std::ostringstream oss;

  // Status line
  oss << "HTTP/1.1 " << response.status_code << " " << response.status_message << "\r\n";

  // Headers
  for (const auto& [name, value] : response.headers) {
    oss << name << ": " << value << "\r\n";
  }

  // Content-Length if not already set
  if (response.headers.find("Content-Length") == response.headers.end()) {
    oss << "Content-Length: " << response.body.length() << "\r\n";
  }

  oss << "\r\n";

  // Body
  oss << response.body;

  return oss.str();
}

}  // namespace SolarSystem::Performance
