#!/usr/bin/env python3
"""
Benchmark Environment Setup for Solar System Suite
Sets up the complete benchmark environment with proper directory structure.
"""

import os
import sys
import json
from pathlib import Path
from datetime import datetime

def create_directory_structure(base_dir="build"):
    """Create the complete benchmark directory structure."""

    directories = [
        os.path.join(base_dir, "tests", "benchmarks", "benchmark_results"),
        os.path.join(base_dir, "tests", "benchmark_baselines"),
        os.path.join(base_dir, "tests", "benchmark_reports"),
        "baseline_performance",
        "benchmark_archives"
    ]

    created_dirs = []

    for directory in directories:
        try:
            Path(directory).mkdir(parents=True, exist_ok=True)
            created_dirs.append(directory)
            print(f"📁 Created/verified: {directory}")
        except Exception as e:
            print(f"❌ Failed to create {directory}: {e}")
            return False

    return True

def create_benchmark_config(config_path="tests/benchmark_config.json"):
    """Create benchmark configuration file."""

    config = {
        "benchmark_settings": {
            "default_iterations": 1000,
            "timeout_seconds": 300,
            "memory_monitoring": True,
            "csv_output": True
        },
        "benchmark_executables": [
            "benchmark_comprehensive",
            "benchmark_jpl_data",
            "benchmark_web_server",
            "scalability_tests",
            "regression_detector"
        ],
        "csv_format": {
            "columns": [
                "Name",
                "AvgDuration(ms)",
                "MinDuration(ms)",
                "MaxDuration(ms)",
                "StdDev(ms)",
                "Iterations",
                "OpsPerSec",
                "MemoryUsage(bytes)"
            ],
            "delimiter": ",",
            "header": True
        },
        "regression_detection": {
            "threshold_percent": 10.0,
            "baseline_file": "baseline_performance/combined_baseline.csv",
            "alert_on_regression": True
        },
        "ci_integration": {
            "github_actions_compatible": True,
            "artifact_paths": [
                "build/tests/benchmarks/benchmark_results/",
                "baseline_performance/",
                "benchmark_reports/"
            ]
        }
    }

    try:
        with open(config_path, 'w') as f:
            json.dump(config, f, indent=2)
        print(f"⚙️  Created benchmark config: {config_path}")
        return True
    except Exception as e:
        print(f"❌ Failed to create config {config_path}: {e}")
        return False

def create_sample_baseline_data(baseline_dir="baseline_performance"):
    """Create sample baseline data for testing regression detection."""

    sample_data = [
        {
            "Name": "CacheLoadingBenchmark",
            "AvgDuration(ms)": "0.125",
            "MinDuration(ms)": "0.098",
            "MaxDuration(ms)": "0.234",
            "StdDev(ms)": "0.045",
            "Iterations": "1000",
            "OpsPerSec": "8000.0",
            "MemoryUsage(bytes)": "1048576"
        },
        {
            "Name": "SimulationStepBenchmark",
            "AvgDuration(ms)": "0.001",
            "MinDuration(ms)": "0.0008",
            "MaxDuration(ms)": "0.0015",
            "StdDev(ms)": "0.0002",
            "Iterations": "10000",
            "OpsPerSec": "1000000.0",
            "MemoryUsage(bytes)": "2097152"
        },
        {
            "Name": "JPLResponseParsingBenchmark",
            "AvgDuration(ms)": "0.050",
            "MinDuration(ms)": "0.045",
            "MaxDuration(ms)": "0.078",
            "StdDev(ms)": "0.012",
            "Iterations": "1000",
            "OpsPerSec": "20000.0",
            "MemoryUsage(bytes)": "524288"
        },
        {
            "Name": "MemoryAllocationBenchmark",
            "AvgDuration(ms)": "0.002",
            "MinDuration(ms)": "0.001",
            "MaxDuration(ms)": "0.005",
            "StdDev(ms)": "0.001",
            "Iterations": "1000",
            "OpsPerSec": "500000.0",
            "MemoryUsage(bytes)": "4194304"
        }
    ]

    baseline_file = os.path.join(baseline_dir, "sample_baseline.csv")

    try:
        import csv
        with open(baseline_file, 'w', newline='') as f:
            fieldnames = ['Name', 'AvgDuration(ms)', 'MinDuration(ms)', 'MaxDuration(ms)',
                         'StdDev(ms)', 'Iterations', 'OpsPerSec', 'MemoryUsage(bytes)']
            writer = csv.DictWriter(f, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(sample_data)

        print(f"📊 Created sample baseline: {baseline_file}")
        return True
    except Exception as e:
        print(f"❌ Failed to create sample baseline: {e}")
        return False

def create_readme_files():
    """Create README files explaining the benchmark system."""

    benchmark_readme = """# Solar System Suite - Performance Benchmarks

## Overview

This directory contains the performance benchmarking system for the Solar System Suite.
The benchmarks validate performance claims and detect regressions.

## Directory Structure

```
benchmark_results/     # Generated CSV results from benchmark runs
benchmark_baselines/   # Historical baseline data for comparison
benchmark_reports/     # Generated performance reports
```

## Benchmark Executables

- `benchmark_comprehensive` - Overall system performance tests
- `benchmark_jpl_data` - JPL data processing performance
- `benchmark_web_server` - Web interface performance
- `scalability_tests` - Scalability analysis with varying loads
- `regression_detector` - Performance regression detection

## CSV Output Format

All benchmarks generate CSV files compatible with `compare_performance.py`:

```csv
Name,AvgDuration(ms),MinDuration(ms),MaxDuration(ms),StdDev(ms),Iterations,OpsPerSec,MemoryUsage(bytes)
```

## Running Benchmarks

```bash
# Run individual benchmarks
./benchmark_comprehensive
./benchmark_jpl_data

# Run all benchmarks via CTest
ctest -L "benchmark" --output-on-failure

# Generate baseline data
python3 ../../scripts/generate_baseline.py

# Compare performance
python3 ../../scripts/compare_performance.py baseline.csv current.csv
```

## Performance Claims Validation

The benchmarks validate these key performance claims:

- **Cache Loading**: 1000-2000x improvement over network fetching
- **Simulation Steps**: Microsecond-level time step execution
- **Memory Usage**: Efficient memory allocation patterns
- **Scalability**: Linear performance scaling with problem size

## CI/CD Integration

The benchmark system integrates with GitHub Actions:

- Automatic baseline generation on main branch
- Performance regression detection on PRs
- Artifact collection for performance tracking
- CSV output compatible with existing scripts
"""

    baseline_readme = """# Performance Baselines

This directory contains baseline performance data for regression detection.

## Files

- `combined_baseline.csv` - Combined baseline from all benchmarks
- `sample_baseline.csv` - Sample data for testing regression detection
- `baseline_metadata.json` - Metadata about baseline generation
- Individual benchmark baselines (e.g., `comprehensive_benchmark.csv`)

## Usage

Baselines are used by `compare_performance.py` to detect performance regressions:

```bash
python3 ../scripts/compare_performance.py combined_baseline.csv ../build/tests/benchmarks/benchmark_results/current_results.csv
```

## Updating Baselines

Baselines should be updated when:
- Significant performance improvements are made
- New benchmarks are added
- System architecture changes

Generate new baselines with:

```bash
python3 ../scripts/generate_baseline.py --baseline-dir baseline_performance
```
"""

    try:
        with open("build/tests/benchmarks/README.md", 'w') as f:
            f.write(benchmark_readme)
        print("📖 Created benchmark README")

        with open("baseline_performance/README.md", 'w') as f:
            f.write(baseline_readme)
        print("📖 Created baseline README")

        return True
    except Exception as e:
        print(f"❌ Failed to create README files: {e}")
        return False

def main():
    print("=" * 80)
    print("SOLAR SYSTEM SUITE - BENCHMARK ENVIRONMENT SETUP")
    print("=" * 80)

    success = True

    # Create directory structure
    print("\n📁 Creating directory structure...")
    if not create_directory_structure():
        success = False

    # Create configuration
    print("\n⚙️  Creating benchmark configuration...")
    if not create_benchmark_config():
        success = False

    # Create sample baseline data
    print("\n📊 Creating sample baseline data...")
    if not create_sample_baseline_data():
        success = False

    # Create documentation
    print("\n📖 Creating documentation...")
    if not create_readme_files():
        success = False

    print("\n" + "=" * 80)
    if success:
        print("✅ BENCHMARK ENVIRONMENT SETUP COMPLETE")
        print("\nNext steps:")
        print("1. Build the project: cmake --build build")
        print("2. Run benchmarks: python3 tests/scripts/generate_baseline.py")
        print("3. Test regression detection: python3 tests/scripts/compare_performance.py")
    else:
        print("❌ BENCHMARK ENVIRONMENT SETUP FAILED")
        print("Please check the error messages above and retry.")
    print("=" * 80)

    return 0 if success else 1

if __name__ == '__main__':
    sys.exit(main())
