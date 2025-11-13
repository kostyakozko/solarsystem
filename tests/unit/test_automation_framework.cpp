/**
 * @file test_automation_framework.cpp
 * @brief Test automation framework (Task 29)
 *
 * Tests automation framework capabilities:
 * - Automated test execution and scheduling
 * - Test result collection and analysis
 * - Test failure notification and alerting
 * - Test maintenance and update automation
 *
 * Requirements: 10.1, 10.2
 */

#include <algorithm>
#include <chrono>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "test_framework.h"

/**
 * @brief Test execution scheduler
 */
class TestScheduler {
 public:
  enum class Schedule { Immediate, Hourly, Daily, Weekly, OnCommit };

  struct ScheduledTest {
    std::string test_name;
    Schedule schedule;
    std::chrono::system_clock::time_point last_run;
    std::chrono::system_clock::time_point next_run;
    bool enabled = true;
  };

  void add_test(const std::string& name, Schedule schedule) {
    ScheduledTest test;
    test.test_name = name;
    test.schedule = schedule;
    test.last_run = std::chrono::system_clock::now();
    test.next_run = calculate_next_run(schedule);
    scheduled_tests_[name] = test;
  }

  std::vector<std::string> get_tests_to_run() {
    std::vector<std::string> tests_to_run;
    auto now = std::chrono::system_clock::now();

    for (auto& [name, test] : scheduled_tests_) {
      if (test.enabled && now >= test.next_run) {
        tests_to_run.push_back(name);
        test.last_run = now;
        test.next_run = calculate_next_run(test.schedule);
      }
    }

    return tests_to_run;
  }

  void disable_test(const std::string& name) {
    if (scheduled_tests_.find(name) != scheduled_tests_.end()) {
      scheduled_tests_[name].enabled = false;
    }
  }

  void enable_test(const std::string& name) {
    if (scheduled_tests_.find(name) != scheduled_tests_.end()) {
      scheduled_tests_[name].enabled = true;
    }
  }

  size_t get_scheduled_test_count() const { return scheduled_tests_.size(); }

 private:
  std::map<std::string, ScheduledTest> scheduled_tests_;

  std::chrono::system_clock::time_point calculate_next_run(Schedule schedule) {
    auto now = std::chrono::system_clock::now();
    switch (schedule) {
      case Schedule::Immediate:
        return now;
      case Schedule::Hourly:
        return now + std::chrono::hours(1);
      case Schedule::Daily:
        return now + std::chrono::hours(24);
      case Schedule::Weekly:
        return now + std::chrono::hours(24 * 7);
      case Schedule::OnCommit:
        return now;  // Triggered externally
    }
    return now;
  }
};

/**
 * @brief Test result collector
 */
class TestResultCollector {
 public:
  struct TestResult {
    std::string test_name;
    bool passed = false;
    std::chrono::milliseconds duration{0};
    std::string error_message;
    std::chrono::system_clock::time_point timestamp;
  };

  void add_result(const TestResult& result) {
    results_.push_back(result);
    if (result.passed) {
      passed_count_++;
    } else {
      failed_count_++;
    }
  }

  std::vector<TestResult> get_all_results() const { return results_; }

  std::vector<TestResult> get_failed_results() const {
    std::vector<TestResult> failed;
    for (const auto& result : results_) {
      if (!result.passed) {
        failed.push_back(result);
      }
    }
    return failed;
  }

  int get_passed_count() const { return passed_count_; }

  int get_failed_count() const { return failed_count_; }

  double get_pass_rate() const {
    int total = passed_count_ + failed_count_;
    return total > 0 ? static_cast<double>(passed_count_) / total * 100.0 : 0.0;
  }

  void clear() {
    results_.clear();
    passed_count_ = 0;
    failed_count_ = 0;
  }

 private:
  std::vector<TestResult> results_;
  int passed_count_ = 0;
  int failed_count_ = 0;
};

/**
 * @brief Test failure notifier
 */
class TestNotifier {
 public:
  enum class NotificationType { Email, Slack, Console, Log };

  struct Notification {
    NotificationType type;
    std::string recipient;
    std::string message;
    std::chrono::system_clock::time_point sent_at;
  };

  void send_notification(NotificationType type, const std::string& recipient,
                        const std::string& message) {
    Notification notification;
    notification.type = type;
    notification.recipient = recipient;
    notification.message = message;
    notification.sent_at = std::chrono::system_clock::now();
    notifications_.push_back(notification);
  }

  void notify_test_failure(const std::string& test_name, const std::string& error) {
    std::string message = "Test failed: " + test_name + "\nError: " + error;
    send_notification(NotificationType::Console, "console", message);
  }

  void notify_test_success(const std::string& test_name) {
    std::string message = "Test passed: " + test_name;
    send_notification(NotificationType::Log, "log", message);
  }

  std::vector<Notification> get_notifications() const { return notifications_; }

  size_t get_notification_count() const { return notifications_.size(); }

  void clear() { notifications_.clear(); }

 private:
  std::vector<Notification> notifications_;
};

/**
 * @brief Test maintenance manager
 */
class TestMaintenanceManager {
 public:
  struct MaintenanceTask {
    std::string task_name;
    std::string description;
    bool completed = false;
    std::chrono::system_clock::time_point due_date;
  };

  void add_maintenance_task(const std::string& name, const std::string& description) {
    MaintenanceTask task;
    task.task_name = name;
    task.description = description;
    task.due_date = std::chrono::system_clock::now() + std::chrono::hours(24 * 7);
    tasks_[name] = task;
  }

  void complete_task(const std::string& name) {
    if (tasks_.find(name) != tasks_.end()) {
      tasks_[name].completed = true;
    }
  }

  std::vector<MaintenanceTask> get_pending_tasks() const {
    std::vector<MaintenanceTask> pending;
    for (const auto& [name, task] : tasks_) {
      if (!task.completed) {
        pending.push_back(task);
      }
    }
    return pending;
  }

  std::vector<MaintenanceTask> get_overdue_tasks() const {
    std::vector<MaintenanceTask> overdue;
    auto now = std::chrono::system_clock::now();
    for (const auto& [name, task] : tasks_) {
      if (!task.completed && now > task.due_date) {
        overdue.push_back(task);
      }
    }
    return overdue;
  }

  size_t get_task_count() const { return tasks_.size(); }

 private:
  std::map<std::string, MaintenanceTask> tasks_;
};

int main() {
  TEST_SUITE("Test Automation Framework Tests");

  // Test 1: Test scheduling
  TEST_CASE("Test Scheduling") {
    TestScheduler scheduler;

    // Test 1.1: Add scheduled tests
    scheduler.add_test("unit_tests", TestScheduler::Schedule::Immediate);
    scheduler.add_test("integration_tests", TestScheduler::Schedule::Daily);
    scheduler.add_test("performance_tests", TestScheduler::Schedule::Weekly);

    ASSERT_EQ(scheduler.get_scheduled_test_count(), 3);

    // Test 1.2: Get tests to run
    auto tests_to_run = scheduler.get_tests_to_run();
    ASSERT_GE(tests_to_run.size(), 1);  // At least immediate tests

    // Test 1.3: Disable/enable tests
    scheduler.disable_test("performance_tests");
    scheduler.enable_test("performance_tests");
    ASSERT_EQ(scheduler.get_scheduled_test_count(), 3);
  });

  // Test 2: Test result collection
  TEST_CASE("Test Result Collection") {
    TestResultCollector collector;

    // Test 2.1: Add passing results
    TestResultCollector::TestResult result1;
    result1.test_name = "test1";
    result1.passed = true;
    result1.duration = std::chrono::milliseconds(10);
    collector.add_result(result1);

    TestResultCollector::TestResult result2;
    result2.test_name = "test2";
    result2.passed = true;
    result2.duration = std::chrono::milliseconds(20);
    collector.add_result(result2);

    ASSERT_EQ(collector.get_passed_count(), 2);
    ASSERT_EQ(collector.get_failed_count(), 0);

    // Test 2.2: Add failing result
    TestResultCollector::TestResult result3;
    result3.test_name = "test3";
    result3.passed = false;
    result3.error_message = "Assertion failed";
    collector.add_result(result3);

    ASSERT_EQ(collector.get_passed_count(), 2);
    ASSERT_EQ(collector.get_failed_count(), 1);

    // Test 2.3: Get failed results
    auto failed = collector.get_failed_results();
    ASSERT_EQ(failed.size(), 1);
    ASSERT_EQ(failed[0].test_name, "test3");

    // Test 2.4: Calculate pass rate
    double pass_rate = collector.get_pass_rate();
    ASSERT_EQ(pass_rate, 66.66666666666666);  // 2/3 * 100
  });

  // Test 3: Test notifications
  TEST_CASE("Test Notifications") {
    TestNotifier notifier;

    // Test 3.1: Send failure notification
    notifier.notify_test_failure("test1", "Assertion failed");
    ASSERT_EQ(notifier.get_notification_count(), 1);

    // Test 3.2: Send success notification
    notifier.notify_test_success("test2");
    ASSERT_EQ(notifier.get_notification_count(), 2);

    // Test 3.3: Get notifications
    auto notifications = notifier.get_notifications();
    ASSERT_EQ(notifications.size(), 2);
    ASSERT_TRUE(notifications[0].message.find("failed") != std::string::npos);
    ASSERT_TRUE(notifications[1].message.find("passed") != std::string::npos);

    // Test 3.4: Clear notifications
    notifier.clear();
    ASSERT_EQ(notifier.get_notification_count(), 0);
  });

  // Test 4: Test maintenance
  TEST_CASE("Test Maintenance") {
    TestMaintenanceManager manager;

    // Test 4.1: Add maintenance tasks
    manager.add_maintenance_task("update_test_data", "Update test fixtures");
    manager.add_maintenance_task("refactor_tests", "Refactor legacy tests");
    ASSERT_EQ(manager.get_task_count(), 2);

    // Test 4.2: Get pending tasks
    auto pending = manager.get_pending_tasks();
    ASSERT_EQ(pending.size(), 2);

    // Test 4.3: Complete task
    manager.complete_task("update_test_data");
    pending = manager.get_pending_tasks();
    ASSERT_EQ(pending.size(), 1);

    // Test 4.4: Get overdue tasks
    auto overdue = manager.get_overdue_tasks();
    ASSERT_EQ(overdue.size(), 0);  // None are overdue yet
  });

  // Test 5: End-to-end automation workflow
  TEST_CASE("End-to-End Automation Workflow") {
    TestScheduler scheduler;
    TestResultCollector collector;
    TestNotifier notifier;

    // Test 5.1: Schedule tests
    scheduler.add_test("automated_test_1", TestScheduler::Schedule::Immediate);
    scheduler.add_test("automated_test_2", TestScheduler::Schedule::Immediate);

    // Test 5.2: Get tests to run
    auto tests_to_run = scheduler.get_tests_to_run();
    ASSERT_GE(tests_to_run.size(), 2);

    // Test 5.3: Simulate test execution and collect results
    for (const auto& test_name : tests_to_run) {
      TestResultCollector::TestResult result;
      result.test_name = test_name;
      result.passed = (test_name == "automated_test_1");  // First passes, second fails
      result.duration = std::chrono::milliseconds(50);
      collector.add_result(result);

      // Test 5.4: Send notifications
      if (result.passed) {
        notifier.notify_test_success(test_name);
      } else {
        notifier.notify_test_failure(test_name, "Test failed");
      }
    }

    // Test 5.5: Verify results
    ASSERT_EQ(collector.get_passed_count(), 1);
    ASSERT_EQ(collector.get_failed_count(), 1);
    ASSERT_EQ(notifier.get_notification_count(), 2);
  });

  return current_suite->all_passed() ? 0 : 1;
}
