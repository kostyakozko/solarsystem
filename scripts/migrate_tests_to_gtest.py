#!/usr/bin/env python3
"""
Script to migrate test files from custom test framework to Google Test.

This script converts:
- #include "../utils/test_framework.h" -> #include <gtest/gtest.h>
- TestSuite suite("Name"); -> removed (Google Test handles this)
- suite.run_test("Test Name", []() { ... }); -> TEST(SuiteName, TestName) { ... }
- ASSERT_* macros remain compatible
- return suite.all_passed() ? 0 : 1; -> removed (gtest_main handles this)
"""

import re
import sys
import os
from pathlib import Path


def sanitize_test_name(name: str) -> str:
    """Convert test name to valid Google Test identifier."""
    # Remove quotes
    name = name.strip('"\'')
    # Replace spaces and special chars with underscores
    name = re.sub(r'[^a-zA-Z0-9_]', '_', name)
    # Remove consecutive underscores
    name = re.sub(r'_+', '_', name)
    # Remove leading/trailing underscores
    name = name.strip('_')
    # Ensure it starts with a letter
    if name and name[0].isdigit():
        name = 'Test_' + name
    return name or 'UnnamedTest'


def extract_suite_name(content: str) -> str:
    """Extract the test suite name from TestSuite constructor."""
    match = re.search(r'TestSuite\s+\w+\s*\(\s*"([^"]+)"', content)
    if match:
        return sanitize_test_name(match.group(1))
    return "TestSuite"


def convert_test_file(filepath: str) -> tuple[str, int]:
    """
    Convert a test file from custom framework to Google Test.
    Returns (converted_content, num_tests_converted).
    """
    with open(filepath, 'r') as f:
        content = f.read()

    # Check if already migrated
    if '#include <gtest/gtest.h>' in content:
        return content, 0

    # Check if it uses the custom framework
    if 'test_framework.h' not in content:
        return content, 0

    suite_name = extract_suite_name(content)

    # Replace include
    content = re.sub(
        r'#include\s*"\.\.\/utils\/test_framework\.h"',
        '#include <gtest/gtest.h>',
        content
    )

    # Also handle other include patterns
    content = re.sub(
        r'#include\s*"test_framework\.h"',
        '#include <gtest/gtest.h>',
        content
    )

    # Remove TestSuite declaration
    content = re.sub(
        r'TestSuite\s+\w+\s*\(\s*"[^"]+"\s*\)\s*;?\s*\n?',
        '',
        content
    )

    # Remove current_suite assignment
    content = re.sub(
        r'current_suite\s*=\s*&\w+\s*;?\s*\n?',
        '',
        content
    )

    # Count and convert suite.run_test patterns
    num_tests = 0

    def convert_run_test(match):
        nonlocal num_tests
        num_tests += 1
        test_name = sanitize_test_name(match.group(1))
        return f'TEST({suite_name}, {test_name})'

    # Pattern: suite.run_test("Name", []() {
    content = re.sub(
        r'\w+\.run_test\s*\(\s*"([^"]+)"\s*,\s*\[\s*\]\s*\(\s*\)\s*\{',
        convert_run_test,
        content
    )

    # Pattern: suite.run_test("Name", [&]() {
    content = re.sub(
        r'\w+\.run_test\s*\(\s*"([^"]+)"\s*,\s*\[\s*&\s*\]\s*\(\s*\)\s*\{',
        convert_run_test,
        content
    )

    # Remove closing }); for run_test lambdas - this is tricky
    # We need to find the matching closing brace
    # For now, replace }); at end of test blocks with just }
    content = re.sub(r'\}\s*\)\s*;(\s*\n\s*(?:TEST\(|$|//|/\*|int\s+main))', r'}\n\1', content)

    # Remove main function that returns suite result
    content = re.sub(
        r'int\s+main\s*\(\s*\)\s*\{[^}]*return\s+\w+\.all_passed\s*\(\s*\)[^}]*\}',
        '',
        content
    )

    # Also remove simpler main patterns
    content = re.sub(
        r'int\s+main\s*\(\s*\)\s*\{[^}]*return\s+\w+\.get_failed_count\s*\(\s*\)[^}]*\}',
        '',
        content
    )

    # Clean up extra blank lines
    content = re.sub(r'\n{3,}', '\n\n', content)

    return content, num_tests


def update_cmake_entry(cmake_path: str, test_name: str) -> bool:
    """Update CMakeLists.txt to use Google Test for the specified test."""
    with open(cmake_path, 'r') as f:
        content = f.read()

    # Pattern to find the test entry
    # add_executable(test_name test_name.cpp)
    # target_link_libraries(test_name test_utils ...)
    # add_test(NAME ... COMMAND test_name)
    # set_tests_properties(...)

    # Find and replace target_link_libraries to use GTest
    pattern = rf'(add_executable\s*\(\s*{test_name}\s+[^)]+\)\s*\n)target_link_libraries\s*\(\s*{test_name}\s+test_utils([^)]*)\)'

    def replace_link(match):
        exe_line = match.group(1)
        other_libs = match.group(2).strip()
        # Remove test_utils, add GTest
        libs = [lib.strip() for lib in other_libs.split() if lib.strip() and lib.strip() != 'test_utils']
        libs_str = ' '.join(libs) if libs else ''
        return f'{exe_line}target_link_libraries({test_name} GTest::gtest_main{" " + libs_str if libs_str else ""})'

    new_content = re.sub(pattern, replace_link, content)

    # Replace add_test with gtest_discover_tests
    pattern = rf'add_test\s*\(\s*NAME\s+\w+\s+COMMAND\s+{test_name}\s*\)\s*\n\s*set_tests_properties\s*\([^)]+\)'
    new_content = re.sub(
        pattern,
        f'gtest_discover_tests({test_name} DISCOVERY_TIMEOUT 30)',
        new_content
    )

    if new_content != content:
        with open(cmake_path, 'w') as f:
            f.write(new_content)
        return True
    return False


def main():
    if len(sys.argv) < 2:
        print("Usage: migrate_tests_to_gtest.py <test_file.cpp> [--cmake <CMakeLists.txt>]")
        print("       migrate_tests_to_gtest.py --all <tests_dir>")
        sys.exit(1)

    if sys.argv[1] == '--all':
        tests_dir = sys.argv[2] if len(sys.argv) > 2 else 'tests'
        test_files = list(Path(tests_dir).rglob('test_*.cpp'))

        total_converted = 0
        total_tests = 0

        for test_file in test_files:
            content, num_tests = convert_test_file(str(test_file))
            if num_tests > 0:
                with open(test_file, 'w') as f:
                    f.write(content)
                print(f"Converted {test_file}: {num_tests} tests")
                total_converted += 1
                total_tests += num_tests

        print(f"\nTotal: {total_converted} files, {total_tests} tests converted")
    else:
        filepath = sys.argv[1]
        content, num_tests = convert_test_file(filepath)

        if num_tests > 0:
            with open(filepath, 'w') as f:
                f.write(content)
            print(f"Converted {filepath}: {num_tests} tests")

            # Update CMake if specified
            if '--cmake' in sys.argv:
                cmake_idx = sys.argv.index('--cmake')
                if cmake_idx + 1 < len(sys.argv):
                    cmake_path = sys.argv[cmake_idx + 1]
                    test_name = Path(filepath).stem
                    if update_cmake_entry(cmake_path, test_name):
                        print(f"Updated CMake entry for {test_name}")
        else:
            print(f"No conversion needed for {filepath}")


if __name__ == '__main__':
    main()
