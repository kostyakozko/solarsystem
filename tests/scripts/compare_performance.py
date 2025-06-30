#!/usr/bin/env python3
"""
Performance Comparison Script for Solar System Suite
Compares benchmark results to detect performance regressions.
"""

import sys
import csv
import argparse
from typing import Dict, List, Tuple

def load_benchmark_data(filename: str) -> Dict[str, Dict[str, float]]:
    """Load benchmark data from CSV file."""
    data = {}
    try:
        with open(filename, 'r') as file:
            reader = csv.DictReader(file)
            for row in reader:
                name = row['Name']
                data[name] = {
                    'avg_duration': float(row['AvgDuration(ms)']),
                    'min_duration': float(row['MinDuration(ms)']),
                    'max_duration': float(row['MaxDuration(ms)']),
                    'std_dev': float(row['StdDev(ms)']),
                    'iterations': int(row['Iterations']),
                    'ops_per_sec': float(row['OpsPerSec']),
                    'memory_usage': int(row['MemoryUsage(bytes)'])
                }
    except FileNotFoundError:
        print(f"Error: File {filename} not found")
        sys.exit(1)
    except Exception as e:
        print(f"Error reading {filename}: {e}")
        sys.exit(1)
    
    return data

def compare_benchmarks(baseline: Dict, current: Dict, threshold: float = 10.0) -> List[Tuple[str, float, str]]:
    """Compare benchmark results and identify regressions."""
    regressions = []
    improvements = []
    
    for name in current:
        if name not in baseline:
            print(f"New benchmark: {name}")
            continue
            
        baseline_perf = baseline[name]['avg_duration']
        current_perf = current[name]['avg_duration']
        
        if baseline_perf == 0:
            continue
            
        change_percent = ((current_perf - baseline_perf) / baseline_perf) * 100
        
        if change_percent > threshold:
            regressions.append((name, change_percent, "REGRESSION"))
        elif change_percent < -threshold:
            improvements.append((name, abs(change_percent), "IMPROVEMENT"))
    
    return regressions, improvements

def print_comparison_report(baseline_file: str, current_file: str, threshold: float):
    """Print detailed comparison report."""
    print("=" * 80)
    print("SOLAR SYSTEM SUITE - PERFORMANCE COMPARISON REPORT")
    print("=" * 80)
    print(f"Baseline: {baseline_file}")
    print(f"Current:  {current_file}")
    print(f"Regression threshold: {threshold}%")
    print()
    
    baseline_data = load_benchmark_data(baseline_file)
    current_data = load_benchmark_data(current_file)
    
    regressions, improvements = compare_benchmarks(baseline_data, current_data, threshold)
    
    # Print regressions
    if regressions:
        print("🔴 PERFORMANCE REGRESSIONS DETECTED:")
        print("-" * 50)
        for name, change, status in regressions:
            print(f"  {name}: +{change:.1f}% slower")
        print()
    else:
        print("✅ No performance regressions detected")
        print()
    
    # Print improvements
    if improvements:
        print("🟢 PERFORMANCE IMPROVEMENTS:")
        print("-" * 50)
        for name, change, status in improvements:
            print(f"  {name}: +{change:.1f}% faster")
        print()
    
    # Detailed comparison table
    print("DETAILED COMPARISON:")
    print("-" * 80)
    print(f"{'Benchmark':<30} {'Baseline (ms)':<15} {'Current (ms)':<15} {'Change':<10}")
    print("-" * 80)
    
    for name in sorted(current_data.keys()):
        if name in baseline_data:
            baseline_perf = baseline_data[name]['avg_duration']
            current_perf = current_data[name]['avg_duration']
            
            if baseline_perf > 0:
                change_percent = ((current_perf - baseline_perf) / baseline_perf) * 100
                change_str = f"{change_percent:+.1f}%"
            else:
                change_str = "N/A"
            
            print(f"{name:<30} {baseline_perf:<15.3f} {current_perf:<15.3f} {change_str:<10}")
    
    print("-" * 80)
    
    # Summary
    total_benchmarks = len(current_data)
    regression_count = len(regressions)
    improvement_count = len(improvements)
    
    print(f"\nSUMMARY:")
    print(f"  Total benchmarks: {total_benchmarks}")
    print(f"  Regressions: {regression_count}")
    print(f"  Improvements: {improvement_count}")
    print(f"  Unchanged: {total_benchmarks - regression_count - improvement_count}")
    
    # Exit with error code if regressions found
    if regressions:
        print(f"\n❌ Performance regression detected! {regression_count} benchmark(s) slower than threshold.")
        return 1
    else:
        print(f"\n✅ Performance check passed! No significant regressions detected.")
        return 0

def main():
    parser = argparse.ArgumentParser(description='Compare Solar System Suite performance benchmarks')
    parser.add_argument('baseline', help='Baseline benchmark CSV file')
    parser.add_argument('current', help='Current benchmark CSV file')
    parser.add_argument('-t', '--threshold', type=float, default=10.0,
                       help='Regression threshold percentage (default: 10.0)')
    parser.add_argument('--json', action='store_true',
                       help='Output results in JSON format')
    
    args = parser.parse_args()
    
    if args.json:
        # JSON output for CI/CD integration
        baseline_data = load_benchmark_data(args.baseline)
        current_data = load_benchmark_data(args.current)
        regressions, improvements = compare_benchmarks(baseline_data, current_data, args.threshold)
        
        import json
        result = {
            'regressions': [{'name': name, 'change_percent': change} for name, change, _ in regressions],
            'improvements': [{'name': name, 'change_percent': change} for name, change, _ in improvements],
            'total_benchmarks': len(current_data),
            'regression_count': len(regressions),
            'improvement_count': len(improvements)
        }
        print(json.dumps(result, indent=2))
        return 1 if regressions else 0
    else:
        return print_comparison_report(args.baseline, args.current, args.threshold)

if __name__ == '__main__':
    sys.exit(main())
