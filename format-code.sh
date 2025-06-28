#!/bin/bash

# Script to format all C++ source files with clang-format

echo "Formatting all C++ source files..."

# Find all C++ files and format them
find . -name "*.cpp" -o -name "*.h" -o -name "*.cc" -o -name "*.cxx" -o -name "*.hpp" | while read -r file; do
    # Skip files in .git directory
    if [[ "$file" == *".git"* ]]; then
        continue
    fi
    
    echo "Formatting: $file"
    clang-format -i "$file"
done

echo "✓ Code formatting completed for all C++ files."
echo ""
echo "You can now commit the formatted code:"
echo "  git add ."
echo "  git commit -m 'Format code with clang-format'"
