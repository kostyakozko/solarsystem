/**
 * @file error_recovery.cpp
 * @brief Implementation of advanced error recovery mechanisms
 */

#include "solar_utils/error_recovery.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <thread>

#include "solar_utils/file_resource_manager.hpp"
#include "solar_utils/network_resource_manager.hpp"
#include "solar_utils/resource_manager.hpp"

namespace SolarSystem::Utils {

// AdvancedErrorRecoveryManager implementation
AdvancedErrorRecoveryManager& AdvancedErrorRecoveryManager::instance() {
  static AdvancedErrorRecoveryManager instance;
  return instance;
}

AdvancedErrorRecoveryManager::~AdvancedErrorRecoveryManager() {
  // Cancel all active recoveries
  std::lock_guard<std::mutex> lock(recoveries_mutex_);
  for (auto& [id, context] : active_recoveries_) {
    context.status = RecoveryStatus::Cancelled;
  }
}

void AdvancedErrorRecoveryManager::register_recovery_workflow(const RecoveryWorkflow& workflow) {
  std::lock_guard<std::mutex> lock(workflows_mutex_);
  workflows_[workflow.workflow_id] = workflow;
}

void AdvancedErrorRecoveryManager::unregister_recovery_workflow(const std::string& workflow_id) {
  std::lock_guard<std::mutex> lock(workflows_mutex_);
  workflows_.erase(workflow_id);
}

std::vector<std::string> AdvancedErrorRecoveryManager::get_available_workflows() const {
  std::lock_guard<std::mutex> lock(workflows_mutex_);
  std::vector<std::string> workflow_ids;
  workflow_ids.reserve(workflows_.size());

  for (const auto& [id, workflow] : workflows_) {
    workflow_ids.push_back(id);
  }

  return workflow_ids;
}

RecoveryWorkflow AdvancedErrorRecoveryManager::get_workflow(const std::string& workflow_id) const {
  std::lock_guard<std::mutex> lock(workflows_mutex_);
  auto it = workflows_.find(workflow_id);
  if (it != workflows_.end()) {
    return it->second;
  }
  return RecoveryWorkflow{};
}

std::future<bool> AdvancedErrorRecoveryManager::execute_recovery_async(
    const DetailedError& error, const std::string& workflow_id, RecoveryMode mode) {
  return std::async(std::launch::async, [this, error, workflow_id, mode]() {
    return execute_recovery_sync(error, workflow_id, mode);
  });
}

bool AdvancedErrorRecoveryManager::execute_recovery_sync(const DetailedError& error,
                                                         const std::string& workflow_id,
                                                         RecoveryMode mode) {
  if (!automatic_recovery_enabled_ && mode == RecoveryMode::Automatic) {
    return false;
  }

  // Check concurrent recovery limit
  {
    std::lock_guard<std::mutex> lock(recoveries_mutex_);
    if (active_recoveries_.size() >= max_concurrent_recoveries_) {
      return false;
    }
  }

  // Select workflow
  std::string selected_workflow_id =
      workflow_id.empty() ? select_best_workflow(error) : workflow_id;

  if (selected_workflow_id.empty()) {
    return false;
  }

  RecoveryWorkflow workflow = get_workflow(selected_workflow_id);
  if (workflow.workflow_id.empty()) {
    return false;
  }

  // Create recovery context
  RecoveryContext context;
  context.execution_id = generate_execution_id();
  context.workflow_id = selected_workflow_id;
  context.original_error = error;
  context.status = RecoveryStatus::InProgress;
  context.start_time = std::chrono::system_clock::now();

  // Add to active recoveries
  {
    std::lock_guard<std::mutex> lock(recoveries_mutex_);
    active_recoveries_[context.execution_id] = context;
  }

  // Execute workflow
  bool success = execute_workflow(workflow, context);

  // Update context
  context.end_time = std::chrono::system_clock::now();
  context.status = success ? RecoveryStatus::Completed : RecoveryStatus::Failed;

  // Update active recoveries
  {
    std::lock_guard<std::mutex> lock(recoveries_mutex_);
    active_recoveries_[context.execution_id] = context;
  }

  // Record learning data
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(context.end_time - context.start_time);

  RecoveryAction action;
  action.strategy = error.get_recovery_strategy();
  action.description = workflow.name;

  record_recovery_outcome(error, action, success, duration);

  // Cleanup completed recovery after a delay
  std::thread([this, execution_id = context.execution_id]() {
    std::this_thread::sleep_for(std::chrono::minutes(5));
    std::lock_guard<std::mutex> lock(recoveries_mutex_);
    active_recoveries_.erase(execution_id);
  }).detach();

  return success;
}

void AdvancedErrorRecoveryManager::set_user_interaction_handler(
    std::function<UserInteractionResponse(const UserInteractionRequest&)> handler) {
  user_interaction_handler_ = handler;
}

UserInteractionResponse AdvancedErrorRecoveryManager::request_user_interaction(
    const UserInteractionRequest& request) {
  if (!user_interaction_handler_) {
    // Default handler - return cancelled response
    UserInteractionResponse response;
    response.request_id = request.id;
    response.cancelled = true;
    response.response_time = std::chrono::system_clock::now();
    return response;
  }

  return user_interaction_handler_(request);
}

std::vector<RecoveryContext> AdvancedErrorRecoveryManager::get_active_recoveries() const {
  std::lock_guard<std::mutex> lock(recoveries_mutex_);
  std::vector<RecoveryContext> contexts;
  contexts.reserve(active_recoveries_.size());

  for (const auto& [id, context] : active_recoveries_) {
    contexts.push_back(context);
  }

  return contexts;
}

RecoveryContext AdvancedErrorRecoveryManager::get_recovery_status(
    const std::string& execution_id) const {
  std::lock_guard<std::mutex> lock(recoveries_mutex_);
  auto it = active_recoveries_.find(execution_id);
  if (it != active_recoveries_.end()) {
    return it->second;
  }
  return RecoveryContext{};
}

void AdvancedErrorRecoveryManager::cancel_recovery(const std::string& execution_id) {
  std::lock_guard<std::mutex> lock(recoveries_mutex_);
  auto it = active_recoveries_.find(execution_id);
  if (it != active_recoveries_.end()) {
    it->second.status = RecoveryStatus::Cancelled;
  }
}

void AdvancedErrorRecoveryManager::enable_automatic_recovery(bool enabled) {
  automatic_recovery_enabled_ = enabled;
}

void AdvancedErrorRecoveryManager::set_automatic_recovery_timeout(std::chrono::seconds timeout) {
  automatic_recovery_timeout_ = timeout;
}

void AdvancedErrorRecoveryManager::set_max_concurrent_recoveries(size_t max_concurrent) {
  max_concurrent_recoveries_ = max_concurrent;
}

void AdvancedErrorRecoveryManager::record_recovery_outcome(const DetailedError& error,
                                                           const RecoveryAction& action,
                                                           bool successful,
                                                           std::chrono::milliseconds duration) {
  std::lock_guard<std::mutex> lock(learning_mutex_);

  LearningDataPoint data_point;
  data_point.error = error;
  data_point.attempted_recovery = action;
  data_point.recovery_successful = successful;
  data_point.recovery_time = duration;
  data_point.timestamp = std::chrono::system_clock::now();

  // Add context factors
  data_point.context_factors["error_category"] =
      ErrorUtils::error_category_to_string(error.category);
  data_point.context_factors["error_severity"] =
      ErrorUtils::error_severity_to_string(error.severity);
  data_point.context_factors["recovery_strategy"] =
      ErrorUtils::recovery_strategy_to_string(action.strategy);

  learning_data_.push_back(data_point);

  // Maintain size limit
  if (learning_data_.size() > max_learning_data_points_) {
    learning_data_.erase(
        learning_data_.begin(),
        learning_data_.begin() + static_cast<std::ptrdiff_t>(learning_data_.size() - max_learning_data_points_));
  }
}

std::vector<LearningDataPoint> AdvancedErrorRecoveryManager::get_learning_data() const {
  std::lock_guard<std::mutex> lock(learning_mutex_);
  return learning_data_;
}

void AdvancedErrorRecoveryManager::update_recovery_strategies_from_learning() {
  std::lock_guard<std::mutex> lock(learning_mutex_);

  // Analyze learning data to improve recovery strategies
  std::unordered_map<ErrorCode, std::vector<LearningDataPoint>> error_data;

  for (const auto& data_point : learning_data_) {
    error_data[data_point.error.code].push_back(data_point);
  }

  // Update strategies based on success rates
  for (const auto& [error_code, data_points] : error_data) {
    if (data_points.size() < 5) continue;  // Need minimum data points

    std::unordered_map<RecoveryStrategy, size_t> strategy_success;
    std::unordered_map<RecoveryStrategy, size_t> strategy_total;

    for (const auto& point : data_points) {
      strategy_total[point.attempted_recovery.strategy]++;
      if (point.recovery_successful) {
        strategy_success[point.attempted_recovery.strategy]++;
      }
    }

    // Find best strategy
    RecoveryStrategy best_strategy = RecoveryStrategy::None;
    double best_success_rate = 0.0;

    for (const auto& [strategy, total] : strategy_total) {
      if (total >= 3) {  // Minimum attempts
        double success_rate = static_cast<double>(strategy_success[strategy]) / total;
        if (success_rate > best_success_rate) {
          best_success_rate = success_rate;
          best_strategy = strategy;
        }
      }
    }

    // Register improved strategy if found
    if (best_strategy != RecoveryStrategy::None && best_success_rate > 0.7) {
      // Register improved strategy with the error handling system
      ErrorHandlingSystem::instance().register_recovery_strategy(
          error_code, [best_strategy](const DetailedError& error) {
            RecoveryAction action(best_strategy, "Learned strategy for " +
                                                     ErrorUtils::error_code_to_string(error.code));
            return action;
          });
    }
  }
}

std::string AdvancedErrorRecoveryManager::generate_execution_id() const {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis(0, 15);

  std::ostringstream oss;
  oss << "recovery_";
  for (int i = 0; i < 8; ++i) {
    oss << std::hex << dis(gen);
  }

  return oss.str();
}

std::string AdvancedErrorRecoveryManager::select_best_workflow(const DetailedError& error) const {
  std::lock_guard<std::mutex> lock(workflows_mutex_);

  // Simple selection based on error category
  std::string category_str = ErrorUtils::error_category_to_string(error.category);

  for (const auto& [id, workflow] : workflows_) {
    if (workflow.name.find(category_str) != std::string::npos) {
      return id;
    }
  }

  // Return first available workflow as fallback
  if (!workflows_.empty()) {
    return workflows_.begin()->first;
  }

  return "";
}

bool AdvancedErrorRecoveryManager::execute_workflow(const RecoveryWorkflow& workflow,
                                                    RecoveryContext& context) {
  auto start_time = std::chrono::steady_clock::now();

  try {
    for (const auto& step : workflow.steps) {
      // Check if recovery was cancelled
      {
        std::lock_guard<std::mutex> lock(recoveries_mutex_);
        auto it = active_recoveries_.find(context.execution_id);
        if (it != active_recoveries_.end() && it->second.status == RecoveryStatus::Cancelled) {
          return false;
        }
      }

      // Check timeout
      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::steady_clock::now() - start_time);
      if (elapsed > workflow.total_timeout) {
        context.status = RecoveryStatus::Timeout;
        return false;
      }

      // Execute step
      context.current_step = step.step_id;
      bool step_success = execute_step(step, context);

      if (step_success) {
        context.completed_steps.push_back(step.step_id);
      } else {
        if (step.is_critical) {
          context.failure_reason = "Critical step failed: " + step.step_id;
          return false;
        }
        // Try fallback strategy for non-critical steps
        if (step.fallback_strategy != RecoveryStrategy::None) {
          // Implement fallback logic here
        }
      }

      // Progress callback
      if (workflow.progress_callback) {
        workflow.progress_callback("Completed step: " + step.step_id);
      }
    }

    return true;

  } catch (const std::exception& ex) {
    context.failure_reason = "Exception during workflow execution: " + std::string(ex.what());
    if (workflow.error_callback) {
      DetailedError workflow_error(ErrorCode::OperationFailed, ex.what());
      workflow.error_callback(workflow_error);
    }
    return false;
  }
}

bool AdvancedErrorRecoveryManager::execute_step(const RecoveryStep& step,
                                                RecoveryContext& context) {
  auto start_time = std::chrono::steady_clock::now();

  for (size_t attempt = 0; attempt < step.max_retries; ++attempt) {
    try {
      // Handle user interactions first
      for (const auto& interaction : step.user_interactions) {
        UserInteractionResponse response = request_user_interaction(interaction);
        if (response.cancelled || response.timeout) {
          return false;
        }

        // Store user response in context
        context.context_data[interaction.id] =
            response.user_input.empty() ? response.selected_option : response.user_input;
      }

      // Execute step action
      if (step.action && step.action()) {
        // Validate step result
        if (step.validation && !step.validation()) {
          continue;  // Retry
        }
        return true;
      }

      // Check timeout
      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::steady_clock::now() - start_time);
      if (elapsed > step.timeout) {
        break;
      }

      // Delay before retry
      if (attempt < step.max_retries - 1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100 * (attempt + 1)));
      }

    } catch (const std::exception& ex) {
      DetailedError step_error(ErrorCode::OperationFailed,
                               "Step execution failed: " + std::string(ex.what()));
      context.recovery_errors.push_back(step_error);
    }
  }

  return false;
}

// ErrorPreventionSystem implementation
ErrorPreventionSystem& ErrorPreventionSystem::instance() {
  static ErrorPreventionSystem instance;
  return instance;
}

ErrorPreventionSystem::~ErrorPreventionSystem() = default;

void ErrorPreventionSystem::register_prevention_rule(const PreventionRule& rule) {
  std::lock_guard<std::mutex> lock(rules_mutex_);
  prevention_rules_[rule.rule_id] = rule;
}

void ErrorPreventionSystem::unregister_prevention_rule(const std::string& rule_id) {
  std::lock_guard<std::mutex> lock(rules_mutex_);
  prevention_rules_.erase(rule_id);
}

void ErrorPreventionSystem::enable_prevention_rule(const std::string& rule_id, bool enabled) {
  std::lock_guard<std::mutex> lock(rules_mutex_);
  auto it = prevention_rules_.find(rule_id);
  if (it != prevention_rules_.end()) {
    it->second.is_enabled = enabled;
  }
}

bool ErrorPreventionSystem::check_and_prevent_errors(
    const std::vector<DetailedError>& recent_errors) {
  if (!prevention_enabled_) {
    return false;
  }

  std::lock_guard<std::mutex> lock(rules_mutex_);
  bool prevented_any = false;

  for (auto& [rule_id, rule] : prevention_rules_) {
    if (!rule.is_enabled) {
      continue;
    }

    if (evaluate_prevention_rule(rule, recent_errors)) {
      rule.trigger_count++;

      try {
        if (rule.prevention_action && rule.prevention_action()) {
          rule.success_count++;
          prevented_any = true;

          // Create prevented error record
          DetailedError prevented_error(ErrorCode::Unknown, "Error prevented by rule: " + rule.name,
                                        ErrorSeverity::Info);
          prevented_errors_.push_back(prevented_error);
        }
      } catch (const std::exception& ex) {
        // Prevention action failed
        DetailedError prevention_error(ErrorCode::OperationFailed,
                                       "Prevention action failed: " + std::string(ex.what()));
        prevented_errors_.push_back(prevention_error);
      }
    }
  }

  return prevented_any;
}

std::vector<DetailedError> ErrorPreventionSystem::get_prevented_errors() const {
  std::lock_guard<std::mutex> lock(rules_mutex_);
  return prevented_errors_;
}

void ErrorPreventionSystem::update_rule_effectiveness() {
  std::lock_guard<std::mutex> lock(rules_mutex_);

  for (auto& [rule_id, rule] : prevention_rules_) {
    if (rule.trigger_count > 0) {
      double effectiveness = static_cast<double>(rule.success_count) / rule.trigger_count;

      // Disable rules with very low effectiveness
      if (effectiveness < 0.1 && rule.trigger_count > 10) {
        rule.is_enabled = false;
      }
    }
  }
}

std::vector<PreventionRule> ErrorPreventionSystem::get_most_effective_rules(size_t count) const {
  std::lock_guard<std::mutex> lock(rules_mutex_);

  std::vector<PreventionRule> rules;
  for (const auto& [id, rule] : prevention_rules_) {
    rules.push_back(rule);
  }

  // Sort by effectiveness
  std::sort(rules.begin(), rules.end(), [](const PreventionRule& a, const PreventionRule& b) {
    double eff_a =
        a.trigger_count > 0 ? static_cast<double>(a.success_count) / a.trigger_count : 0.0;
    double eff_b =
        b.trigger_count > 0 ? static_cast<double>(b.success_count) / b.trigger_count : 0.0;
    return eff_a > eff_b;
  });

  if (rules.size() > count) {
    rules.resize(count);
  }

  return rules;
}

void ErrorPreventionSystem::set_prevention_enabled(bool enabled) { prevention_enabled_ = enabled; }

void ErrorPreventionSystem::set_check_interval(std::chrono::seconds interval) {
  check_interval_ = interval;
}

bool ErrorPreventionSystem::evaluate_prevention_rule(const PreventionRule& rule,
                                                     const std::vector<DetailedError>& errors) {
  if (errors.empty()) {
    return false;
  }

  // Check if any recent error matches the rule's target
  for (const auto& error : errors) {
    if (rule.target_category != ErrorCategory::Unknown && error.category != rule.target_category) {
      continue;
    }

    if (!rule.target_codes.empty() && std::find(rule.target_codes.begin(), rule.target_codes.end(),
                                                error.code) == rule.target_codes.end()) {
      continue;
    }

    // Evaluate custom condition
    if (rule.condition && rule.condition(error)) {
      return true;
    }
  }

  return false;
}

void ErrorPreventionSystem::record_prevention_attempt(const std::string& rule_id, bool successful) {
  std::lock_guard<std::mutex> lock(rules_mutex_);
  auto it = prevention_rules_.find(rule_id);
  if (it != prevention_rules_.end()) {
    it->second.trigger_count++;
    if (successful) {
      it->second.success_count++;
    }
  }
}

// EarlyDetectionSystem implementation
EarlyDetectionSystem& EarlyDetectionSystem::instance() {
  static EarlyDetectionSystem instance;
  return instance;
}

EarlyDetectionSystem::~EarlyDetectionSystem() { stop_monitoring(); }

void EarlyDetectionSystem::register_monitor(const EarlyDetectionMonitor& monitor) {
  std::lock_guard<std::mutex> lock(monitors_mutex_);
  monitors_[monitor.monitor_id] = monitor;
}

void EarlyDetectionSystem::unregister_monitor(const std::string& monitor_id) {
  std::lock_guard<std::mutex> lock(monitors_mutex_);
  monitors_.erase(monitor_id);
}

void EarlyDetectionSystem::enable_monitor(const std::string& monitor_id, bool enabled) {
  std::lock_guard<std::mutex> lock(monitors_mutex_);
  auto it = monitors_.find(monitor_id);
  if (it != monitors_.end()) {
    it->second.is_active = enabled;
  }
}

void EarlyDetectionSystem::start_monitoring() {
  if (monitoring_active_.load()) {
    return;
  }

  monitoring_active_.store(true);
  monitoring_thread_ = std::make_unique<std::thread>(&EarlyDetectionSystem::monitoring_loop, this);
}

void EarlyDetectionSystem::stop_monitoring() {
  if (!monitoring_active_.load()) {
    return;
  }

  monitoring_active_.store(false);
  if (monitoring_thread_ && monitoring_thread_->joinable()) {
    monitoring_thread_->join();
  }
  monitoring_thread_.reset();
}

bool EarlyDetectionSystem::is_monitoring() const { return monitoring_active_.load(); }

double EarlyDetectionSystem::get_overall_health_score() const {
  std::lock_guard<std::mutex> lock(monitors_mutex_);

  if (monitors_.empty()) {
    return 1.0;
  }

  double total_score = 0.0;
  size_t active_monitors = 0;

  for (const auto& [id, monitor] : monitors_) {
    if (monitor.is_active) {
      total_score += monitor.last_health_score;
      active_monitors++;
    }
  }

  return active_monitors > 0 ? total_score / active_monitors : 1.0;
}

std::vector<DetailedError> EarlyDetectionSystem::get_detected_issues() const {
  std::lock_guard<std::mutex> lock(monitors_mutex_);
  return detected_issues_;
}

std::unordered_map<std::string, double> EarlyDetectionSystem::get_monitor_health_scores() const {
  std::lock_guard<std::mutex> lock(monitors_mutex_);

  std::unordered_map<std::string, double> scores;
  for (const auto& [id, monitor] : monitors_) {
    scores[id] = monitor.last_health_score;
  }

  return scores;
}

void EarlyDetectionSystem::set_monitoring_interval(std::chrono::seconds interval) {
  monitoring_interval_ = interval;
}

void EarlyDetectionSystem::set_health_thresholds(double warning_threshold,
                                                 double critical_threshold) {
  warning_threshold_ = warning_threshold;
  critical_threshold_ = critical_threshold;
}

void EarlyDetectionSystem::monitoring_loop() {
  while (monitoring_active_.load()) {
    {
      std::lock_guard<std::mutex> lock(monitors_mutex_);
      for (auto& [id, monitor] : monitors_) {
        if (monitor.is_active) {
          check_monitor(monitor);
        }
      }
    }

    std::this_thread::sleep_for(monitoring_interval_);
  }
}

void EarlyDetectionSystem::check_monitor(EarlyDetectionMonitor& monitor) {
  auto now = std::chrono::system_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - monitor.last_check);

  if (elapsed < monitor.check_interval) {
    return;
  }

  monitor.last_check = now;

  try {
    double health_score = monitor.health_check ? monitor.health_check() : 1.0;
    std::vector<DetailedError> issues =
        monitor.diagnostic ? monitor.diagnostic() : std::vector<DetailedError>{};

    process_monitor_results(monitor.monitor_id, health_score, issues);
    monitor.last_health_score = health_score;

  } catch (const std::exception& ex) {
    DetailedError monitor_error(ErrorCode::OperationFailed,
                                "Monitor check failed: " + std::string(ex.what()));
    detected_issues_.push_back(monitor_error);
    monitor.last_health_score = 0.0;
  }
}

void EarlyDetectionSystem::process_monitor_results(const std::string& monitor_id,
                                                   double health_score,
                                                   const std::vector<DetailedError>& issues) {
  // Add issues to detected issues
  for (const auto& issue : issues) {
    detected_issues_.push_back(issue);
  }

  // Generate health-based alerts
  if (health_score <= critical_threshold_) {
    DetailedError critical_alert(ErrorCode::PerformanceDegraded,
                                 "Critical health alert from monitor: " + monitor_id,
                                 ErrorSeverity::Critical);
    detected_issues_.push_back(critical_alert);
  } else if (health_score <= warning_threshold_) {
    DetailedError warning_alert(ErrorCode::PerformanceDegraded,
                                "Warning health alert from monitor: " + monitor_id,
                                ErrorSeverity::Warning);
    detected_issues_.push_back(warning_alert);
  }

  // Limit detected issues size
  if (detected_issues_.size() > 1000) {
    detected_issues_.erase(detected_issues_.begin(), detected_issues_.begin() + 500);
  }
}

// MLErrorPatternAnalyzer implementation
MLErrorPatternAnalyzer& MLErrorPatternAnalyzer::instance() {
  static MLErrorPatternAnalyzer instance;
  return instance;
}

void MLErrorPatternAnalyzer::train_model(const std::vector<LearningDataPoint>& training_data) {
  std::lock_guard<std::mutex> lock(model_mutex_);

  training_data_ = training_data;
  last_training_time_ = std::chrono::system_clock::now();

  // Simple weight update based on success patterns
  update_weights(training_data);
}

std::vector<std::pair<ErrorCode, double>> MLErrorPatternAnalyzer::predict_likely_errors(
    const std::vector<DetailedError>& recent_errors,
    const std::unordered_map<std::string, std::string>& context) const {
  std::lock_guard<std::mutex> lock(model_mutex_);

  std::vector<double> features = extract_features(recent_errors, context);
  std::vector<std::pair<ErrorCode, double>> predictions;

  // Predict probability for each known error code
  std::unordered_set<ErrorCode> known_codes;
  for (const auto& data_point : training_data_) {
    known_codes.insert(data_point.error.code);
  }

  for (ErrorCode code : known_codes) {
    double probability = calculate_error_probability(features, code);
    if (probability > 0.1) {  // Threshold for meaningful predictions
      predictions.emplace_back(code, probability);
    }
  }

  // Sort by probability
  std::sort(predictions.begin(), predictions.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

  return predictions;
}

std::vector<ErrorPattern> MLErrorPatternAnalyzer::discover_new_patterns(
    const std::vector<DetailedError>& errors) {
  std::vector<ErrorPattern> patterns;

  // Simple pattern discovery - look for sequences
  for (size_t len = 2; len <= 4 && len <= errors.size(); ++len) {
    for (size_t start = 0; start <= errors.size() - len; ++start) {
      ErrorPattern pattern;
      pattern.pattern_id = "ml_pattern_" + std::to_string(patterns.size());

      for (size_t i = start; i < start + len; ++i) {
        pattern.error_sequence.push_back(errors[i].code);
      }

      pattern.description = "ML-discovered pattern of length " + std::to_string(len);
      pattern.recommended_strategy = RecoveryStrategy::Retry;
      pattern.confidence_score = 0.5;  // Initial confidence
      pattern.occurrence_count = 1;
      pattern.first_seen = errors[start].timestamp;
      pattern.last_seen = errors[start + len - 1].timestamp;

      patterns.push_back(pattern);
    }
  }

  return patterns;
}

void MLErrorPatternAnalyzer::update_pattern_confidence(const std::string& pattern_id,
                                                       bool prediction_correct) {
  std::lock_guard<std::mutex> lock(model_mutex_);

  total_predictions_++;
  if (prediction_correct) {
    correct_predictions_++;
  }

  // Update pattern-specific confidence
  auto it = pattern_weights_.find(pattern_id);
  if (it != pattern_weights_.end()) {
    if (prediction_correct) {
      it->second = std::min(1.0, it->second * 1.1);  // Increase confidence
    } else {
      it->second = std::max(0.1, it->second * 0.9);  // Decrease confidence
    }
  }
}

void MLErrorPatternAnalyzer::save_model(const std::string& file_path) const {
  std::lock_guard<std::mutex> lock(model_mutex_);

  std::ofstream file(file_path);
  if (!file.is_open()) {
    return;
  }

  // Save model data (simplified format)
  file << "# ML Error Pattern Analyzer Model\n";
  file << "training_data_size=" << training_data_.size() << "\n";
  file << "correct_predictions=" << correct_predictions_ << "\n";
  file << "total_predictions=" << total_predictions_ << "\n";

  for (const auto& [pattern_id, weight] : pattern_weights_) {
    file << "pattern_weight," << pattern_id << "," << weight << "\n";
  }
}

void MLErrorPatternAnalyzer::load_model(const std::string& file_path) {
  std::lock_guard<std::mutex> lock(model_mutex_);

  std::ifstream file(file_path);
  if (!file.is_open()) {
    return;
  }

  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }

    // Parse model data (simplified implementation)
    if (line.find("pattern_weight,") == 0) {
      // Parse pattern weight line
      size_t first_comma = line.find(',');
      size_t second_comma = line.find(',', first_comma + 1);

      if (first_comma != std::string::npos && second_comma != std::string::npos) {
        std::string pattern_id = line.substr(first_comma + 1, second_comma - first_comma - 1);
        double weight = std::stod(line.substr(second_comma + 1));
        pattern_weights_[pattern_id] = weight;
      }
    }
  }
}

void MLErrorPatternAnalyzer::reset_model() {
  std::lock_guard<std::mutex> lock(model_mutex_);

  training_data_.clear();
  pattern_weights_.clear();
  correct_predictions_ = 0;
  total_predictions_ = 0;
  last_training_time_ = std::chrono::system_clock::time_point{};
}

double MLErrorPatternAnalyzer::get_prediction_accuracy() const {
  std::lock_guard<std::mutex> lock(model_mutex_);

  if (total_predictions_ == 0) {
    return 0.0;
  }

  return static_cast<double>(correct_predictions_) / total_predictions_;
}

size_t MLErrorPatternAnalyzer::get_training_data_size() const {
  std::lock_guard<std::mutex> lock(model_mutex_);
  return training_data_.size();
}

std::chrono::system_clock::time_point MLErrorPatternAnalyzer::get_last_training_time() const {
  std::lock_guard<std::mutex> lock(model_mutex_);
  return last_training_time_;
}

std::vector<double> MLErrorPatternAnalyzer::extract_features(
    const std::vector<DetailedError>& errors,
    const std::unordered_map<std::string, std::string>& context) const {
  std::vector<double> features;

  // Feature 1: Number of recent errors
  features.push_back(static_cast<double>(errors.size()));

  // Feature 2: Average error severity
  if (!errors.empty()) {
    double avg_severity = 0.0;
    for (const auto& error : errors) {
      avg_severity += static_cast<double>(error.severity);
    }
    features.push_back(avg_severity / errors.size());
  } else {
    features.push_back(0.0);
  }

  // Feature 3: Error category distribution
  std::unordered_map<ErrorCategory, size_t> category_counts;
  for (const auto& error : errors) {
    category_counts[error.category]++;
  }

  // Add features for each category
  for (int i = 0; i < 10; ++i) {  // Assuming 10 error categories
    ErrorCategory category = static_cast<ErrorCategory>(i);
    double ratio =
        errors.empty() ? 0.0 : static_cast<double>(category_counts[category]) / errors.size();
    features.push_back(ratio);
  }

  // Feature 4: Context factors
  features.push_back(context.size());  // Number of context factors

  return features;
}

double MLErrorPatternAnalyzer::calculate_error_probability(const std::vector<double>& features,
                                                           ErrorCode error_code) const {
  // Simple linear model for demonstration
  // In a real implementation, this would use a more sophisticated ML algorithm

  std::string code_key = std::to_string(static_cast<int>(error_code));
  auto it = pattern_weights_.find(code_key);
  double base_weight = it != pattern_weights_.end() ? it->second : 0.1;

  // Calculate weighted sum of features
  double probability = base_weight;
  for (size_t i = 0; i < features.size() && i < 5; ++i) {
    probability += features[i] * 0.1;  // Simple weighting
  }

  // Normalize to [0, 1]
  return std::max(0.0, std::min(1.0, probability));
}

void MLErrorPatternAnalyzer::update_weights(const std::vector<LearningDataPoint>& data) {
  // Simple weight update based on success patterns
  std::unordered_map<ErrorCode, double> success_rates;
  std::unordered_map<ErrorCode, size_t> total_attempts;

  for (const auto& point : data) {
    total_attempts[point.error.code]++;
    if (point.recovery_successful) {
      success_rates[point.error.code] += 1.0;
    }
  }

  // Update weights based on success rates
  for (const auto& [error_code, total] : total_attempts) {
    if (total > 0) {
      double success_rate = success_rates[error_code] / total;
      std::string code_key = std::to_string(static_cast<int>(error_code));
      pattern_weights_[code_key] = success_rate;
    }
  }
}

// ErrorRecoveryOrchestrator implementation
ErrorRecoveryOrchestrator& ErrorRecoveryOrchestrator::instance() {
  static ErrorRecoveryOrchestrator instance;
  return instance;
}

void ErrorRecoveryOrchestrator::initialize() {
  if (initialized_.load()) {
    return;
  }

  setup_default_workflows();
  setup_default_prevention_rules();
  setup_default_monitors();

  // Start monitoring systems
  EarlyDetectionSystem::instance().start_monitoring();

  initialized_.store(true);
}

void ErrorRecoveryOrchestrator::shutdown() {
  if (!initialized_.load()) {
    return;
  }

  EarlyDetectionSystem::instance().stop_monitoring();
  initialized_.store(false);
}

bool ErrorRecoveryOrchestrator::handle_error(const DetailedError& error) {
  if (!initialized_.load()) {
    initialize();
  }

  // Try prevention first
  std::vector<DetailedError> recent_errors = {error};
  ErrorPreventionSystem::instance().check_and_prevent_errors(recent_errors);

  // Attempt recovery based on mode
  switch (default_recovery_mode_) {
    case RecoveryMode::Automatic:
      return attempt_automatic_recovery(error);
    case RecoveryMode::SemiAutomatic:
    case RecoveryMode::Manual:
    case RecoveryMode::Interactive:
      return attempt_guided_recovery(error);
    default:
      return false;
  }
}

void ErrorRecoveryOrchestrator::configure_recovery_system(const std::string& ) {
  // Configuration loading would be implemented here
  // For now, use default configuration
}

void ErrorRecoveryOrchestrator::set_recovery_mode(RecoveryMode mode) {
  default_recovery_mode_ = mode;
}

void ErrorRecoveryOrchestrator::enable_learning(bool enabled) { learning_enabled_ = enabled; }

bool ErrorRecoveryOrchestrator::is_system_healthy() const {
  return EarlyDetectionSystem::instance().get_overall_health_score() > 0.7;
}

std::string ErrorRecoveryOrchestrator::generate_system_report() const {
  std::ostringstream oss;

  oss << "=== Error Recovery System Report ===\n";
  oss << "System Status: " << (is_system_healthy() ? "HEALTHY" : "DEGRADED") << "\n";
  oss << "Overall Health Score: " << std::fixed << std::setprecision(2)
      << (EarlyDetectionSystem::instance().get_overall_health_score() * 100) << "%\n";

  // Active recoveries
  auto active_recoveries = AdvancedErrorRecoveryManager::instance().get_active_recoveries();
  oss << "Active Recoveries: " << active_recoveries.size() << "\n";

  // Prevention statistics
  auto prevented_errors = ErrorPreventionSystem::instance().get_prevented_errors();
  oss << "Prevented Errors: " << prevented_errors.size() << "\n";

  // Detection statistics
  auto detected_issues = EarlyDetectionSystem::instance().get_detected_issues();
  oss << "Detected Issues: " << detected_issues.size() << "\n";

  // Learning statistics
  if (learning_enabled_) {
    auto& ml_analyzer = MLErrorPatternAnalyzer::instance();
    oss << "ML Model Accuracy: " << std::fixed << std::setprecision(1)
        << (ml_analyzer.get_prediction_accuracy() * 100) << "%\n";
    oss << "Training Data Points: " << ml_analyzer.get_training_data_size() << "\n";
  }

  return oss.str();
}

void ErrorRecoveryOrchestrator::export_recovery_data(const std::string& file_path) const {
  std::ofstream file(file_path);
  if (!file.is_open()) {
    return;
  }

  file << generate_system_report();

  // Export learning data if enabled
  if (learning_enabled_) {
    auto learning_data = AdvancedErrorRecoveryManager::instance().get_learning_data();
    file << "\n=== Learning Data ===\n";
    for (const auto& data_point : learning_data) {
      file << "Error: " << static_cast<int>(data_point.error.code)
           << ", Recovery: " << static_cast<int>(data_point.attempted_recovery.strategy)
           << ", Success: " << (data_point.recovery_successful ? "YES" : "NO")
           << ", Time: " << data_point.recovery_time.count() << "ms\n";
    }
  }
}

void ErrorRecoveryOrchestrator::setup_default_workflows() {
  // Setup default recovery workflows
  auto& recovery_manager = AdvancedErrorRecoveryManager::instance();

  // Network recovery workflow
  recovery_manager.register_recovery_workflow(
      ErrorRecoveryUtils::create_network_recovery_workflow());

  // Filesystem recovery workflow
  recovery_manager.register_recovery_workflow(
      ErrorRecoveryUtils::create_filesystem_recovery_workflow());

  // Memory recovery workflow
  recovery_manager.register_recovery_workflow(
      ErrorRecoveryUtils::create_memory_recovery_workflow());

  // Configuration recovery workflow
  recovery_manager.register_recovery_workflow(
      ErrorRecoveryUtils::create_configuration_recovery_workflow());
}

void ErrorRecoveryOrchestrator::setup_default_prevention_rules() {
  auto& prevention_system = ErrorPreventionSystem::instance();

  // Memory leak prevention
  prevention_system.register_prevention_rule(
      ErrorRecoveryUtils::create_memory_leak_prevention_rule());

  // Network timeout prevention
  prevention_system.register_prevention_rule(
      ErrorRecoveryUtils::create_network_timeout_prevention_rule());

  // Disk space prevention
  prevention_system.register_prevention_rule(
      ErrorRecoveryUtils::create_disk_space_prevention_rule());
}

void ErrorRecoveryOrchestrator::setup_default_monitors() {
  auto& detection_system = EarlyDetectionSystem::instance();

  // Memory usage monitor
  detection_system.register_monitor(ErrorRecoveryUtils::create_memory_usage_monitor());

  // Network health monitor
  detection_system.register_monitor(ErrorRecoveryUtils::create_network_health_monitor());

  // Filesystem health monitor
  detection_system.register_monitor(ErrorRecoveryUtils::create_filesystem_health_monitor());

  // Performance monitor
  detection_system.register_monitor(ErrorRecoveryUtils::create_performance_monitor());
}

bool ErrorRecoveryOrchestrator::attempt_automatic_recovery(const DetailedError& error) {
  return AdvancedErrorRecoveryManager::instance().execute_recovery_sync(error, "",
                                                                        RecoveryMode::Automatic);
}

bool ErrorRecoveryOrchestrator::attempt_guided_recovery(const DetailedError& error) {
  return AdvancedErrorRecoveryManager::instance().execute_recovery_sync(error, "",
                                                                        default_recovery_mode_);
}

}  // namespace SolarSystem::Utils

// ErrorRecoveryUtils implementation
namespace SolarSystem::Utils::ErrorRecoveryUtils {

RecoveryWorkflow create_network_recovery_workflow() {
  RecoveryWorkflow workflow;
  workflow.workflow_id = "network_recovery";
  workflow.name = "Network Error Recovery";
  workflow.description = "Handles network-related errors with retry and fallback strategies";
  workflow.mode = RecoveryMode::Automatic;
  workflow.total_timeout = std::chrono::seconds(120);

  // Step 1: Check network connectivity
  RecoveryStep connectivity_check;
  connectivity_check.step_id = "check_connectivity";
  connectivity_check.description = "Check network connectivity";
  connectivity_check.action = []() {
    // Simple connectivity check
    return NetworkUtils::is_endpoint_reachable("8.8.8.8", std::chrono::seconds(5));
  };
  connectivity_check.timeout = std::chrono::seconds(10);
  connectivity_check.max_retries = 1;
  workflow.steps.push_back(connectivity_check);

  // Step 2: Retry with exponential backoff
  RecoveryStep retry_step;
  retry_step.step_id = "retry_operation";
  retry_step.description = "Retry the failed network operation";
  retry_step.action = []() {
    // This would retry the original operation
    return true;  // Placeholder
  };
  retry_step.timeout = std::chrono::seconds(30);
  retry_step.max_retries = 3;
  workflow.steps.push_back(retry_step);

  // Step 3: Use fallback endpoint
  RecoveryStep fallback_step;
  fallback_step.step_id = "use_fallback";
  fallback_step.description = "Use fallback network endpoint";
  fallback_step.action = []() {
    // Switch to fallback endpoint
    return true;  // Placeholder
  };
  fallback_step.timeout = std::chrono::seconds(20);
  fallback_step.max_retries = 1;
  workflow.steps.push_back(fallback_step);

  return workflow;
}

RecoveryWorkflow create_filesystem_recovery_workflow() {
  RecoveryWorkflow workflow;
  workflow.workflow_id = "filesystem_recovery";
  workflow.name = "Filesystem Error Recovery";
  workflow.description = "Handles filesystem-related errors";
  workflow.mode = RecoveryMode::SemiAutomatic;
  workflow.total_timeout = std::chrono::seconds(180);

  // Step 1: Check disk space
  RecoveryStep disk_check;
  disk_check.step_id = "check_disk_space";
  disk_check.description = "Check available disk space";
  disk_check.action = []() {
    // Check disk space
    return std::filesystem::space("/").available > 1024 * 1024 * 100;  // 100MB minimum
  };
  disk_check.timeout = std::chrono::seconds(5);
  disk_check.max_retries = 1;
  workflow.steps.push_back(disk_check);

  // Step 2: Clean temporary files
  RecoveryStep cleanup_step;
  cleanup_step.step_id = "cleanup_temp_files";
  cleanup_step.description = "Clean up temporary files";
  cleanup_step.action = []() {
    // Clean temporary files
    FileResourceManager::instance().cleanup_temp_files();
    return true;
  };
  cleanup_step.timeout = std::chrono::seconds(30);
  cleanup_step.max_retries = 1;
  workflow.steps.push_back(cleanup_step);

  // Step 3: Retry operation
  RecoveryStep retry_step;
  retry_step.step_id = "retry_file_operation";
  retry_step.description = "Retry the failed file operation";
  retry_step.action = []() {
    // Retry the original file operation
    return true;  // Placeholder
  };
  retry_step.timeout = std::chrono::seconds(60);
  retry_step.max_retries = 2;
  workflow.steps.push_back(retry_step);

  return workflow;
}

RecoveryWorkflow create_memory_recovery_workflow() {
  RecoveryWorkflow workflow;
  workflow.workflow_id = "memory_recovery";
  workflow.name = "Memory Error Recovery";
  workflow.description = "Handles memory-related errors";
  workflow.mode = RecoveryMode::Automatic;
  workflow.total_timeout = std::chrono::seconds(60);

  // Step 1: Force garbage collection
  RecoveryStep gc_step;
  gc_step.step_id = "force_gc";
  gc_step.description = "Force garbage collection";
  gc_step.action = []() {
    // Force cleanup of managed resources
    ResourceManager::instance().cleanup_expired_resources();
    return true;
  };
  gc_step.timeout = std::chrono::seconds(10);
  gc_step.max_retries = 1;
  workflow.steps.push_back(gc_step);

  // Step 2: Reduce memory usage
  RecoveryStep reduce_step;
  reduce_step.step_id = "reduce_memory";
  reduce_step.description = "Reduce memory usage";
  reduce_step.action = []() {
    // Reduce memory usage by clearing caches
    return true;  // Placeholder
  };
  reduce_step.timeout = std::chrono::seconds(15);
  reduce_step.max_retries = 1;
  workflow.steps.push_back(reduce_step);

  return workflow;
}

RecoveryWorkflow create_configuration_recovery_workflow() {
  RecoveryWorkflow workflow;
  workflow.workflow_id = "configuration_recovery";
  workflow.name = "Configuration Error Recovery";
  workflow.description = "Handles configuration-related errors";
  workflow.mode = RecoveryMode::Interactive;
  workflow.total_timeout = std::chrono::seconds(300);

  // Step 1: Validate configuration
  RecoveryStep validate_step;
  validate_step.step_id = "validate_config";
  validate_step.description = "Validate configuration files";
  validate_step.action = []() {
    // Validate configuration
    return true;  // Placeholder
  };
  validate_step.timeout = std::chrono::seconds(10);
  validate_step.max_retries = 1;
  workflow.steps.push_back(validate_step);

  // Step 2: User interaction for config fix
  RecoveryStep user_fix_step;
  user_fix_step.step_id = "user_config_fix";
  user_fix_step.description = "Get user input for configuration fix";

  UserInteractionRequest config_request;
  config_request.id = "config_fix_choice";
  config_request.type = UserInteractionType::Selection;
  config_request.title = "Configuration Error Recovery";
  config_request.message = "How would you like to fix the configuration error?";
  config_request.options = {"Reset to defaults", "Edit manually", "Load backup", "Skip"};
  config_request.default_value = "Reset to defaults";
  config_request.timeout = std::chrono::seconds(60);

  user_fix_step.user_interactions.push_back(config_request);
  user_fix_step.action = []() {
    // Action based on user choice would be implemented here
    return true;
  };
  user_fix_step.timeout = std::chrono::seconds(120);
  user_fix_step.max_retries = 1;
  workflow.steps.push_back(user_fix_step);

  return workflow;
}

PreventionRule create_memory_leak_prevention_rule() {
  PreventionRule rule;
  rule.rule_id = "memory_leak_prevention";
  rule.name = "Memory Leak Prevention";
  rule.description = "Prevents memory leaks by monitoring resource usage";
  rule.target_category = ErrorCategory::Memory;
  rule.target_codes = {ErrorCode::OutOfMemory, ErrorCode::MemoryLeak};
  rule.confidence_threshold = 0.8;

  rule.condition = [](const DetailedError& ) {
    // Check if memory usage is trending upward
    auto stats = ResourceManager::instance().get_statistics();
    return stats.current_bytes_allocated > stats.peak_bytes_allocated * 0.9;
  };

  rule.prevention_action = []() {
    // Force cleanup of expired resources
    ResourceManager::instance().cleanup_expired_resources();
    return true;
  };

  return rule;
}

PreventionRule create_network_timeout_prevention_rule() {
  PreventionRule rule;
  rule.rule_id = "network_timeout_prevention";
  rule.name = "Network Timeout Prevention";
  rule.description = "Prevents network timeouts by monitoring connection health";
  rule.target_category = ErrorCategory::Network;
  rule.target_codes = {ErrorCode::ConnectionTimeout, ErrorCode::NetworkUnavailable};
  rule.confidence_threshold = 0.7;

  rule.condition = [](const DetailedError& ) {
    // Check network health
    return !NetworkUtils::is_endpoint_reachable("8.8.8.8", std::chrono::seconds(2));
  };

  rule.prevention_action = []() {
    // Cleanup expired network connections
    NetworkResourceManager::instance().cleanup_expired_connections();
    return true;
  };

  return rule;
}

PreventionRule create_disk_space_prevention_rule() {
  PreventionRule rule;
  rule.rule_id = "disk_space_prevention";
  rule.name = "Disk Space Prevention";
  rule.description = "Prevents disk space issues by monitoring usage";
  rule.target_category = ErrorCategory::FileSystem;
  rule.target_codes = {ErrorCode::DiskFull};
  rule.confidence_threshold = 0.9;

  rule.condition = [](const DetailedError& ) {
    // Check available disk space
    try {
      auto space_info = std::filesystem::space("/");
      double usage_ratio =
          static_cast<double>(space_info.capacity - space_info.available) / space_info.capacity;
      return usage_ratio > 0.9;  // 90% full
    } catch (...) {
      return false;
    }
  };

  rule.prevention_action = []() {
    // Clean up temporary files and caches
    FileResourceManager::instance().cleanup_temp_files();
    return true;
  };

  return rule;
}

EarlyDetectionMonitor create_memory_usage_monitor() {
  EarlyDetectionMonitor monitor;
  monitor.monitor_id = "memory_usage_monitor";
  monitor.name = "Memory Usage Monitor";
  monitor.description = "Monitors system memory usage";
  monitor.check_interval = std::chrono::seconds(30);
  monitor.warning_threshold = 0.8;
  monitor.critical_threshold = 0.95;

  monitor.health_check = []() {
    auto stats = ResourceManager::instance().get_statistics();
    if (stats.peak_bytes_allocated == 0) {
      return 1.0;  // No memory usage tracked
    }

    double usage_ratio =
        static_cast<double>(stats.current_bytes_allocated) / stats.peak_bytes_allocated;
    return 1.0 - usage_ratio;  // Higher usage = lower health
  };

  monitor.diagnostic = []() {
    std::vector<DetailedError> issues;
    auto leak_result = ResourceManager::instance().detect_leaks();

    if (leak_result.has_leaks) {
      DetailedError leak_issue(ErrorCode::MemoryLeak,
                               "Memory leaks detected: " + std::to_string(leak_result.leak_count),
                               ErrorSeverity::Warning);
      issues.push_back(leak_issue);
    }

    return issues;
  };

  return monitor;
}

EarlyDetectionMonitor create_network_health_monitor() {
  EarlyDetectionMonitor monitor;
  monitor.monitor_id = "network_health_monitor";
  monitor.name = "Network Health Monitor";
  monitor.description = "Monitors network connectivity and performance";
  monitor.check_interval = std::chrono::seconds(60);
  monitor.warning_threshold = 0.7;
  monitor.critical_threshold = 0.3;

  monitor.health_check = []() {
    // Test connectivity to multiple endpoints
    std::vector<std::string> test_endpoints = {"8.8.8.8", "1.1.1.1", "google.com"};
    size_t successful_connections = 0;

    for (const auto& endpoint : test_endpoints) {
      if (NetworkUtils::is_endpoint_reachable(endpoint, std::chrono::seconds(3))) {
        successful_connections++;
      }
    }

    return static_cast<double>(successful_connections) / test_endpoints.size();
  };

  monitor.diagnostic = []() {
    std::vector<DetailedError> issues;
    auto stats = NetworkResourceManager::instance().get_statistics();

    if (stats.connection_failures > stats.total_connections_created * 0.1) {
      DetailedError connection_issue(ErrorCode::ConnectionFailed,
                                     "High connection failure rate detected",
                                     ErrorSeverity::Warning);
      issues.push_back(connection_issue);
    }

    return issues;
  };

  return monitor;
}

EarlyDetectionMonitor create_filesystem_health_monitor() {
  EarlyDetectionMonitor monitor;
  monitor.monitor_id = "filesystem_health_monitor";
  monitor.name = "Filesystem Health Monitor";
  monitor.description = "Monitors filesystem health and disk space";
  monitor.check_interval = std::chrono::seconds(120);
  monitor.warning_threshold = 0.8;
  monitor.critical_threshold = 0.95;

  monitor.health_check = []() {
    try {
      auto space_info = std::filesystem::space("/");
      double available_ratio = static_cast<double>(space_info.available) / space_info.capacity;
      return available_ratio;  // More available space = better health
    } catch (...) {
      return 0.5;  // Unknown state
    }
  };

  monitor.diagnostic = []() {
    std::vector<DetailedError> issues;
    auto stats = FileResourceManager::instance().get_statistics();

    if (stats.failed_operations > stats.total_files_opened * 0.05) {
      DetailedError file_issue(ErrorCode::FileAccessDenied,
                               "High file operation failure rate detected", ErrorSeverity::Warning);
      issues.push_back(file_issue);
    }

    return issues;
  };

  return monitor;
}

EarlyDetectionMonitor create_performance_monitor() {
  EarlyDetectionMonitor monitor;
  monitor.monitor_id = "performance_monitor";
  monitor.name = "Performance Monitor";
  monitor.description = "Monitors overall system performance";
  monitor.check_interval = std::chrono::seconds(45);
  monitor.warning_threshold = 0.6;
  monitor.critical_threshold = 0.3;

  monitor.health_check = []() {
    // Simple performance check based on resource usage
    auto resource_stats = ResourceManager::instance().get_statistics();
    auto file_stats = FileResourceManager::instance().get_statistics();
    auto network_stats = NetworkResourceManager::instance().get_statistics();

    // Calculate performance score based on various factors
    double performance_score = 1.0;

    // Factor in resource usage
    if (resource_stats.current_allocations > resource_stats.peak_allocations * 0.8) {
      performance_score *= 0.8;
    }

    // Factor in file operation performance
    if (file_stats.failed_operations > 0) {
      double failure_rate = static_cast<double>(file_stats.failed_operations) /
                            std::max(1UL, file_stats.total_files_opened);
      performance_score *= (1.0 - failure_rate);
    }

    // Factor in network performance
    if (network_stats.connection_failures > 0) {
      double failure_rate = static_cast<double>(network_stats.connection_failures) /
                            std::max(1UL, network_stats.total_connections_created);
      performance_score *= (1.0 - failure_rate);
    }

    return std::max(0.0, std::min(1.0, performance_score));
  };

  monitor.diagnostic = []() {
    std::vector<DetailedError> issues;

    // Check for performance degradation indicators
    auto resource_stats = ResourceManager::instance().get_statistics();
    if (resource_stats.leak_count > 0) {
      DetailedError perf_issue(ErrorCode::PerformanceDegraded,
                               "Performance degradation due to resource leaks",
                               ErrorSeverity::Warning);
      issues.push_back(perf_issue);
    }

    return issues;
  };

  return monitor;
}

UserInteractionRequest create_confirmation_request(const std::string& message) {
  UserInteractionRequest request;
  request.id = "confirmation_" + std::to_string(std::hash<std::string>{}(message));
  request.type = UserInteractionType::Confirmation;
  request.title = "Confirmation Required";
  request.message = message;
  request.options = {"Yes", "No"};
  request.default_value = "No";
  request.timeout = std::chrono::seconds(30);
  return request;
}

UserInteractionRequest create_selection_request(const std::string& message,
                                                const std::vector<std::string>& options) {
  UserInteractionRequest request;
  request.id = "selection_" + std::to_string(std::hash<std::string>{}(message));
  request.type = UserInteractionType::Selection;
  request.title = "Selection Required";
  request.message = message;
  request.options = options;
  request.default_value = options.empty() ? "" : options[0];
  request.timeout = std::chrono::seconds(60);
  return request;
}

UserInteractionRequest create_input_request(const std::string& message,
                                            const std::string& default_value) {
  UserInteractionRequest request;
  request.id = "input_" + std::to_string(std::hash<std::string>{}(message));
  request.type = UserInteractionType::Input;
  request.title = "Input Required";
  request.message = message;
  request.default_value = default_value;
  request.timeout = std::chrono::seconds(120);
  return request;
}

std::function<bool()> create_retry_action(std::function<bool()> original_action,
                                          size_t max_retries) {
  return [original_action, max_retries]() {
    for (size_t attempt = 0; attempt < max_retries; ++attempt) {
      if (original_action()) {
        return true;
      }

      if (attempt < max_retries - 1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100 * (attempt + 1)));
      }
    }
    return false;
  };
}

std::function<bool()> create_fallback_action(std::function<bool()> primary_action,
                                             std::function<bool()> fallback_action) {
  return [primary_action, fallback_action]() {
    if (primary_action()) {
      return true;
    }
    return fallback_action();
  };
}

std::function<bool()> create_timeout_action(std::function<bool()> action,
                                            std::chrono::seconds timeout) {
  return [action, timeout]() {
    auto future = std::async(std::launch::async, action);
    return future.wait_for(timeout) == std::future_status::ready && future.get();
  };
}

}  // namespace SolarSystem::Utils::ErrorRecoveryUtils
