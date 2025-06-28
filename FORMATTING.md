# Code Formatting

This project uses `clang-format` to maintain consistent code formatting across all C++ source files.

## Setup

### Prerequisites
- `clang-format` must be installed and available in your PATH
- On macOS: `brew install clang-format`
- On Ubuntu/Debian: `apt-get install clang-format`

### Pre-commit Hook
A pre-commit hook is automatically installed that will:
- Format all staged C++ files before each commit
- Re-stage the formatted files
- Ensure consistent code style across the project

### Manual Formatting

#### Format All Files
```bash
./format-code.sh
```

#### Format Specific File
```bash
clang-format -i filename.cpp
```

#### Check Formatting (without modifying)
```bash
clang-format filename.cpp | diff filename.cpp -
```

## Configuration

The formatting rules are defined in `.clang-format` and are based on the Google C++ Style Guide with some customizations:

- **Indent Width**: 2 spaces
- **Column Limit**: 100 characters
- **Brace Style**: Attach (K&R style)
- **Pointer Alignment**: Left
- **Include Sorting**: Enabled

## Troubleshooting

### Pre-commit Hook Not Running
```bash
# Check if hook is executable
ls -la .git/hooks/pre-commit

# Make executable if needed
chmod +x .git/hooks/pre-commit
```

### Disable Hook Temporarily
```bash
git commit --no-verify -m "Commit message"
```

### Format Check in CI
```bash
# Check if all files are properly formatted
find . -name "*.cpp" -o -name "*.h" | xargs clang-format -output-replacements-xml | grep -c "<replacement " && echo "Code needs formatting" || echo "Code is properly formatted"
```
