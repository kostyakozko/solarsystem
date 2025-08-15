#!/bin/bash

# Performance Monitoring Test Runner
# Comprehensive performance testing with baseline management and regression detection

set -e

# Configuration
BUILD_DIR="${BUILD_DIR:-build}"
TEST_TIMEOUT="${TEST_TIMEOUT:-120}"
BASELINE_FILE="performance_baselines.txt"
REPORT_DIR="performance_reports"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}🚀 Solar System Suite - Performance Monitoring Test Runner${NC}"
echo "=============================================================="

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}❌ Build directory '$BUILD_DIR' not found${NC}"
    echo "Please run 'cmake -B $BUILD_DIR && cmake --build $BUILD_DIR -j8' first"
    exit 1
fi

# Create reports directory
mkdir -p "$REPORT_DIR"

# Function to run performance test with monitoring
run_performance_test() {
    local test_name="$1"
    local test_executable="$2"

    echo}🧪 Running: $test_name${NC}"

    # Set up environment
    export PERFORMANCE_BASELINE_FILE="$BASELINE_FILE"
    export PERFORMANCE_REPORT_DIR="$REPORT_DIR"

    # Run the test with timeout
    if timeout "$TEST_TIMEOUT" "$BUILD_DIR/tests/unit/$test_executable"; then
        echo -e "${GREEN}✅ $test_name PASSED${NC}"
        return 0
    else
        local exit_code=$?
        if [ $exit_code -eq 124 ]; then
            echo -e "${RED}❌ $test_name TIMEOUT (${TEST_TIMEOUT}s)${NC}"
        else
            echo -e "${RED}❌ $test_name FAILED (exit code: $exit_code)${NC}"
        fi
        return $exit_code
    fi
}

# Function to check for performance regressions
check_regressions() {
    echo -e "${YELLOW}📊 Checking for performance regressions...${NC}"

    if [ -f "$REPORT_DIR/performance_summary.txt" ]; then
        if grep -q "Regression Alerts:" "$REPORT_DIR/performance_summary.txt"; then
            echo -e "${YELLOW}⚠️  Performance regressions detected:${NC}"
            grep -A 10 "Regression Alerts:" "$REPORT_DIR/performance_summary.txt" | tail -n +2
            return 1
        else
            echo -e "${GREEN}✅ No performance regressions detected${NC}"
            return 0
        fi
    else
        echo -e "${YELLOW}⚠️  No performance summary found${NC}"
        return 0
    fi
}

# Function to generate performance report
generate_report() {
    echo -e "${BLUE}📊 Generating comprehensive performance report...${NC}"

    local report_file="$REPORT_DIR/comprehensive_performance_report.md"

    cat > "$report_file" << EOF
# Solar System Suite - Performance Monitoring Report

Generated: $(date)

## Test Summary

EOF

    # Add baseline information if available
    if [ -f "$BASELINE_FILE" ]; then
        echo "## Performance Baselines" >> "$report_file"
        echo "" >> "$report_file"
        echo "| Test Name | Baseline Time (ms) | CPU Usage (%) | Memory (KB) | Cache Miss Rate |" >> "$report_file"
        echo "|-----------|-------------------|---------------|-------------|-----------------|" >> "$report_file"

        while IFS=' ' read -r test_name time_ns cpu_percent memory_kb cache_miss_rate sample_count; do
            if [[ ! "$test_name" =~ ^# ]]; then
                time_ms=$(echo "scale=2; $time_ns / 1000000" | bc -l 2>/dev/null || echo "N/A")
                echo "| $test_name | $time_ms | $cpu_percent | $memory_kb | $cache_miss_rate |" >> "$report_file"
            fi
        done < "$BASELINE_FILE"

        echo "" >> "$report_file"
    fi

    # Add regression information if available
    if [ -f "$REPORT_DIR/performance_summary.txt" ]; then
        echo "## Performance Analysis" >> "$report_file"
        echo "" >> "$report_file"
        echo '```' >> "$report_file"
        cat "$REPORT_DIR/performance_summary.txt" >> "$report_file"
        echo '```' >> "$report_file"
        echo "" >> "$report_file"
    fi

    echo "## Test Execution Details" >> "$report_file"
    echo "" >> "$report_file"
    echo "- Build Directory: $BUILD_DIR" >> "$report_file"
    echo "- Test Timeout: ${TEST_TIMEOUT}s" >> "$report_file"
    echo "- Baseline File: $BASELINE_FILE" >> "$report_file"
    echo "- Report Directory: $REPORT_DIR" >> "$report_file"
    echo "" >> "$report_file"

    echo -e "${GREEN}📄 Report generated: $report_file${NC}"
}

# Main execution
main() {
    local failed_tests=0
    local total_tests=0

    echo -e "${BLUE}🔧 Setting up performance monitoring environment...${NC}"

    # Ensure we're in the right directory
    cd "$(dirname "$0")/../.."

    # Load existing baselines if available
    if [ -f "$BASELINE_FILE" ]; then
        echo -e "${GREEN}📈 Loading existing performance baselines from $BASELINE_FILE${NC}"
    else
        echo -e "${YELLOW}📈 No existing baselines found, will create new ones${NC}"
    fi

    echo -e "${BLUE}🧪 Running performance monitoring tests...${NC}"
    echo ""

    # Run comprehensive performance tests
    tests=(
        "PerformanceComprehensive:test_performance_comprehensive"
        "SimulationBuilderEnhanced:test_simulation_builder_enhanced"
        "DateTimeComprehensive:test_date_time_comprehensive"
    )

    for test_spec in "${tests[@]}"; do
        IFS=':' read -r test_name test_executable <<< "$test_spec"
        total_tests=$((total_tests + 1))

        if ! run_performance_test "$test_name" "$test_executable"; then
            failed_tests=$((failed_tests + 1))
        fi
        echo ""
    done

    # Check for regressions
    if ! check_regressions; then
        echo -e "${YELLOW}⚠️  Performance regressions detected but tests will continue${NC}"
    fi

    # Generate comprehensive report
    generate_report

    # Summary
    echo ""
    echo -e "${BLUE}📊 Performance Monitoring Summary${NC}"
    echo "=================================="
    echo "Total tests: $total_tests"
    echo "Passed: $((total_tests - failed_tests))"
    echo "Failed: $failed_tests"

    if [ $failed_tests -eq 0 ]; then
        echo -e "${GREEN}✅ All performance monitoring tests passed!${NC}"

        # Save baselines if all tests passed
        if [ -f "$BASELINE_FILE" ]; then
            cp "$BASELINE_FILE" "$REPORT_DIR/baselines_backup_$(date +%Y%m%d_%H%M%S).txt"
            echo -e "${GREEN}💾 Performance baselines backed up${NC}"
        fi

        return 0
    else
        echo -e "${RED}❌ $failed_tests performance monitoring test(s) failed${NC}"
        return 1
    fi
}

# Handle script arguments
case "${1:-}" in
    --help|-h)
        echo "Usage: $0 [options]"
        echo ""
        echo "Options:"
        echo "  --help, -h          Show this help message"
        echo "  --build-dir DIR     Set build directory (default: build)"
        echo "  --timeout SECONDS   Set test timeout (default: 120)"
        echo "  --baseline FILE     Set baseline file (default: performance_baselines.txt)"
        echo "  --report-dir DIR    Set report directory (default: performance_reports)"
        echo ""
        echo "Environment variables:"
        echo "  BUILD_DIR           Build directory path"
        echo "  TEST_TIMEOUT        Test timeout in seconds"
        exit 0
        ;;
    --build-dir)
        BUILD_DIR="$2"
        shift 2
        ;;
    --timeout)
        TEST_TIMEOUT="$2"
        shift 2
        ;;
    --baseline)
        BASELINE_FILE="$2"
        shift 2
        ;;
    --report-dir)
        REPORT_DIR="$2"
        shift 2
        ;;
    --*)
        echo -e "${RED}❌ Unknown option: $1${NC}"
        echo "Use --help for usage information"
        exit 1
        ;;
esac

# Run main function
main "$@"
