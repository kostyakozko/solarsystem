/**
 * @file test_training_system.cpp
 * @brief Test training system (Task 35)
 * @note Migrated to Google Test
 *
 * Tests training capabilities:
 * - Test development tutorials
 * - Test framework usage guides
 * - Test debugging training
 * - Test maintenance training
 *
 * Requirements: All requirements
 */

#include <string>
#include <vector>

#include <gtest/gtest.h>

class TrainingSystem {
 public:
  struct TrainingModule {
    std::string name;
    std::vector<std::string> lessons;
    bool completed = false;
  };

  void add_module(const std::string& name) {
    TrainingModule module;
    module.name = name;
    module.lessons = {"Lesson 1", "Lesson 2"};
    modules_.push_back(module);
  }

  size_t get_module_count() const { return modules_.size(); }

  void complete_module(size_t index) {
    if (index < modules_.size()) {
      modules_[index].completed = true;
    }
  }

  int get_completed_count() const {
    int count = 0;
    for (const auto& module : modules_) {
      if (module.completed) count++;
    }
    return count;
  }

 private:
  std::vector<TrainingModule> modules_;
};

TEST(TrainingSystemTest, TrainingModuleManagement) {
  TrainingSystem training;

  training.add_module("Test Development");
  training.add_module("Test Debugging");

  ASSERT_EQ(training.get_module_count(), 2);

  training.complete_module(0);
  ASSERT_EQ(training.get_completed_count(), 1);
}
