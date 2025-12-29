#!/bin/bash
# Bulk migrate test files from custom framework to Google Test
# This script does the basic transformations, manual review may be needed

set -e

# Function to convert a single test file
convert_test_file() {
    local file="$1"
    local suite_name="$2"

    echo "Converting: $file"

    # Create backup
    cp "$file" "${file}.bak"

    # Replace include
    sed -i '' 's|#include "../utils/test_framework.h"|#include <gtest/gtest.h>|g' "$file"
    sed -i '' 's|#include "test_framework.h"|#include <gtest/gtest.h>|g' "$file"

    # Remove TestSuite declaration
    sed -i '' '/TestSuite.*suite.*(/d' "$file"

    # Remove current_suite assignment
    sed -i '' '/current_suite.*=.*&/d' "$file"

    # Convert suite.run_test to TEST macro
    # This is a simplified conversion - complex cases need manual review
    sed -i '' "s/suite\.run_test(\"\([^\"]*\)\", \[\]() {/TEST(${suite_name}, \1) {/g" "$file"
    sed -i '' "s/suite\.run_test(\"\([^\"]*\)\", \[&\]() {/TEST(${suite_name}, \1) {/g" "$file"

    # Convert ASSERT_EQ to EXPECT_EQ (non-fatal is usually better)
    # Keep ASSERT_* for critical checks that should stop the test

    # Remove closing }); and replace with }
    # This is tricky - we need to be careful
    sed -i '' 's/});$/}/g' "$file"

    # Remove main function
    sed -i '' '/int main()/,/^}/d' "$file"

    # Clean up test names (replace spaces with underscores)
    # This needs to be done carefully

    echo "Converted: $file (backup at ${file}.bak)"
}

# Process all test files in a directory
process_directory() {
    local dir="$1"

    for file in "$dir"/test_*.cpp; do
        if [ -f "$file" ]; then
            # Check if already migrated
            if grep -q "gtest/gtest.h" "$file"; then
                echo "Already migrated: $file"
                continue
            fi

            # Check if uses custom framework
            if ! grep -q "test_framework.h" "$file"; then
                echo "Skipping (no custom framework): $file"
                continue
            fi

            # Extract suite name from file
            local basename=$(basename "$file" .cpp)
            local suite_name=$(echo "$basename" | sed 's/test_//' | sed 's/_/ /g' | awk '{for(i=1;i<=NF;i++) $i=toupper(substr($i,1,1)) tolower(substr($i,2))}1' | sed 's/ //g')

            convert_test_file "$file" "$suite_name"
        fi
    done
}

# Main
if [ "$1" == "--help" ]; then
    echo "Usage: $0 [directory]"
    echo "Converts test files from custom framework to Google Test"
    exit 0
fi

DIR="${1:-tests/unit}"
process_directory "$DIR"

echo "Done! Please review the converted files and fix any issues."
echo "Backups are saved with .bak extension."
