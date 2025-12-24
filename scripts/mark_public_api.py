#!/usr/bin/env python3
"""
Script to automatically mark public API with export macros
"""

import re
import sys
from pathlib import Path

def add_export_macro_to_class(content, library_name):
    """Add export macro to class declarations"""
    macro = f"{library_name.upper()}_API"

    # Pattern: class ClassName {
    # Replace with: class LIBRARY_API ClassName {
    pattern = r'(^class\s+)(?!' + macro + r')(\w+)'
    replacement = r'\1' + macro + r' \2'

    content = re.sub(pattern, replacement, content, flags=re.MULTILINE)

    # Pattern: struct StructName {
    pattern = r'(^struct\s+)(?!' + macro + r')(\w+)'
    content = re.sub(pattern, replacement, content, flags=re.MULTILINE)

    return content

def add_export_macro_to_functions(content, library_name):
    """Add export macro to function declarations"""
    macro = f"{library_name.upper()}_API"

    # Pattern: [[nodiscard]] ReturnType function_name(
    # Replace with: [[nodiscard]] LIBRARY_API ReturnType function_name(
    pattern = r'(\[\[nodiscard\]\]\s+)(?!' + macro + r')(\w+)'
    replacement = r'\1' + macro + r' \2'

    content = re.sub(pattern, replacement, content, flags=re.MULTILINE)

    return content

def add_export_include(content, library_name):
    """Add export header include if not present"""
    export_include = f'#include "{library_name}/export.hpp"'

    if export_include in content:
        return content

    # Find the last #include and add after it
    lines = content.split('\n')
    last_include_idx = -1

    for i, line in enumerate(lines):
        if line.strip().startswith('#include'):
            last_include_idx = i

    if last_include_idx >= 0:
        lines.insert(last_include_idx + 1, export_include)
        return '\n'.join(lines)

    return content

def process_header(filepath, library_name):
    """Process a single header file"""
    try:
        with open(filepath, 'r') as f:
            content = f.read()

        original = content

        # Add export include
        content = add_export_include(content, library_name)

        # Mark classes and structs
        content = add_export_macro_to_class(content, library_name)

        # Mark functions
        content = add_export_macro_to_functions(content, library_name)

        if content != original:
            with open(filepath, 'w') as f:
                f.write(content)
            return True

        return False
    except Exception as e:
        print(f"Error processing {filepath}: {e}", file=sys.stderr)
        return False

def main():
    if len(sys.argv) < 2:
        print("Usage: mark_public_api.py <library_name>")
        print("Example: mark_public_api.py solar_core")
        sys.exit(1)

    library_name = sys.argv[1]
    include_dir = Path(f"lib/{library_name}/include/{library_name}")

    if not include_dir.exists():
        print(f"Error: {include_dir} does not exist")
        sys.exit(1)

    headers = list(include_dir.rglob("*.hpp"))
    modified = 0

    for header in headers:
        if process_header(header, library_name):
            print(f"Modified: {header}")
            modified += 1

    print(f"\nProcessed {len(headers)} headers, modified {modified}")

if __name__ == "__main__":
    main()
