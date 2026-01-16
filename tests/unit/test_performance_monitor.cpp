/**
 * @file test_performance_monitor.cpp
 * @brief Unit tests for performance monitoring system
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "solar_core/performance/performance_monitor.hpp"

using namespace SolarSystem::Performance;
// Counter tests
TEST(PerformanceMonitorTests, Counter_Basic_Operations) {
  auto& monitor = PerformanceMonitor::instance();
  monitor.reset_all();

  auto counter = monitor.register_counter("test_counter");

  if (counter->get() != 0) throw std::runtime_error("Counter should start at 0");

  counter->increment();
  if (counter->get() != 1) throw std::runtime_error("Counter should be 1 after increment");

  counter->increment(5);
  if (counter->get() != 6) throw std::runtime_error("Counter should be 6 after increment(5)");

  counter->reset();
  if (counter->get() != 0) throw std::runtime_error("Counter should be 0 after reset");
}

TEST(PerformanceMonitorTests, Counter_Registration) {
  auto& monitor = PerformanceMonitor::instance();
  monitor.reset_all();

  auto counter1 = monitor.register_counter("reg_counter");
  auto counter2 = monitor.get_counter("reg_counter");

  if (!counter2) throw std::runtime_error("Should retrieve registered counter");
  if (counter1.get() != counter2.get()) {
    throw std::runtime_error("Should return same counter instance");
  }

  counter1->increment();
  if (counter2->get() != 1) {
    throw std::runtime_error("Both references should see same value");
  }
}

// Gauge tests
TEST(PerformanceMonitorTests, Gauge_Basic_Operations) {
  auto& monitor = PerformanceMonitor::instance();
  monitor.reset_all();

  auto gauge = monitor.register_gauge("test_gauge");

  if (gauge->get() != 0) throw std::runtime_error("Gauge should start at 0");

  gauge->set(42.5);
  if (gauge->get() != 42.5) throw std::runtime_error("Gauge should be 42.5");

  gauge->increment(10);
  if (gauge->get() != 52.5) throw std::runtime_error("Gauge should be 52.5");

  gauge->decrement(2.5);
  if (gauge->get() != 50.0) throw std::runtime_error("Gauge should be 50.0");
}

// Histogram tests
TEST(PerformanceMonitorTests, Histogram_Basic_Operations) {
  auto& monitor = PerformanceMonitor::instance();
  monitor.reset_all();

  auto histogram = monitor.register_histogram("test_histogram");

  histogram->observe(1.0);
  histogram->observe(2.0);
  histogram->observe(3.0);
  histogram->observe(4.0);
  histogram->observe(5.0);

  auto stats = histogram->get_statistics();

  if (stats.count != 5) throw std::runtime_error("Should have 5 observations");
  if (stats.min != 1.0) throw std::runtime_error("Min should be 1.0");
  if (stats.max != 5.0) throw std::runtime_error("Max should be 5.0");
  if (stats.mean != 3.0) throw std::runtime_error("Mean should be 3.0");
  if (stats.median != 3.0) throw std::runtime_error("Median should be 3.0");
}

TEST(PerformanceMonitorTests, Histogram_Empty_Statistics) {
  auto& monitor = PerformanceMonitor::instance();
  monitor.reset_all();

  auto histogram = monitor.register_histogram("empty_histogram");
  auto stats = histogram->get_statistics();

  if (stats.count != 0) throw std::runtime_error("Empty histogram should have count 0");
  if (stats.min != 0) throw std::runtime_error("Empty histogram min should be 0");
  if (stats.max != 0) throw std::runtime_error("Empty histogram max should be 0");
}

// Timer tests
TEST(PerformanceMonitorTests, Timer_Basic_Operations) {
  auto& monitor = PerformanceMonitor::instance();
  monitor.reset_all();

  auto timer = monitor.register_timer("test_timer");

  timer->record(0.001);  // 1ms
  timer->record(0.002);  // 2ms
  timer->record(0.003);  // 3ms

  auto stats = timer->get_statistics();

  if (stats.count != 3) throw std::runtime_error("Should have 3 recordings");
  if (stats.min != 0.001) throw std::runtime_error("Min should be 0.001");
  if (stats.max != 0.003) throw std::runtime_error("Max should be 0.003");
}

TEST(PerformanceMonitorTests, Timer_Scoped_Timing) {
  auto& monitor = PerformanceMonitor::instance();
  monitor.reset_all();

  auto timer = monitor.register_timer("scoped_timer");

  {
    auto scoped = timer->time();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  auto stats = timer->get_statistics();

  if (stats.count != 1) throw std::runtime_error("Should have 1 recording");
  if (stats.min < 0.008) {  // At least 8ms (allowing for timing variance)
    throw std::runtime_error("Should have recorded at least 8ms");
  }
}

// Threshold tests
TEST(PerformanceMonitorTests, Threshold_Management) {
  auto& monitor = PerformanceMonitor::instance();
  monitor.reset_all();

  PerformanceThreshold threshold;
  threshold.metric_name = "test_metric";
  threshold.warning_threshold = 80.0;
  threshold.critical_threshold = 100.0;
  threshold.above_threshold = true;

  monitor.set_threshold(threshold);

  auto thresholds = monitor.get_thresholds();
  if (thresholds.size() != 1) throw std::runtime_error("Should have 1 threshold");
  if (thresholds[0].metric_name != "test_metric") {
    throw std::runtime_error("Wrong threshold name");
  }

  monitor.remove_threshold("test_metric");
  thresholds = monitor.get_thresholds();
  if (!thresholds.empty()) throw std::runtime_error("Should have no thresholds");
}

// Alert tests
TEST(PerformanceMonitorTests, Alert_Management) {
  auto& monitor = PerformanceMonitor::instance();
  monitor.reset_all();
  monitor.clear_alerts();

  auto alerts = monitor.get_alerts();
  if (!alerts.empty()) throw std::runtime_error("Should start with no alerts");

  monitor.clear_alerts();
  alerts = monitor.get_alerts();
  if (!alerts.empty()) throw std::runtime_error("Should have no alerts after clear");
}

// Report generation tests
TEST(PerformanceMonitorTests, Report_Generation) {
  auto& monitor = PerformanceMonitor::instance();
  monitor.reset_all();

  auto counter = monitor.register_counter("report_counter");
  auto gauge = monitor.register_gauge("report_gauge");

  counter->increment(10);
  gauge->set(42.0);

  std::string report = monitor.generate_report();

  if (report.empty()) throw std::runtime_error("Report should not be empty");
  if (report.find("report_counter") == std::string::npos) {
    throw std::runtime_error("Report should contain counter name");
  }
  if (report.find("report_gauge") == std::string::npos) {
    throw std::runtime_error("Report should contain gauge name");
  }
}

// Metrics retrieval tests
TEST(PerformanceMonitorTests, Get_All_Metrics) {
  auto& monitor = PerformanceMonitor::instance();
  monitor.reset_all();

  monitor.register_counter("metric1")->increment(5);
  monitor.register_gauge("metric2")->set(10);

  auto metrics = monitor.get_all_metrics();

  if (metrics.size() < 2) throw std::runtime_error("Should have at least 2 metrics");

  bool found_counter = false;
  bool found_gauge = false;

  for (const auto& metric : metrics) {
    if (metric.name == "metric1" && metric.type == MetricType::COUNTER) {
      found_counter = true;
      if (metric.value != 5) throw std::runtime_error("Counter value should be 5");
    }
    if (metric.name == "metric2" && metric.type == MetricType::GAUGE) {
      found_gauge = true;
      if (metric.value != 10) throw std::runtime_error("Gauge value should be 10");
    }
  }

  if (!found_counter) throw std::runtime_error("Should find counter metric");
  if (!found_gauge) throw std::runtime_error("Should find gauge metric");
}

// Monitoring control tests
TEST(PerformanceMonitorTests, Monitoring_EnableDisable) {
  auto& monitor = PerformanceMonitor::instance();

  if (!monitor.is_monitoring_enabled()) {
    throw std::runtime_error("Monitoring should be enabled by default");
  }

  monitor.enable_monitoring(false);
  if (monitor.is_monitoring_enabled()) {
    throw std::runtime_error("Monitoring should be disabled");
  }

  monitor.enable_monitoring(true);
  if (!monitor.is_monitoring_enabled()) {
    throw std::runtime_error("Monitoring should be enabled");
  }
}

// Scoped timer helper tests
TEST(PerformanceMonitorTests, Scoped_Performance_Timer) {
  auto& monitor = PerformanceMonitor::instance();
  monitor.reset_all();

  {
    ScopedPerformanceTimer timer("scoped_perf_timer");
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

  auto timer = monitor.get_timer("scoped_perf_timer");
  if (!timer) throw std::runtime_error("Timer should be registered");

  auto stats = timer->get_statistics();
  if (stats.count != 1) throw std::runtime_error("Should have 1 recording");
  if (stats.min < 0.003) {  // At least 3ms
    throw std::runtime_error("Should have recorded at least 3ms");
  }
}

// Macro tests
TEST(PerformanceMonitorTests, Performance_Macros) {
  auto& monitor = PerformanceMonitor::instance();
  monitor.reset_all();

  PERF_COUNTER_INC("macro_counter");
  PERF_COUNTER_INC("macro_counter");
  PERF_GAUGE_SET("macro_gauge", 99.9);

  auto counter = monitor.get_counter("macro_counter");
  auto gauge = monitor.get_gauge("macro_gauge");

  if (!counter) throw std::runtime_error("Counter should exist");
  if (!gauge) throw std::runtime_error("Gauge should exist");

  if (counter->get() != 2) throw std::runtime_error("Counter should be 2");
  if (gauge->get() != 99.9) throw std::runtime_error("Gauge should be 99.9");
}

// Concurrent access tests
TEST(PerformanceMonitorTests, Concurrent_Counter_Access) {
  auto& monitor = PerformanceMonitor::instance();
  monitor.reset_all();

  auto counter = monitor.register_counter("concurrent_counter");

  std::vector<std::thread> threads;
  for (int i = 0; i < 10; ++i) {
    threads.emplace_back([&counter]() {
      for (int j = 0; j < 100; ++j) {
        counter->increment();
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  if (counter->get() != 1000) {
    throw std::runtime_error("Counter should be 1000 after concurrent increments");
  }
}

// Reset tests
TEST(PerformanceMonitorTests, Reset_All_Metrics) {
  auto& monitor = PerformanceMonitor::instance();

  auto counter = monitor.register_counter("reset_counter");
  auto gauge = monitor.register_gauge("reset_gauge");

  counter->increment(10);
  gauge->set(20);

  monitor.reset_all();

  if (counter->get() != 0) throw std::runtime_error("Counter should be 0 after reset");
  // Note: Gauges are not reset by reset_all, only counters and alerts
}
