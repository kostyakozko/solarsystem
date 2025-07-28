#!/usr/bin/env python3
"""
Benchmark System Verification Script
Verifies that the complete benchmark system is working correctly.
"""

import os
import sys
import subprocess
import csv
from pathlib import Path

def verify_directory_structure():
    """Verify that all required directories exist."""
    required_dirs = [
        "build/tests/benchmarks/benchmark_results",
        "baseline_performance",
        "benchmark_archives"
    ]

    print("🔍 Verifying directory structure...")
    all_exist = True

    for directory in required_dirs:
        if os.path.exists(directory):
            print(f"  ✅ {directory}")
        else:
            print(f"  ❌ {directory} - MISSING")
            all_exist = False

    return all_exist

def verify_csv_files():
    """Verify that CSV files exist and have correct format."""
    results_dir = "build/tests/benchmarks/benchmark_results"
    expected_files = [
        "comprehensive_benchmark.csv",
        "jpl_data_benchmark.csv",
        "web_server_benchmark.csv",
        "scalability_analysis.csv",
        "regression_detection.csv"
    ]

    expected_columns = [
        'Name', 'AvgDuration(ms)', 'MinDuration(ms)', 'MaxDuration(ms)',
        'StdDev(ms)', 'Iterations', 'OpsPerSec', 'MemoryUsage(bytes)'
    ]

    print("\n📊 Verifying CSV files...")
    all_valid = True

    for filename in expected_files:
        filepath = os.path.join(results_dir, filename)

        if not os.path.exists(filepath):
            print(f"  ❌ {filename} - MISSING")
            all_valid = False
            continue

        try:
            with open(filepath, 'r') as f:
                reader = csv.DictReader(f)
                columns = reader.fieldnames

                if columns == expected_columns:
                    # Count rows to verify data exists
                    row_count = sum(1 for row in reader)
                    print(f"  ✅ {filename} - {row_count} benchmarks")
                else:
                    print(f"  ❌ {filename} - Invalid columns: {columns}")
                    all_valid = False
        except Exception as e:
            print(f"  ❌ {filename} - Error reading: {e}")
            all_valid = False

    return all_valid

def verify_baseline_files():
    """Verify that baseline files exist and are valid."""
    baseline_dir = "baseline_performance"
    expected_files = [
        "combined_baseline.csv",
        "sample_baseline.csv",
        "baseline_metadata.json"
    ]

    print("\n📈 Verifying baseline files...")
    all_exist = True

    for filename in expected_files:
        filepath = os.path.join(baseline_dir, filename)

        if os.path.exists(filepath):
            print(f"  ✅ {filename}")
        else:
            print(f"  ❌ {filename} - MISSING")
            all_exist = False

    return all_exist

def verify_ctest_integration():
    """Verify that CTest can find and run benchmark tests."""
    print("\n🧪 Verifying CTest integration...")

    try:
        # Check if benchmark tests are discoverable
        result = subprocess.run(
            ['ctest', '-L', 'benchmark', '-N'],
            cwd='build',
            capture_output=True,
            text=True,
            timeout=30
        )

        if result.returncode == 0:
            # Count number of benchmark tests found
            lines = result.stdout.split('\n')
            test_count = 0
            for line in lines:
                if 'Test #' in line and 'Benchmark_' in line:
                    test_count += 1

            print(f"  ✅ CTest found {test_count} benchmark tests")
            return test_count > 0
        else:
            print(f"  ❌ CTest failed: {result.stderr}")
            return False

    except Exception as e:
        print(f"  ❌ CTest error: {e}")
        return False

def verify_performance_comparison():
    """Verify that performance comparison script works."""
    print("\n📊 Verifying performance comparison...")

    baseline_file = "baseline_performance/sample_baseline.csv"
    current_file = "build/tests/benchmarks/benchmark_results/jpl_data_benchmark.csv"

    if not os.path.exists(baseline_file):
        print(f"  ❌ Baseline file missing: {baseline_file}")
        return False

    if not os.path.exists(current_file):
        print(f"  ❌ Current file missing: {current_file}")
        return False

    try:
        # Test normal output
        result = subprocess.run(
            ['python3', 'tests/scripts/compare_performance.py', baseline_file, current_file],
            capture_output=True,
            text=True,
            timeout=30
        )

        if result.returncode == 0:
            print("  ✅ Performance comparison (normal output)")
        else:
            print(f"  ❌ Performance comparison failed: {result.stderr}")
            return False

        # Test JSON output
        result = subprocess.run(
            ['python3', 'tests/scripts/compare_performance.py', '--json', baseline_file, current_file],
            capture_output=True,
            text=True,
            timeout=30
        )

        if result.returncode == 0:
            # Verify JSON is valid
            import json
            try:
                json.loads(result.stdout.strip())
                print("  ✅ Performance comparison (JSON output)")
                return True
            except json.JSONDecodeError as e:
                print(f"  ❌ Invalid JSON output: {e}")
                return False
            else:
                print("  ❌ No JSON found in output")
                return False
        else:
            print(f"  ❌ JSON comparison failed: {result.stderr}")
            return False

    except Exception as e:
        print(f"  ❌ Comparison error: {e}")
        return False

def verify_scripts_executable():
    """Verify that all scripts are executable."""
    scripts = [
        "tests/scripts/compare_performance.py",
        "tests/scripts/generate_baseline.py",
        "tests/scripts/setup_benchmark_environment.py"
    ]

    print("\n🔧 Verifying script permissions...")
    all_executable = True

    for script in scripts:
        if os.path.exists(script) and os.access(script, os.X_OK):
            print(f"  ✅ {script}")
        else:
            print(f"  ❌ {script} - Not executable")
            all_executable = False

    return all_executable

def main():
    print("=" * 80)
    print("SOLAR SYSTEM SUITE - BENCHMARK SYSTEM VERIFICATION")
    print("=" * 80)

    checks = [
        ("Directory Structure", verify_directory_structure),
        ("CSV Files", verify_csv_files),
        ("Baseline Files", verify_baseline_files),
        ("CTest Integration", verify_ctest_integration),
        ("Performance Comparison", verify_performance_comparison),
        ("Script Permissions", verify_scripts_executable)
    ]

    passed_checks = 0
    total_checks = len(checks)

    for check_name, check_func in checks:
        try:
            if check_func():
                passed_checks += 1
        except Exception as e:
            print(f"\n❌ {check_name} check failed with exception: {e}")

    print("\n" + "=" * 80)
    print("VERIFICATION SUMMARY")
    print("=" * 80)
    print(f"Passed: {passed_checks}/{total_checks} checks")
    print(f"Success rate: {passed_checks/total_checks*100:.1f}%")

    if passed_checks == total_checks:
        print("\n✅ ALL CHECKS PASSED - Benchmark system is fully functional!")
        print("\nThe benchmark system provides:")
        print("  • CSV output compatible with compare_performance.py")
        print("  • Proper directory structure (build/tests/benchmark_results/)")
        print("  • Sample baseline data for regression testing")
        print("  • CTest integration with 'benchmark' label")
        print("  • Performance regression detection")
        print("  • JSON output for CI/CD integration")
    else:
        print(f"\n❌ {total_checks - passed_checks} CHECKS FAILED")
        print("Please review the errors above and fix the issues.")

    print("=" * 80)

    return 0 if passed_checks == total_checks else 1

if __name__ == '__main__':
    sys.exit(main())
