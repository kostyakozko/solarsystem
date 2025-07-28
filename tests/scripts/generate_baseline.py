#!/usr/bin/env python3
"""
Baseline Performance Data Generator for Solar System Suite
Generates comprehensive baseline performance data for regression testing.
"""

import os
import sys
import subprocess
import shutil
import json
from datetime import datetime
from pathlib import Path

def ensure_directory_exists(path):
    """Create directory if it doesn't exist."""
    Path(path).mkdir(parents=True, exist_ok=True)

def run_benchmark(executable_path, output_dir):
    """Run a benchmark executable and return success status."""
    try:
        print(f"Running benchmark: {executable_path}")
        result = subprocess.run([executable_path],
                              cwd=output_dir,
                              capture_output=True,
                              text=True,
                              timeout=300)

        if result.returncode == 0:
            print(f"  ✅ Success: {executable_path}")
            return True
        else:
            print(f"  ❌ Failed: {executable_path}")
            print(f"  Error: {result.stderr}")
            return False
    except subprocess.TimeoutExpired:
        print(f"  ⏰ Timeout: {executable_path}")
        return False
    except Exception as e:
        print(f"  💥 Exception: {executable_path} - {e}")
        return False

def generate_baseline_data(build_dir="build", baseline_dir="baseline_performance"):
    """Generate comprehensive baseline performance data."""

    print("=" * 80)
    print("SOLAR SYSTEM SUITE - BASELINE PERFORMANCE DATA GENERATION")
    print("=" * 80)
    print(f"Build directory: {build_dir}")
    print(f"Baseline directory: {baseline_dir}")
    print()

    # Ensure directories exist
    benchmark_dir = os.path.join(build_dir, "tests", "benchmarks")
    results_dir = os.path.join(benchmark_dir, "benchmark_results")
    ensure_directory_exists(results_dir)
    ensure_directory_exists(baseline_dir)

    # List of benchmark executables
    benchmarks = [
        "benchmark_comprehensive",
        "benchmark_jpl_data",
        "benchmark_web_server",
        "scalability_tests",
        "regression_detector"
    ]

    successful_benchmarks = []
    failed_benchmarks = []

    # Run all benchmarks
    for benchmark in benchmarks:
        executable_path = os.path.join(benchmark_dir, benchmark)

        if os.path.exists(executable_path):
            if run_benchmark(executable_path, benchmark_dir):
                successful_benchmarks.append(benchmark)
            else:
                failed_benchmarks.append(benchmark)
        else:
            print(f"  ⚠️  Not found: {executable_path}")
            failed_benchmarks.append(benchmark)

    print()
    print("=" * 80)
    print("BENCHMARK EXECUTION SUMMARY")
    print("=" * 80)
    print(f"Successful: {len(successful_benchmarks)}")
    print(f"Failed: {len(failed_benchmarks)}")

    if successful_benchmarks:
        print("\n✅ Successful benchmarks:")
        for benchmark in successful_benchmarks:
            print(f"  - {benchmark}")

    if failed_benchmarks:
        print("\n❌ Failed benchmarks:")
        for benchmark in failed_benchmarks:
            print(f"  - {benchmark}")

    # Copy CSV files to baseline directory
    csv_files = []
    for file in os.listdir(results_dir):
        if file.endswith('.csv'):
            src_path = os.path.join(results_dir, file)
            dst_path = os.path.join(baseline_dir, file)
            shutil.copy2(src_path, dst_path)
            csv_files.append(file)
            print(f"📋 Copied baseline: {file}")

    # Generate metadata
    metadata = {
        "generation_date": datetime.now().isoformat(),
        "build_directory": build_dir,
        "successful_benchmarks": successful_benchmarks,
        "failed_benchmarks": failed_benchmarks,
        "csv_files": csv_files,
        "total_benchmarks": len(benchmarks),
        "success_rate": len(successful_benchmarks) / len(benchmarks) * 100
    }

    metadata_path = os.path.join(baseline_dir, "baseline_metadata.json")
    with open(metadata_path, 'w') as f:
        json.dump(metadata, f, indent=2)

    print(f"\n📊 Baseline metadata saved: {metadata_path}")
    print(f"📈 Success rate: {metadata['success_rate']:.1f}%")

    # Generate combined baseline file for compare_performance.py
    combined_baseline = os.path.join(baseline_dir, "combined_baseline.csv")
    generate_combined_baseline(results_dir, combined_baseline)

    print(f"🔗 Combined baseline: {combined_baseline}")
    print()
    print("=" * 80)
    print("BASELINE GENERATION COMPLETE")
    print("=" * 80)

    return len(failed_benchmarks) == 0

def generate_combined_baseline(results_dir, output_file):
    """Generate a combined baseline CSV file from all benchmark results."""
    import csv

    combined_data = []

    # Read all CSV files in results directory
    for file in os.listdir(results_dir):
        if file.endswith('.csv'):
            csv_path = os.path.join(results_dir, file)
            try:
                with open(csv_path, 'r') as f:
                    reader = csv.DictReader(f)
                    for row in reader:
                        combined_data.append(row)
            except Exception as e:
                print(f"Warning: Could not read {csv_path}: {e}")

    # Write combined CSV
    if combined_data:
        with open(output_file, 'w', newline='') as f:
            fieldnames = ['Name', 'AvgDuration(ms)', 'MinDuration(ms)', 'MaxDuration(ms)',
                         'StdDev(ms)', 'Iterations', 'OpsPerSec', 'MemoryUsage(bytes)']
            writer = csv.DictWriter(f, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(combined_data)

        print(f"📊 Combined {len(combined_data)} benchmark results")
    else:
        print("⚠️  No benchmark data found to combine")

def validate_csv_format(csv_file):
    """Validate that CSV file matches expected format for compare_performance.py."""
    expected_columns = [
        'Name', 'AvgDuration(ms)', 'MinDuration(ms)', 'MaxDuration(ms)',
        'StdDev(ms)', 'Iterations', 'OpsPerSec', 'MemoryUsage(bytes)'
    ]

    try:
        import csv
        with open(csv_file, 'r') as f:
            reader = csv.DictReader(f)
            actual_columns = reader.fieldnames

            if actual_columns == expected_columns:
                print(f"✅ CSV format valid: {csv_file}")
                return True
            else:
                print(f"❌ CSV format invalid: {csv_file}")
                print(f"  Expected: {expected_columns}")
                print(f"  Actual: {actual_columns}")
                return False
    except Exception as e:
        print(f"❌ Error validating {csv_file}: {e}")
        return False

def main():
    import argparse

    parser = argparse.ArgumentParser(description='Generate baseline performance data')
    parser.add_argument('--build-dir', default='build',
                       help='Build directory (default: build)')
    parser.add_argument('--baseline-dir', default='baseline_performance',
                       help='Baseline output directory (default: baseline_performance)')
    parser.add_argument('--validate', action='store_true',
                       help='Validate CSV format compatibility')

    args = parser.parse_args()

    success = generate_baseline_data(args.build_dir, args.baseline_dir)

    if args.validate:
        print("\n" + "=" * 80)
        print("CSV FORMAT VALIDATION")
        print("=" * 80)

        results_dir = os.path.join(args.build_dir, "tests", "benchmarks", "benchmark_results")
        all_valid = True

        for file in os.listdir(results_dir):
            if file.endswith('.csv'):
                csv_path = os.path.join(results_dir, file)
                if not validate_csv_format(csv_path):
                    all_valid = False

        if all_valid:
            print("✅ All CSV files have valid format")
        else:
            print("❌ Some CSV files have invalid format")
            success = False

    return 0 if success else 1

if __name__ == '__main__':
    sys.exit(main())
