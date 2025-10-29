/**
 * @file protocol.cpp
 * @brief Implementation of communication protocols
 */

#include "solar_core/communication/protocol.hpp"

#include <condition_variable>
#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>

namespace SolarSystem::Communication {

// InProcessProtocol implementation
struct InProcessProtocol::Impl {
  std::deque<Message> message_queue;
  std::mutex queue_mutex;
  std::condition_variable queue_cv;
  bool initialized = false;
  ProtocolStats stats;
};

InProcessProtocol::InProcessProtocol() : impl_(std::make_unique<Impl>()) {}

InProcessProtocol::~InProcessProtocol() { shutdown(); }

SolarSystem::Utils::Expected<void, ProtocolError> InProcessProtocol::initialize() {
  impl_->initialized = true;
  return SolarSystem::Utils::Expected<void, ProtocolError>();
}

void InProcessProtocol::shutdown() { impl_->initialized = false; }

SolarSystem::Utils::Expected<void, ProtocolError> InProcessProtocol::send(const Message& message) {
  if (!impl_->initialized) {
    return ProtocolError::CONNECTION_FAILED;
  }

  std::lock_guard<std::mutex> lock(impl_->queue_mutex);
  impl_->message_queue.push_back(message);
  impl_->stats.messages_sent++;
  impl_->stats.last_activity = std::chrono::system_clock::now();
  impl_->queue_cv.notify_one();

  return SolarSystem::Utils::Expected<void, ProtocolError>();
}

SolarSystem::Utils::Expected<Message, ProtocolError> InProcessProtocol::receive(
    std::chrono::milliseconds timeout) {
  if (!impl_->initialized) {
    return ProtocolError::CONNECTION_FAILED;
  }

  std::unique_lock<std::mutex> lock(impl_->queue_mutex);

  if (!impl_->queue_cv.wait_for(lock, timeout,
                                [this] { return !impl_->message_queue.empty(); })) {
    return ProtocolError::TIMEOUT;
  }

  Message msg = impl_->message_queue.front();
  impl_->message_queue.pop_front();
  impl_->stats.messages_received++;
  impl_->stats.last_activity = std::chrono::system_clock::now();

  return msg;
}

SolarSystem::Utils::Expected<Message, ProtocolError> InProcessProtocol::request(
    const Message& request, std::chrono::milliseconds timeout) {
  auto send_result = send(request);
  if (!send_result.has_value()) {
    return send_result.error();
  }

  // Wait for response with matching correlation ID
  auto start = std::chrono::steady_clock::now();
  while (true) {
    auto remaining = timeout - std::chrono::duration_cast<std::chrono::milliseconds>(
                                  std::chrono::steady_clock::now() - start);
    if (remaining <= std::chrono::milliseconds(0)) {
      return ProtocolError::TIMEOUT;
    }

    auto receive_result = receive(remaining);
    if (!receive_result.has_value()) {
      return receive_result.error();
    }

    const auto& response = receive_result.value();
    if (response.header.correlation_id && *response.header.correlation_id == request.header.message_id) {
      return response;
    }

    // Not our response, put it back (simplified - in real impl would use better queue)
    std::lock_guard<std::mutex> lock(impl_->queue_mutex);
    impl_->message_queue.push_front(response);
  }
}

bool InProcessProtocol::is_ready() const { return impl_->initialized; }

ProtocolStats InProcessProtocol::get_stats() const { return impl_->stats; }

// FileProtocol implementation
struct FileProtocol::Impl {
  std::string directory;
  bool initialized = false;
  ProtocolStats stats;
  std::string inbox_dir;
  std::string outbox_dir;
};

FileProtocol::FileProtocol(const std::string& directory) : impl_(std::make_unique<Impl>()) {
  impl_->directory = directory;
  impl_->inbox_dir = directory + "/inbox";
  impl_->outbox_dir = directory + "/outbox";
}

FileProtocol::~FileProtocol() { shutdown(); }

SolarSystem::Utils::Expected<void, ProtocolError> FileProtocol::initialize() {
  try {
    std::filesystem::create_directories(impl_->inbox_dir);
    std::filesystem::create_directories(impl_->outbox_dir);
    impl_->initialized = true;
    return SolarSystem::Utils::Expected<void, ProtocolError>();
  } catch (...) {
    return ProtocolError::CONNECTION_FAILED;
  }
}

void FileProtocol::shutdown() { impl_->initialized = false; }

SolarSystem::Utils::Expected<void, ProtocolError> FileProtocol::send(const Message& message) {
  if (!impl_->initialized) {
    return ProtocolError::CONNECTION_FAILED;
  }

  try {
    std::string filename = impl_->outbox_dir + "/" + message.header.message_id + ".msg";
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
      return ProtocolError::SEND_FAILED;
    }

    // Simplified: write message ID
    file << message.header.message_id;
    impl_->stats.messages_sent++;
    impl_->stats.last_activity = std::chrono::system_clock::now();

    return SolarSystem::Utils::Expected<void, ProtocolError>();
  } catch (...) {
    return ProtocolError::SEND_FAILED;
  }
}

SolarSystem::Utils::Expected<Message, ProtocolError> FileProtocol::receive(
    std::chrono::milliseconds timeout) {
  if (!impl_->initialized) {
    return ProtocolError::CONNECTION_FAILED;
  }

  auto start = std::chrono::steady_clock::now();

  while (true) {
    try {
      for (const auto& entry : std::filesystem::directory_iterator(impl_->inbox_dir)) {
        if (entry.is_regular_file()) {
          // Found a message file
          Message msg;
          msg.header.message_id = entry.path().stem().string();
          msg.header.type = MessageType::NOTIFICATION;
          msg.header.source_application = "file";
          msg.header.timestamp = std::chrono::system_clock::now();

          // Delete the file
          std::filesystem::remove(entry.path());

          impl_->stats.messages_received++;
          impl_->stats.last_activity = std::chrono::system_clock::now();

          return msg;
        }
      }
    } catch (...) {
      return ProtocolError::RECEIVE_FAILED;
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    if (elapsed >= timeout) {
      return ProtocolError::TIMEOUT;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}

SolarSystem::Utils::Expected<Message, ProtocolError> FileProtocol::request(
    const Message& request, std::chrono::milliseconds timeout) {
  auto send_result = send(request);
  if (!send_result.has_value()) {
    return send_result.error();
  }

  return receive(timeout);
}

bool FileProtocol::is_ready() const { return impl_->initialized; }

ProtocolStats FileProtocol::get_stats() const { return impl_->stats; }

// RetryProtocol implementation
struct RetryProtocol::Impl {
  std::shared_ptr<IProtocol> underlying;
  RetryConfig config;
  ProtocolStats stats;
};

RetryProtocol::RetryProtocol(std::shared_ptr<IProtocol> underlying_protocol, RetryConfig config)
    : impl_(std::make_unique<Impl>()) {
  impl_->underlying = underlying_protocol;
  impl_->config = config;
}

RetryProtocol::~RetryProtocol() { shutdown(); }

SolarSystem::Utils::Expected<void, ProtocolError> RetryProtocol::initialize() {
  return impl_->underlying->initialize();
}

void RetryProtocol::shutdown() { impl_->underlying->shutdown(); }

SolarSystem::Utils::Expected<void, ProtocolError> RetryProtocol::send(const Message& message) {
  size_t attempt = 0;
  std::chrono::milliseconds delay = impl_->config.initial_delay;

  while (attempt < impl_->config.max_retries) {
    auto result = impl_->underlying->send(message);
    if (result.has_value()) {
      impl_->stats.messages_sent++;
      return result;
    }

    attempt++;
    if (attempt < impl_->config.max_retries) {
      std::this_thread::sleep_for(delay);
      delay = std::min(
          std::chrono::milliseconds(static_cast<int64_t>(delay.count() * impl_->config.backoff_multiplier)),
          impl_->config.max_delay);
    }
  }

  impl_->stats.messages_failed++;
  return ProtocolError::SEND_FAILED;
}

SolarSystem::Utils::Expected<Message, ProtocolError> RetryProtocol::receive(
    std::chrono::milliseconds timeout) {
  return impl_->underlying->receive(timeout);
}

SolarSystem::Utils::Expected<Message, ProtocolError> RetryProtocol::request(
    const Message& request, std::chrono::milliseconds timeout) {
  size_t attempt = 0;
  std::chrono::milliseconds delay = impl_->config.initial_delay;

  while (attempt < impl_->config.max_retries) {
    auto result = impl_->underlying->request(request, timeout);
    if (result.has_value()) {
      return result;
    }

    attempt++;
    if (attempt < impl_->config.max_retries) {
      std::this_thread::sleep_for(delay);
      delay = std::min(
          std::chrono::milliseconds(static_cast<int64_t>(delay.count() * impl_->config.backoff_multiplier)),
          impl_->config.max_delay);
    }
  }

  return ProtocolError::TIMEOUT;
}

bool RetryProtocol::is_ready() const { return impl_->underlying->is_ready(); }

ProtocolStats RetryProtocol::get_stats() const {
  auto stats = impl_->underlying->get_stats();
  stats.messages_sent += impl_->stats.messages_sent;
  stats.messages_failed += impl_->stats.messages_failed;
  return stats;
}

std::string RetryProtocol::get_name() const {
  return "Retry(" + impl_->underlying->get_name() + ")";
}

// ProtocolMonitor implementation
struct ProtocolMonitor::Impl {
  std::shared_ptr<IProtocol> protocol;
  ProtocolMonitor::MessageCallback message_callback;
  ProtocolMonitor::ErrorCallback error_callback;
  std::deque<Message> message_history;
  std::deque<std::pair<ProtocolError, std::string>> error_history;
  std::mutex history_mutex;
  static constexpr size_t MAX_HISTORY = 1000;
};

ProtocolMonitor::ProtocolMonitor(std::shared_ptr<IProtocol> protocol)
    : impl_(std::make_unique<Impl>()) {
  impl_->protocol = protocol;
}

void ProtocolMonitor::set_message_callback(MessageCallback callback) {
  impl_->message_callback = callback;
}

void ProtocolMonitor::set_error_callback(ErrorCallback callback) {
  impl_->error_callback = callback;
}

std::shared_ptr<IProtocol> ProtocolMonitor::get_protocol() const { return impl_->protocol; }

std::vector<Message> ProtocolMonitor::get_message_history(size_t max_count) const {
  std::lock_guard<std::mutex> lock(impl_->history_mutex);
  size_t count = std::min(max_count, impl_->message_history.size());
  return std::vector<Message>(impl_->message_history.begin(),
                             impl_->message_history.begin() + static_cast<long>(count));
}

std::vector<std::pair<ProtocolError, std::string>> ProtocolMonitor::get_error_history(
    size_t max_count) const {
  std::lock_guard<std::mutex> lock(impl_->history_mutex);
  size_t count = std::min(max_count, impl_->error_history.size());
  return std::vector<std::pair<ProtocolError, std::string>>(
      impl_->error_history.begin(), impl_->error_history.begin() + static_cast<long>(count));
}

void ProtocolMonitor::clear_history() {
  std::lock_guard<std::mutex> lock(impl_->history_mutex);
  impl_->message_history.clear();
  impl_->error_history.clear();
}

}  // namespace SolarSystem::Communication
