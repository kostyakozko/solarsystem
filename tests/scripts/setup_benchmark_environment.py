#!/usr/bin/env python3
"""
Setup script for benchmark environment in CI/CD
Ensures all required directories and baseline files exist
"""

import os
import sys
import json
import csv
from pathlib import Path

def create_benchmark_directories():
    """Create required benchmark directories"""
    dirs = [
        "build/Testing",
        "build/tests/benchmark_results",
        "baseline_performance"
    ]

    for dir_path in dirs:
        Path(dir_path).mkdir(parents=True, exist_ok=True)
        print(f"✅ Created directory: {dir_path}")

def create_sample_baseline():
    """Create sample baseline performance data for CI"""
    baseline_data = [
        {
            "Name": "SimulationStepMicrosecondBenchmark",
            "AvgDuration(ms)": 0.001,
            "MinDuration(ms)": 0.0008,
            "MaxDuration(ms)": 0.0015,
            "StdDev(ms)": 0.0002,
            "Iterations": 50000,
            "OpsPerSec": 1000000.0,
            "MemoryUsage(bytes)": 1048576
        },
        {
            "Name": "CacheLoadingPerformance",
            "AvgDuration(ms)": 0.1,
            "MinDuration(ms)": 0.08,
            "MaxDuration(ms)": 0.15,
            "StdDev(ms)": 0.02,
            "Iterations": 1000,
            "OpsPerSec": 10000.0,
            "MemoryUsage(bytes)": 2097152
        }
    ]

    baseline_file = "baseline_performance/simulation_performance.csv"
    Path("baseline_performance").mkdir(exist_ok=True)

    with open(baseline_file, 'w', newline='') as f:
        if baseline_data:
            writer = csv.DictWriter(f, fieldnames=baseline_data[0].keys())
            writer.writeheader()
            writer.writerows(baseline_data)

    print(f"✅ Created baseline file: {baseline_file}")

def create_ci_metadata():
    """Create CI metadata file"""
    metadata = {
        "version": "4.0.0",
        "created": "2025-01-08",
        "environment": "ci",
        "test_categories": ["unit", "integration", "benchmark"],
        "benchmark_thresholds": {
            "regression_threshold_percent": 10.0,
            "memory_threshold_mb": 100,
            "timeout_seconds": 300
        }
    }

    with open("baseline_performance/baseline_metadata.json", 'w') as f:
        json.dump(metadata, f, indent=2)

    print("✅ Created CI metadata file")

def verify_test_executables():
    """Verify that test executables exist"""
    test_dirs = [
        "build/tests/unit",
        "build/tests/integration",
        "build/tests/benchmarks"
    ]

    for test_dir in test_dirs:
        if Path(test_dir).exists():
            executables = list(Path(test_dir).glob("*"))
            print(f"✅ Found {len(executables)} executables in {test_dir}")
        else:
            print(f"⚠️  Directory not found: {test_dir}")

def main():
    print("🔧 Setting up benchmark environment for CI/CD...")

    create_benchmark_directories()
    create_sample_baseline()
    create_ci_metadata()
    verify_test_executables()

    print("\n✅ Benchmark environment setup complete!")
    return 0

if __name__ == '__main__':
    sys.exit(main())
