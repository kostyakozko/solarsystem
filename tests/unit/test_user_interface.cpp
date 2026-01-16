/**
 * @file test_user_interface.cpp
 * @brief Unit tests for user-friendly interface components
 * @note Migrated to Google Test
 */

#include <gtest/gtest.h>

#include "solar_core/ui/user_interface.hpp"

using namespace SolarSystem::UI;
TEST(UserInterfaceTests, Progress_Indicator_Creation) {
  ProgressIndicator progress("Test Task", 100);

  if (progress.get_percentage() != 0.0) throw std::runtime_error("Initial percentage should be 0");
  if (progress.is_complete()) throw std::runtime_error("Should not be complete initially");
}
TEST(UserInterfaceTests, Progress_Indicator_Update) {
  ProgressIndicator progress("Test Task", 100);

  progress.update(50);
  if (progress.get_percentage() != 50.0) throw std::runtime_error("Percentage should be 50");

  progress.update(75, "Processing...");
  if (progress.get_percentage() != 75.0) throw std::runtime_error("Percentage should be 75");
}
TEST(UserInterfaceTests, Progress_Indicator_Completion) {
  ProgressIndicator progress("Test Task", 100);

  progress.complete();
  if (!progress.is_complete()) throw std::runtime_error("Should be complete");
  if (progress.get_percentage() != 100.0) throw std::runtime_error("Percentage should be 100");
}
TEST(UserInterfaceTests, Progress_Indicator_Rendering) {
  ProgressIndicator progress("Test Task", 100);
  progress.update(50);

  std::string rendered = progress.render();
  if (rendered.empty()) throw std::runtime_error("Rendered output is empty");
  if (rendered.find("Test Task") == std::string::npos)
    throw std::runtime_error("Task name not in output");
}
TEST(UserInterfaceTests, Progress_Styles) {
  ProgressIndicator progress("Test Task", 100);
  progress.update(50);

  progress.set_style(ProgressStyle::BAR);
  std::string bar = progress.render();
  if (bar.empty()) throw std::runtime_error("Bar style output is empty");

  progress.set_style(ProgressStyle::PERCENTAGE);
  std::string pct = progress.render();
  if (pct.empty()) throw std::runtime_error("Percentage style output is empty");

  progress.set_style(ProgressStyle::MINIMAL);
  std::string minimal = progress.render();
  if (minimal.empty()) throw std::runtime_error("Minimal style output is empty");
}
TEST(UserInterfaceTests, Status_Feedback) {
  auto& status = StatusFeedback::instance();
  status.clear();

  status.info("Info message");
  status.success("Success message");
  status.warning("Warning message");
  status.error("Error message");

  auto messages = status.get_messages();
  if (messages.size() != 4) throw std::runtime_error("Should have 4 messages");
  if (messages[0].level != StatusLevel::INFO)
    throw std::runtime_error("First message should be INFO");
  if (messages[1].level != StatusLevel::SUCCESS)
    throw std::runtime_error("Second message should be SUCCESS");
}
TEST(UserInterfaceTests, Status_Message_Formatting) {
  auto& status = StatusFeedback::instance();
  status.clear();

  status.info("Test info");
  auto messages = status.get_messages();

  std::string formatted = status.format_message(messages[0]);
  if (formatted.empty()) throw std::runtime_error("Formatted message is empty");
  if (formatted.find("Test info") == std::string::npos)
    throw std::runtime_error("Message text not in output");
}
TEST(UserInterfaceTests, Interactive_Input) {
  auto& input = InteractiveInput::instance();

  input.set_interactive_mode(false);
  if (input.is_interactive()) throw std::runtime_error("Should not be interactive");

  InputPrompt prompt;
  prompt.prompt_text = "Enter value:";
  prompt.default_value = "default";
  prompt.required = false;

  std::string result = input.prompt(prompt);
  if (result != "default") throw std::runtime_error("Should return default value");
}
TEST(UserInterfaceTests, Accessibility_Support) {
  auto& accessibility = AccessibilitySupport::instance();

  accessibility.enable_screen_reader_mode(true);
  if (!accessibility.is_screen_reader_enabled())
    throw std::runtime_error("Screen reader should be enabled");

  accessibility.enable_high_contrast(true);
  if (!accessibility.is_high_contrast_enabled())
    throw std::runtime_error("High contrast should be enabled");

  accessibility.set_text_scale(1.5);
  if (accessibility.get_text_scale() != 1.5) throw std::runtime_error("Text scale mismatch");
}
TEST(UserInterfaceTests, Alt_Text) {
  auto& accessibility = AccessibilitySupport::instance();

  accessibility.set_alt_text("element1", "Description of element 1");
  std::string alt = accessibility.get_alt_text("element1");

  if (alt != "Description of element 1") throw std::runtime_error("Alt text mismatch");
}
TEST(UserInterfaceTests, Format_Table) {
  std::vector<std::string> headers = {"Name", "Value", "Status"};
  std::vector<std::vector<std::string>> data = {{"Item1", "100", "OK"}, {"Item2", "200", "OK"}};

  std::string table = FormattingUtils::format_table(data, headers);
  if (table.empty()) throw std::runtime_error("Table output is empty");
  if (table.find("Name") == std::string::npos) throw std::runtime_error("Header not in table");
}
TEST(UserInterfaceTests, Format_List) {
  std::vector<std::string> items = {"Item 1", "Item 2", "Item 3"};

  std::string bulleted = FormattingUtils::format_list(items, false);
  if (bulleted.empty()) throw std::runtime_error("Bulleted list is empty");

  std::string numbered = FormattingUtils::format_list(items, true);
  if (numbered.empty()) throw std::runtime_error("Numbered list is empty");
  if (numbered.find("1.") == std::string::npos) throw std::runtime_error("Numbering not in list");
}
TEST(UserInterfaceTests, Format_Box) {
  std::string box = FormattingUtils::format_box("Test content", "Test Title");
  if (box.empty()) throw std::runtime_error("Box output is empty");
  if (box.find("Test Title") == std::string::npos) throw std::runtime_error("Title not in box");
}
TEST(UserInterfaceTests, Text_Formatting) {
  std::string bold = FormattingUtils::bold("Bold text");
  if (bold.empty()) throw std::runtime_error("Bold output is empty");

  std::string colored = FormattingUtils::colorize("Red text", "red");
  if (colored.empty()) throw std::runtime_error("Colored output is empty");
}
