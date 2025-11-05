#!/bin/bash
# Test runner script for local CI testing
# Matches GitHub Actions CI test execution exactly

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "╭─────────────────────────────────────────────────────────╮"
echo "│   Solar System Suite - Local CI Test Runner            │"
echo "╰─────────────────────────────────────────────────────────╯"
echo ""

# Configuration matching GitHub Actions CI
BUILD_TYPE="${BUILD_TYPE:-Release}"
INSTALL_PREFIX="/workspace/install"
BUILD_DIR="/workspace/build"

echo "Configuration:"
echo "  Build Type: $BUILD_TYPE"
echo "  Install Prefix: $INSTALL_PREFIX"
echo "  Build Directory: $BUILD_DIR"
echo "  Compiler: $CC / $CXX"
echo ""

# Step 1: Configure CMake (matching CI)
echo -e "${BLUE}[1/6]${NC} Configuring CMake..."
cmake -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
  -DENABLE_TESTING=ON \
  -DENABLE_PROFILING=OFF

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓${NC} CMake configuration successful"
else
    echo -e "${RED}✗${NC} CMake configuration failed"
    exit 1
fi
echo ""

# Step 2: Build (matching CI)
echo -e "${BLUE}[2/6]${NC} Building project..."
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -j"$(nproc)"

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓${NC} Build successful"
else
    echo -e "${RED}✗${NC} Build failed"
    exit 1
fi
echo ""

# Step 3: Install (matching CI)
echo -e "${BLUE}[3/6]${NC} Installing..."
cmake --build "$BUILD_DIR" --target install

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓${NC} Installation successful"
else
    echo -e "${RED}✗${NC} Installation failed"
    exit 1
fi
echo ""

# Step 4: Run Unit Tests (matching CI)
echo -e "${BLUE}[4/6]${NC} Running Unit Tests..."
cd "$BUILD_DIR"
ctest -L "unit" --output-on-failure --timeout 60

UNIT_RESULT=$?
if [ $UNIT_RESULT -eq 0 ]; then
    echo -e "${GREEN}✓${NC} Unit tests passed"
else
    echo -e "${RED}✗${NC} Unit tests failed"
fi
echo ""

# Step 5: Run Integration Tests (matching CI)
echo -e "${BLUE}[5/6]${NC} Running Integration Tests..."
ctest -L "integration" --output-on-failure --timeout 180

INTEGRATION_RESULT=$?
if [ $INTEGRATION_RESULT -eq 0 ]; then
    echo -e "${GREEN}✓${NC} Integration tests passed"
else
    echo -e "${RED}✗${NC} Integration tests failed"
fi
echo ""

# Step 6: Run Performance Benchmarks (matching CI)
echo -e "${BLUE}[6/6]${NC} Running Performance Benchmarks..."
ctest -L "benchmark" --output-on-failure --timeout 300

BENCHMARK_RESULT=$?
if [ $BENCHMARK_RESULT -eq 0 ]; then
    echo -e "${GREEN}✓${NC} Benchmarks passed"
else
    echo -e "${YELLOW}⚠${NC} Benchmarks failed (may vary by system)"
    # Don't fail on benchmark errors (matching CI: continue-on-error: true)
    BENCHMARK_RESULT=0
fi
echo ""

# Summary
echo "╭─────────────────────────────────────────────────────────╮"
echo "│   Test Execution Summary                                │"
echo "╰─────────────────────────────────────────────────────────╯"
echo ""

if [ $UNIT_RESULT -eq 0 ] && [ $INTEGRATION_RESULT -eq 0 ] && [ $BENCHMARK_RESULT -eq 0 ]; then
    echo -e "${GREEN}✓ All tests passed!${NC}"
    echo ""
    echo "Your changes are ready to push to GitHub."
    exit 0
else
    echo -e "${RED}✗ Some tests failed${NC}"
    echo ""
    echo "Unit Tests: $([ $UNIT_RESULT -eq 0 ] && echo -e "${GREEN}PASS${NC}" || echo -e "${RED}FAIL${NC}")"
    echo "Integration Tests: $([ $INTEGRATION_RESULT -eq 0 ] && echo -e "${GREEN}PASS${NC}" || echo -e "${RED}FAIL${NC}")"
    echo "Benchmarks: $([ $BENCHMARK_RESULT -eq 0 ] && echo -e "${GREEN}PASS${NC}" || echo -e "${YELLOW}WARN${NC}")"
    echo ""
    echo "Please fix the failing tests before pushing."
    exit 1
fi
