/**
 * @file test_profiling.cpp
 * @brief Unit tests for profiling tools
 */

#include <gtest/gtest.h>

#include <thread>

#include "solar_core/performance/profiling.hpp"

using namespace SolarSystem::Performance;

class ProfilingTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ExecutionTracer::instance().clear();
    ExecutionTracer::instance().set_enabled(true);
    BottleneckAnalyzer::instance().clear();
  }
};

// ExecutionTracer tests
TEST_F(ProfilingTest, TracerStartsTrace) {
  auto trace_id = ExecutionTracer::instance().start_trace("test_op", "test_component");
  EXPECT_FALSE(trace_id.empty());

  auto spans = ExecutionTracer::instance().get_trace(trace_id);
  EXPECT_EQ(spans.size(), 1);
  EXPECT_EQ(spans[0].operation_name, "test_op");
}

TEST_F(ProfilingTest, TracerCreatesChildSpans) {
  auto trace_id = ExecutionTracer::instance().start_trace("parent", "comp");
  auto span_id = ExecutionTracer::instance().start_span(trace_id, "child", trace_id);

  ExecutionTracer::instance().end_span(span_id);
  ExecutionTracer::instance().end_span(trace_id);

  auto spans = ExecutionTracer::instance().get_trace(trace_id);
  EXPECT_EQ(spans.size(), 2);
}

TEST_F(ProfilingTest, TracerRecordsDuration) {
  auto trace_id = ExecutionTracer::instance().start_trace("timed_op");
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ExecutionTracer::instance().end_span(trace_id);

  auto spans = ExecutionTracer::instance().get_trace(trace_id);
  EXPECT_GE(spans[0].duration_ms(), 10);
}

TEST_F(ProfilingTest, TracerFiltersByComponent) {
  ExecutionTracer::instance().start_trace("op1", "jpl");
  ExecutionTracer::instance().start_trace("op2", "cache");
  ExecutionTracer::instance().start_trace("op3", "jpl");

  auto jpl_spans = ExecutionTracer::instance().filter_by_component("jpl");
  EXPECT_EQ(jpl_spans.size(), 2);
}

TEST_F(ProfilingTest, TracerDisables) {
  ExecutionTracer::instance().set_enabled(false);
  auto trace_id = ExecutionTracer::instance().start_trace("disabled_op");
  EXPECT_TRUE(trace_id.empty());
}

TEST_F(ProfilingTest, ScopedSpanAutoEnds) {
  std::string trace_id = ExecutionTracer::instance().start_trace("parent");
  {
    ScopedSpan span(trace_id, "scoped_child");
    span.add_tag("key", "value");
  }

  auto spans = ExecutionTracer::instance().get_trace(trace_id);
  EXPECT_EQ(spans.size(), 2);
}

// BottleneckAnalyzer tests
TEST_F(ProfilingTest, AnalyzerRecordsOperations) {
  BottleneckAnalyzer::instance().record_operation("fetch", "jpl", 100);
  BottleneckAnalyzer::instance().record_operation("fetch", "jpl", 150);

  auto stats = BottleneckAnalyzer::instance().get_operation_stats();
  EXPECT_EQ(stats.size(), 1);
  EXPECT_EQ(stats[0].count, 2);
  EXPECT_EQ(stats[0].avg_time_ms, 125);
}

TEST_F(ProfilingTest, AnalyzerIdentifiesBottlenecks) {
  BottleneckAnalyzer::instance().record_operation("slow_op", "comp", 500, true);
  BottleneckAnalyzer::instance().record_operation("fast_op", "comp", 10);

  auto bottlenecks = BottleneckAnalyzer::instance().identify_bottlenecks();
  EXPECT_FALSE(bottlenecks.empty());
  EXPECT_EQ(bottlenecks[0].operation, "slow_op");
}

TEST_F(ProfilingTest, AnalyzerDetectsContention) {
  BottleneckAnalyzer::instance().record_operation("op", "comp", 100);
  BottleneckAnalyzer::instance().record_contention("mutex_lock", 50);
  BottleneckAnalyzer::instance().record_contention("mutex_lock", 60);

  auto bottlenecks = BottleneckAnalyzer::instance().identify_bottlenecks();
  bool found_contention = false;
  for (const auto& b : bottlenecks) {
    if (b.type == Bottleneck::Type::CONTENTION) {
      found_contention = true;
      break;
    }
  }
  EXPECT_TRUE(found_contention);
}

TEST_F(ProfilingTest, AnalyzerProvidesRecommendations) {
  BottleneckAnalyzer::instance().record_operation("cpu_op", "comp", 200, true);

  auto recommendations = BottleneckAnalyzer::instance().get_recommendations();
  EXPECT_FALSE(recommendations.empty());
}

TEST_F(ProfilingTest, AnalyzerGetsHotspots) {
  for (int i = 0; i < 5; ++i) {
    BottleneckAnalyzer::instance().record_operation("op" + std::to_string(i), "comp",
                                                    (i + 1) * 100);
  }

  auto hotspots = BottleneckAnalyzer::instance().get_hotspots(3);
  EXPECT_LE(hotspots.size(), 3);
}
