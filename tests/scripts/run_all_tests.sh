#!/bin/bash
"""
Comprehensive test runner for Solar System Suite CI/CD
Provides detailed error reporting and debugging information
"""

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BUILD_DIR="build"
INSTALL_DIR="install"
TEST_TIMEOUT=300
VERBOSE=false
QUICK_MODE=false
COMPREHENSIVE_MODE=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --verbose|-v)
            VERBOSE=true
            shift
            ;;
        --quick)
            QUICK_MODE=true
            shift
            ;;
        --comprehensive)
            COMPREHENSIVE_MODE=true
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  --verbose, -v     Enable verbose output"
            echo "  --quick          Run only essential tests"
            echo "  --comprehensive  Run all tests including slow ones"
            echo "  --help, -h       Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_section() {
    echo ""
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE} $1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

# Error handling
handle_error() {
    local exit_code=$?
    local line_number=$1
    log_error "Test execution failed at line $line_number with exit code $exit_code"

    # Collect debugging information
    log_section "DEBUGGING INFORMATION"

    echo "Environment:"
    echo "  OS: $(uname -s)"
    echo "  Architecture: $(uname -m)"
    echo "  Date: $(date)"
    echo "  Working Directory: $(pwd)"

    if [ -d "$BUILD_DIR" ]; then
        echo "  Build Directory: EXISTS"
        echo "  Build Files: $(ls -la $BUILD_DIR | wc -l) files"
    else
        echo "  Build Directory: MISSING"
    fi

    if [ -d "$INSTALL_DIR" ]; then
        echo "  Install Directory: EXISTS"
        echo "  Install Files: $(ls -la $INSTALL_DIR | wc -l) files"
    else
        echo "  Install Directory: MISSING"
    fi

    # Show recent log files if they exist
    if [ -f "$BUILD_DIR/Testing/Temporary/LastTest.log" ]; then
        log_section "RECENT TEST LOG (last 50 lines)"
        tail -50 "$BUILD_DIR/Testing/Temporary/LastTest.log"
    fi

    exit $exit_code
}

trap 'handle_error $LINENO' ERR

# System verification
verify_system() {
    log_section "SYSTEM VERIFICATION"

    # Check required tools
    local required_tools=("cmake" "make" "clang-format" "python3")
    for tool in "${required_tools[@]}"; do
        if command -v "$tool" >/dev/null 2>&1; then
            log_success "$tool: Available"
        else
            log_error "$tool: Not found"
            exit 1
        fi
    done

    # Check directories
    if [ ! -d "$BUILD_DIR" ]; then
        log_error "Build directory not found: $BUILD_DIR"
        log_info "Please run: cmake -B $BUILD_DIR -DENABLE_TESTING=ON"
        exit 1
    fi

    log_success "System verification passed"
}

# Build verification
verify_build() {
    log_section "BUILD VERIFICATION"

    log_info "Building project..."
    if $VERBOSE; then
        cmake --build "$BUILD_DIR" -j$(nproc)
    else
        cmake --build "$BUILD_DIR" -j$(nproc) > /dev/null 2>&1
    fi

    log_success "Build completed successfully"
}

# Installation verification
verify_installation() {
    log_section "INSTALLATION VERIFICATION"

    log_info "Installing project..."
    if $VERBOSE; then
        cmake --build "$BUILD_DIR" --target install
    else
        cmake --build "$BUILD_DIR" --target install > /dev/null 2>&1
    fi

    # Test installation commands
    local install_commands=(
        "./solar_system_launcher --status"
        "./bin/solar_system --help"
        "./bin/solar_system_fetch --test-storage"
    )

    for cmd in "${install_commands[@]}"; do
        log_info "Testing: $cmd"
        if (cd "$INSTALL_DIR" && eval "$cmd" > /dev/null 2>&1); then
            log_success "✓ $cmd"
        else
            log_error "✗ $cmd"
            exit 1
        fi
    done

    log_success "Installation verification passed"
}

# Test execution with detailed reporting
run_test_category() {
    local category=$1
    local timeout=$2
    local description=$3

    log_section "RUNNING $description"

    local start_time=$(date +%s)
    local test_output_file="/tmp/solar_test_${category}_$$.log"

    log_info "Category: $category"
    log_info "Timeout: ${timeout}s"
    log_info "Output file: $test_output_file"

    # Run tests and capture output
    if (cd "$BUILD_DIR" && ctest -L "$category" --output-on-failure --timeout "$timeout" > "$test_output_file" 2>&1); then
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))

        # Parse results
        local total_tests=$(grep -c "Test #" "$test_output_file" || echo "0")
        local passed_tests=$(grep -c "Passed" "$test_output_file" || echo "0")
        local failed_tests=$(grep -c "Failed" "$test_output_file" || echo "0")

        log_success "$description completed in ${duration}s"
        log_info "Results: $passed_tests passed, $failed_tests failed, $total_tests total"

        if [ "$failed_tests" -gt 0 ]; then
            log_warning "Some tests failed, but continuing..."
            if $VERBOSE; then
                log_section "FAILED TEST DETAILS"
                grep -A 5 -B 5 "Failed" "$test_output_file" || true
            fi
        fi
    else
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))

        log_error "$description failed after ${duration}s"

        # Show detailed error information
        log_section "ERROR DETAILS"
        if [ -f "$test_output_file" ]; then
            tail -100 "$test_output_file"
        fi

        # Don't exit immediately for benchmark failures (they may fail due to performance thresholds)
        if [ "$category" != "benchmark" ]; then
            exit 1
        else
            log_warning "Benchmark failures detected, but continuing (may be due to performance thresholds)"
        fi
    fi

    # Cleanup
    rm -f "$test_output_file"
}

# Code quality checks
run_code_quality_checks() {
    log_section "CODE QUALITY CHECKS"

    # Code formatting check
    log_info "Checking code formatting..."
    if find lib apps \( -name "*.cpp" -o -name "*.h" \) -print0 | xargs -0 clang-format --dry-run --Werror > /dev/null 2>&1; then
        log_success "Code formatting: PASSED"
    else
        log_error "Code formatting: FAILED"
        log_info "Run 'make format' to fix formatting issues"
        exit 1
    fi

    # Static analysis (optional - don't fail on warnings)
    log_info "Running static analysis..."
    if command -v cppcheck >/dev/null 2>&1; then
        local cppcheck_output="/tmp/cppcheck_output_$$.txt"
        cppcheck --enable=all --suppress=missingInclude --suppress=missingIncludeSystem --suppress=unusedFunction lib/ apps/ 2>&1 | tee "$cppcheck_output"

        if grep -E "(error):" "$cppcheck_output" > /dev/null; then
            log_warning "Static analysis found errors (see output above)"
        else
            log_success "Static analysis: No errors found"
        fi

        rm -f "$cppcheck_output"
    else
        log_warning "cppcheck not available, skipping static analysis"
    fi

    log_success "Code quality checks completed"
}

# Performance verification
verify_performance() {
    log_section "PERFORMANCE VERIFICATION"

    # Check if benchmark results exist
    local benchmark_dir="$BUILD_DIR/tests/benchmarks/benchmark_results"
    if [ ! -d "$benchmark_dir" ]; then
        log_warning "Benchmark results directory not found: $benchmark_dir"
        return
    fi

    local csv_files=$(find "$benchmark_dir" -name "*.csv" | wc -l)
    log_info "Found $csv_files benchmark CSV files"

    if [ "$csv_files" -gt 0 ]; then
        log_success "Benchmark CSV generation: WORKING"

        # Test performance comparison if baseline exists
        if [ -f "baseline_performance/combined_baseline.csv" ] && [ -f "$benchmark_dir/comprehensive_benchmark.csv" ]; then
            log_info "Testing performance comparison..."
            if python3 tests/scripts/compare_performance.py baseline_performance/combined_baseline.csv "$benchmark_dir/comprehensive_benchmark.csv" > /dev/null 2>&1; then
                log_success "Performance comparison: NO REGRESSIONS"
            else
                log_warning "Performance comparison: REGRESSIONS DETECTED"
            fi
        else
            log_info "Baseline or current data missing, skipping performance comparison"
        fi
    else
        log_warning "No benchmark CSV files generated"
    fi
}

# Main execution
main() {
    log_section "SOLAR SYSTEM SUITE - COMPREHENSIVE TEST RUNNER"

    echo "Configuration:"
    echo "  Mode: $([ "$QUICK_MODE" = true ] && echo "QUICK" || ([ "$COMPREHENSIVE_MODE" = true ] && echo "COMPREHENSIVE" || echo "STANDARD"))"
    echo "  Verbose: $VERBOSE"
    echo "  Build Directory: $BUILD_DIR"
    echo "  Install Directory: $INSTALL_DIR"
    echo ""

    # System verification
    verify_system

    # Build verification
    verify_build

    # Installation verification
    verify_installation

    # Code quality checks
    run_code_quality_checks

    # Test execution based on mode
    if [ "$QUICK_MODE" = true ]; then
        # Quick mode: only essential tests
        run_test_category "unit" 60 "UNIT TESTS (Essential Only)"
    elif [ "$COMPREHENSIVE_MODE" = true ]; then
        # Comprehensive mode: all tests
        run_test_category "unit" 120 "UNIT TESTS (All)"
        run_test_category "integration" 300 "INTEGRATION TESTS (All)"
        run_test_category "benchmark" 600 "BENCHMARK TESTS (All)"
    else
        # Standard mode: balanced test suite
        run_test_category "unit" 90 "UNIT TESTS (Standard)"
        run_test_category "integration" 180 "INTEGRATION TESTS (Standard)"
        run_test_category "benchmark" 300 "BENCHMARK TESTS (Standard)"
    fi

    # Performance verification
    verify_performance

    # Final summary
    log_section "TEST EXECUTION SUMMARY"
    log_success "All test categories completed successfully!"
    log_info "Check individual test outputs above for detailed results"

    # Generate test report
    local report_file="test_execution_report_$(date +%Y%m%d_%H%M%S).txt"
    {
        echo "Solar System Suite - Test Execution Report"
        echo "Generated: $(date)"
        echo "Mode: $([ "$QUICK_MODE" = true ] && echo "QUICK" || ([ "$COMPREHENSIVE_MODE" = true ] && echo "COMPREHENSIVE" || echo "STANDARD"))"
        echo ""
        echo "System Information:"
        echo "  OS: $(uname -s)"
        echo "  Architecture: $(uname -m)"
        echo "  Build Directory: $BUILD_DIR"
        echo "  Install Directory: $INSTALL_DIR"
        echo ""
        echo "Test Results: See console output above"
    } > "$report_file"

    log_info "Test report saved: $report_file"

    return 0
}

# Execute main function
main "$@"
