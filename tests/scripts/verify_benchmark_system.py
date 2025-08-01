#!/usr/bin/env python3
"""
Verification script for CI/CD benchmark system
Validates that all required components are working correctly
"""

import os
import sys
import subprocess
import csv
from pathlib import Path

def run_command(cmd, cwd=None, timeout=60):
    """Run a command and return result"""
    try:
        result = subprocess.run(
            cmd, shell=True, capture_output=True, text=True,
            cwd=cwd, timeout=timeout
        )
        return result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return -1, "", f"Command timed out after {timeout}s"
    except Exception as e:
        return -1, "", str(e)

def verify_test_discovery():
    """Verify that ctest can discover tests with proper labels"""
    print("🔍 Verifying test discovery...")

    test_categories = [
        ("unit", 60),
        ("integration", 180),
        ("benchmark", 300)
    ]

    for category, timeout in test_categories:
        # Use -N to show tests without running them (equivalent to --dry-run)
        cmd = f'ctest -L "{category}" -N'
        returncode, stdout, stderr = run_command(cmd, cwd="build")

        if returncode == 0:
            # Count discovered tests
            test_count = stdout.count("Test #")
            print(f"  ✅ {category}: {test_count} tests discovered")
        else:
            print(f"  ❌ {category}: Failed to discover tests")
            print(f"     Error: {stderr}")
            return False

    return True

def verify_installation_commands():
    """Verify that installation test commands work"""
    print("🔧 Verifying installation commands...")

    commands = [
        ("./solar_system_launcher --status", "install"),
        ("./bin/solar_system --help", "install"),
        ("./bin/solar_system_fetch --test-storage", "install")
    ]

    for cmd, cwd in commands:
        returncode, stdout, stderr = run_command(cmd, cwd=cwd, timeout=30)

        if returncode == 0:
            print(f"  ✅ {cmd}: Working")
        else:
            print(f"  ❌ {cmd}: Failed")
            print(f"     Error: {stderr}")
            return False

    return True

def verify_benchmark_csv_output():
    """Verify that benchmarks generate CSV output in correct format"""
    print("📊 Verifying benchmark CSV output...")

    csv_files = [
        "build/tests/benchmarks/benchmark_results/comprehensive_benchmark.csv",
        "build/tests/benchmarks/benchmark_results/core_performance_benchmark.csv",
        "build/tests/benchmarks/benchmark_results/jpl_data_benchmark.csv"
    ]

    expected_headers = [
        "Name", "AvgDuration(ms)", "MinDuration(ms)", "MaxDuration(ms)",
        "StdDev(ms)", "Iterations", "OpsPerSec", "MemoryUsage(bytes)"
    ]

    for csv_file in csv_files:
        if not Path(csv_file).exists():
            print(f"  ❌ {csv_file}: File not found")
            return False

        try:
            with open(csv_file, 'r') as f:
                reader = csv.reader(f)
                headers = next(reader)

                if headers == expected_headers:
                    row_count = sum(1 for _ in reader)
                    print(f"  ✅ {Path(csv_file).name}: {row_count} benchmarks")
                else:
                    print(f"  ❌ {Path(csv_file).name}: Invalid headers")
                    print(f"     Expected: {expected_headers}")
                    print(f"     Got: {headers}")
                    return False
        except Exception as e:
            print(f"  ❌ {csv_file}: Error reading file - {e}")
            return False

    return True

def verify_performance_comparison():
    """Verify that performance comparison script works"""
    print("📈 Verifying performance comparison...")

    baseline_file = "baseline_performance/simulation_performance.csv"
    current_file = "build/tests/benchmarks/benchmark_results/comprehensive_benchmark.csv"

    if not Path(baseline_file).exists():
        print(f"  ❌ Baseline file not found: {baseline_file}")
        return False

    if not Path(current_file).exists():
        print(f"  ❌ Current file not found: {current_file}")
        return False

    cmd = f"python3 tests/scripts/compare_performance.py {baseline_file} {current_file}"
    returncode, stdout, stderr = run_command(cmd, timeout=30)

    if returncode in [0, 1]:  # 0 = no regression, 1 = regression found (both valid)
        print("  ✅ Performance comparison script working")
        return True
    else:
        print("  ❌ Performance comparison script failed")
        print(f"     Error: {stderr}")
        return False

def verify_code_quality():
    """Verify code quality checks"""
    print("🔍 Verifying code quality...")

    # Check code formatting
    cmd = 'find lib apps \\( -name "*.cpp" -o -name "*.h" \\) -print0 | xargs -0 clang-format --dry-run --Werror'
    returncode, stdout, stderr = run_command(cmd, timeout=60)

    if returncode == 0:
        print("  ✅ Code formatting: Passed")
    else:
        print("  ❌ Code formatting: Failed")
        print(f"     Error: {stderr}")
        return False

    return True

def verify_test_artifacts():
    """Verify that test artifacts are in expected locations"""
    print("📁 Verifying test artifacts...")

    required_dirs = [
        "build/Testing",
        "build/tests/benchmark_results"
    ]

    for dir_path in required_dirs:
        if Path(dir_path).exists():
            file_count = len(list(Path(dir_path).iterdir()))
            print(f"  ✅ {dir_path}: {file_count} files")
        else:
            print(f"  ❌ {dir_path}: Directory not found")
            return False

    return True

def main():
    print("🚀 Solar System Suite CI/CD Verification")
    print("=" * 50)

    checks = [
        ("Test Discovery", verify_test_discovery),
        ("Installation Commands", verify_installation_commands),
        ("Benchmark CSV Output", verify_benchmark_csv_output),
        ("Performance Comparison", verify_performance_comparison),
        ("Code Quality", verify_code_quality),
        ("Test Artifacts", verify_test_artifacts)
    ]

    passed = 0
    total = len(checks)

    for name, check_func in checks:
        print(f"\n{name}:")
        if check_func():
            passed += 1
        else:
            print(f"❌ {name} failed")

    print("\n" + "=" * 50)
    print(f"Results: {passed}/{total} checks passed")

    if passed == total:
        print("✅ All CI/CD verification checks passed!")
        return 0
    else:
        print("❌ Some CI/CD verification checks failed")
        return 1

if __name__ == '__main__':
    sys.exit(main())
