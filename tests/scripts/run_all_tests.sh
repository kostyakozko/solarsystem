#!/bin/bash

# Solar System Suite - Comprehensive Test Runner
# Runs all tests in the correct order with proper reporting

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BUILD_DIR="build"
TEST_TIMEOUT=300
VERBOSE=false
BENCHMARK=false
COVERAGE=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -b|--benchmark)
            BENCHMARK=true
            shift
            ;;
        -c|--coverage)
            COVERAGE=true
            shift
            ;;
        -t|--timeout)
            TEST_TIMEOUT="$2"
            shift 2
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  -v, --verbose     Enable verbose output"
            echo "  -b, --benchmark   Run performance benchmarks"
            echo "  -c, --coverage    Generate coverage report"
            echo "  -t, --timeout N   Set test timeout to N seconds"
            echo "  --build-dir DIR   Use DIR as build directory"
            echo "  -h, --help        Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to run tests with proper error handling
run_test_category() {
    local category=$1
    local description=$2
    
    print_status "Running $description..."
    
    if [ "$VERBOSE" = true ]; then
        ctest -L "$category" --output-on-failure --timeout "$TEST_TIMEOUT" --verbose
    else
        ctest -L "$category" --output-on-failure --timeout "$TEST_TIMEOUT"
    fi
    
    local exit_code=$?
    if [ $exit_code -eq 0 ]; then
        print_success "$description completed successfully"
    else
        print_error "$description failed with exit code $exit_code"
        return $exit_code
    fi
}

# Main execution
main() {
    print_status "Solar System Suite - Comprehensive Test Runner"
    print_status "=============================================="
    
    # Check if build directory exists
    if [ ! -d "$BUILD_DIR" ]; then
        print_error "Build directory '$BUILD_DIR' not found!"
        print_status "Please run 'cmake -B $BUILD_DIR && cmake --build $BUILD_DIR' first"
        exit 1
    fi
    
    # Change to build directory
    cd "$BUILD_DIR"
    
    # Check if tests are built
    if [ ! -f "tests/unit/test_solar_core" ]; then
        print_error "Tests not found! Please build with -DENABLE_TESTING=ON"
        exit 1
    fi
    
    # Initialize test results
    local total_tests=0
    local passed_tests=0
    local failed_tests=0
    
    print_status "Starting test execution..."
    echo
    
    # 1. Unit Tests
    print_status "Phase 1: Unit Tests"
    print_status "==================="
    if run_test_category "unit" "Unit Tests"; then
        ((passed_tests++))
    else
        ((failed_tests++))
    fi
    ((total_tests++))
    echo
    
    # 2. Integration Tests
    print_status "Phase 2: Integration Tests"
    print_status "=========================="
    if run_test_category "integration" "Integration Tests"; then
        ((passed_tests++))
    else
        ((failed_tests++))
    fi
    ((total_tests++))
    echo
    
    # 3. Performance Benchmarks (optional)
    if [ "$BENCHMARK" = true ]; then
        print_status "Phase 3: Performance Benchmarks"
        print_status "==============================="
        if run_test_category "benchmark" "Performance Benchmarks"; then
            ((passed_tests++))
        else
            print_warning "Benchmarks failed - this may be due to system load"
            ((failed_tests++))
        fi
        ((total_tests++))
        echo
    fi
    
    # 4. Generate coverage report (optional)
    if [ "$COVERAGE" = true ]; then
        print_status "Phase 4: Coverage Analysis"
        print_status "=========================="
        
        if command -v lcov >/dev/null 2>&1; then
            lcov --directory . --capture --output-file coverage.info
            lcov --remove coverage.info '/usr/*' --output-file coverage.info
            lcov --list coverage.info
            
            if command -v genhtml >/dev/null 2>&1; then
                genhtml coverage.info --output-directory coverage_html
                print_success "Coverage report generated in coverage_html/"
            fi
        else
            print_warning "lcov not found - skipping coverage analysis"
        fi
        echo
    fi
    
    # 5. Test Summary
    print_status "Test Execution Summary"
    print_status "====================="
    echo "Total test categories: $total_tests"
    echo "Passed: $passed_tests"
    echo "Failed: $failed_tests"
    
    if [ $failed_tests -eq 0 ]; then
        print_success "All tests passed! ✅"
        
        # Additional validation
        print_status "Running additional validation..."
        
        # Check installation
        if [ -f "../install/solar_system_launcher" ]; then
            print_success "Installation verified"
        else
            print_warning "Installation not found - run 'make install'"
        fi
        
        # Check documentation
        if [ -d "../docs/api/html" ]; then
            print_success "Documentation generated"
        else
            print_warning "Documentation not found - run 'doxygen docs/Doxyfile'"
        fi
        
        echo
        print_success "🎉 Solar System Suite is ready for production!"
        
    else
        print_error "Some tests failed! ❌"
        echo
        print_status "Failed test categories:"
        
        # Re-run failed tests with verbose output for debugging
        if [ $failed_tests -gt 0 ]; then
            print_status "Re-running failed tests with verbose output..."
            ctest --rerun-failed --output-on-failure --verbose
        fi
        
        exit 1
    fi
}

# Trap to ensure we return to original directory
trap 'cd - >/dev/null 2>&1' EXIT

# Run main function
main "$@"
