#include "solar_test/reporters/console_reporter.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace SolarSystem::Testing {

ConsoleReporter::ConsoleReporter(Configuration config) : config_(std::move(config)) {}

ConsoleReporter::ConsoleReporter() : config_(Configuration{}) {}

void ConsoleReporter::on_suite_started(const std::string& suite_name, size_t total_tests) {
  if (quiet_) return;

  suite_start_time_ = std::chrono::steady_clock::now();
  total_tests_ = total_tests;
  current_test_index_ = 0;

  print_header("Running Test Suite: " + suite_name);
  *config_.output_stream << colorize("Total tests: ", Colors::CYAN) << total_tests << "\n\n";
}

void ConsoleReporter::on_suite_finished(const TestSuiteResult& result) {
  if (quiet_) return;

  auto suite_end_time = std::chrono::steady_clock::now();
  auto total_duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(suite_end_time - suite_start_time_);

  print_separator('=');
  print_header("Test Suite Results");
  print_summary_statistics(result);

  *config_.output_stream << "\n" << colorize("Total execution time: ", Colors::CYAN);
  print_execution_time(total_duration);
  *config_.output_stream << "\n";

  // Print overall result
  if (result.all_passed()) {
    *config_.output_stream << "\n"
                           << colorize("✓ ALL TESTS PASSED", Colors::GREEN)
                           << colorize(" (" + std::to_string(result.passed_count) + "/" +
                                           std::to_string(result.test_results.size()) + ")",
                                       Colors::BOLD)
                           << "\n\n";
  } else {
    *config_.output_stream << "\n"
                           << colorize("✗ SOME TESTS FAILED", Colors::RED)
                           << colorize(" (" + std::to_string(result.failed_count) + " failed, " +
                                           std::to_string(result.passed_count) + " passed)",
                                       Colors::BOLD)
                           << "\n\n";
  }
}

void ConsoleReporter::on_test_started(const std::string& test_name) {
  if (quiet_) return;

  test_start_time_ = std::chrono::steady_clock::now();
  current_test_index_++;

  if (verbose_) {
    *config_.output_stream << colorize("[" + std::to_string(current_test_index_) + "/" +
                                           std::to_string(total_tests_) + "] ",
                                       Colors::DIM)
                           << colorize("Running: ", Colors::BLUE) << test_name;

    if (config_.show_progress) {
      *config_.output_stream << " ";
      show_spinner();
    }
    *config_.output_stream << "\n";
  } else if (config_.show_progress) {
    double percentage =
        static_cast<double>(current_test_index_) / static_cast<double>(total_tests_) * 100.0;
    print_progress_bar(percentage);
  }
}

void ConsoleReporter::on_test_finished(const TestResult& result) {
  if (quiet_ && result.passed()) return;

  auto test_end_time = std::chrono::steady_clock::now();
  auto test_duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(test_end_time - test_start_time_);

  if (verbose_ || !result.passed()) {
    print_test_status(result);

    if (config_.show_timing) {
      *config_.output_stream << " " << colorize("(", Colors::DIM);
      print_execution_time(test_duration);
      *config_.output_stream << colorize(")", Colors::DIM);
    }

    if (config_.show_memory_usage && result.memory_usage_bytes > 0) {
      *config_.output_stream << " " << colorize("[", Colors::DIM);
      print_memory_usage(result.memory_usage_bytes);
      *config_.output_stream << colorize("]", Colors::DIM);
    }

    *config_.output_stream << "\n";

    if (!result.passed() && config_.show_stack_traces) {
      print_failure_details(result);
    }
  }
}

void ConsoleReporter::on_progress(const std::string& message, double percentage) {
  if (quiet_) return;

  if (verbose_) {
    *config_.output_stream << colorize("Progress: ", Colors::CYAN) << message << " (" << std::fixed
                           << std::setprecision(1) << percentage << "%)\n";
  } else if (config_.show_progress) {
    print_progress_bar(percentage);
  }
}

void ConsoleReporter::on_error(const std::string& error_message) {
  *config_.error_stream << colorize("ERROR: ", Colors::RED) << error_message << "\n";
}

void ConsoleReporter::print_header(const std::string& title) {
  print_separator('=');
  *config_.output_stream << colorize(title, Colors::BOLD) << "\n";
  print_separator('=');
}

void ConsoleReporter::print_separator(char character, size_t length) {
  *config_.output_stream << std::string(length, character) << "\n";
}

void ConsoleReporter::print_progress_bar(double percentage, size_t width) {
  if (!config_.show_progress) return;

  clear_line();
  *config_.output_stream << "\r" << colorize("Progress: [", Colors::CYAN);

  size_t filled = static_cast<size_t>(percentage / 100.0 * static_cast<double>(width));
  size_t empty = width - filled;

  *config_.output_stream << colorize(std::string(filled, '#'), Colors::GREEN)
                         << std::string(empty, '-') << colorize("] ", Colors::CYAN) << std::fixed
                         << std::setprecision(1) << percentage << "%";

  config_.output_stream->flush();
}

void ConsoleReporter::print_test_status(const TestResult& result) {
  std::string status_symbol;
  std::string status_text;

  switch (result.status) {
    case TestResult::Status::Passed:
      status_symbol = "✓";
      status_text = "PASSED";
      break;
    case TestResult::Status::Failed:
      status_symbol = "✗";
      status_text = "FAILED";
      break;
    case TestResult::Status::Skipped:
      status_symbol = "⊝";
      status_text = "SKIPPED";
      break;
    case TestResult::Status::Timeout:
      status_symbol = "⏱";
      status_text = "TIMEOUT";
      break;
    case TestResult::Status::Error:
      status_symbol = "⚠";
      status_text = "ERROR";
      break;
    case TestResult::Status::ExpectedFailure:
      status_symbol = "⚠";
      status_text = "EXPECTED FAILURE";
      break;
  }

  *config_.output_stream << colorize(status_symbol + " " + status_text, status_color(result.status))
                         << " " << colorize(result.test_name, Colors::BOLD);
}

void ConsoleReporter::print_failure_details(const TestResult& result) {
  if (result.passed()) return;

  *config_.output_stream << colorize("  Error: ", Colors::RED) << result.error_message << "\n";

  if (!result.assertion_failures.empty()) {
    *config_.output_stream << colorize("  Assertion failures:", Colors::YELLOW) << "\n";
    print_stack_trace(result.assertion_failures);
  }

  if (!result.metadata.empty()) {
    *config_.output_stream << colorize("  Additional information:", Colors::CYAN) << "\n";
    for (const auto& [key, value] : result.metadata) {
      *config_.output_stream << colorize("    " + key + ": ", Colors::DIM) << value << "\n";
    }
  }
  *config_.output_stream << "\n";
}

void ConsoleReporter::print_summary_statistics(const TestSuiteResult& result) {
  *config_.output_stream << colorize("Passed:  ", Colors::GREEN) << result.passed_count << "\n"
                         << colorize("Failed:  ", Colors::RED) << result.failed_count << "\n"
                         << colorize("Skipped: ", Colors::YELLOW) << result.skipped_count << "\n"
                         << colorize("Total:   ", Colors::BOLD) << result.test_results.size()
                         << "\n";

  if (result.code_coverage_percentage > 0.0) {
    *config_.output_stream << colorize("Coverage: ", Colors::CYAN) << std::fixed
                           << std::setprecision(1) << result.code_coverage_percentage << "%\n";
  }

  double success_rate = result.success_rate();
  *config_.output_stream << colorize("Success rate: ", Colors::CYAN) << std::fixed
                         << std::setprecision(1) << success_rate << "%\n";
}

void ConsoleReporter::print_execution_time(std::chrono::milliseconds duration) {
  *config_.output_stream << format_duration(duration);
}

void ConsoleReporter::print_memory_usage(size_t bytes) {
  *config_.output_stream << format_memory(bytes);
}

std::string ConsoleReporter::colorize(const std::string& text, const char* color) const {
  if (!config_.colorized) {
    return text;
  }
  return std::string(color) + text + Colors::RESET;
}

const char* ConsoleReporter::status_color(TestResult::Status status) const {
  switch (status) {
    case TestResult::Status::Passed:
      return Colors::GREEN;
    case TestResult::Status::Failed:
    case TestResult::Status::Error:
      return Colors::RED;
    case TestResult::Status::Skipped:
    case TestResult::Status::ExpectedFailure:
      return Colors::YELLOW;
    case TestResult::Status::Timeout:
      return Colors::MAGENTA;
    default:
      return Colors::WHITE;
  }
}

std::string ConsoleReporter::format_duration(std::chrono::milliseconds duration) const {
  auto ms = duration.count();

  if (ms < 1000) {
    return std::to_string(ms) + "ms";
  } else if (ms < 60000) {
    return std::to_string(static_cast<double>(ms) / 1000.0) + "s";
  } else {
    auto minutes = ms / 60000;
    auto seconds = static_cast<double>(ms % 60000) / 1000.0;
    return std::to_string(minutes) + "m " + std::to_string(seconds) + "s";
  }
}

std::string ConsoleReporter::format_memory(size_t bytes) const {
  const char* units[] = {"B", "KB", "MB", "GB"};
  size_t unit_index = 0;
  double size = static_cast<double>(bytes);

  while (size >= 1024.0 && unit_index < 3) {
    size /= 1024.0;
    unit_index++;
  }

  std::ostringstream oss;
  oss << std::fixed << std::setprecision(1) << size << units[unit_index];
  return oss.str();
}

void ConsoleReporter::show_spinner() {
  static const char spinner_chars[] = {'|', '/', '-', '\\'};
  static size_t spinner_index = 0;

  *config_.output_stream << spinner_chars[spinner_index];
  spinner_index = (spinner_index + 1) % 4;
}

void ConsoleReporter::clear_line() { *config_.output_stream << "\r\033[K"; }

void ConsoleReporter::move_cursor_up(size_t lines) {
  *config_.output_stream << "\033[" << lines << "A";
}

void ConsoleReporter::print_stack_trace(const std::vector<std::string>& assertion_failures) {
  for (const auto& failure : assertion_failures) {
    *config_.output_stream << colorize("    • ", Colors::RED) << format_assertion_failure(failure)
                           << "\n";
  }
}

std::string ConsoleReporter::format_assertion_failure(const std::string& failure) const {
  // Simple formatting - could be enhanced with more sophisticated parsing
  return failure;
}

}  // namespace SolarSystem::Testing
