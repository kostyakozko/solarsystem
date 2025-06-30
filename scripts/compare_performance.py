#!/usr/bin/env python3
"""
Performance regression detection script for Solar System Suite.
Compares current benchmark results with baseline to detect regressions.
"""

import sys
import csv
import os
from typing import Dict, List, Tuple

def load_benchmark_data(filepath: str) -> Dict[str, float]:
    """Load benchmark data from CSV file."""
    data = {}
    try:
        with open(filepath, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                test_name = row.get('test_name', row.get('Test', ''))
                time_value = row.get('time_seconds', row.get('Time', '0'))
                if test_name and time_value:
                    data[test_name] = float(time_value)
    except (FileNotFoundError, ValueError, KeyError) as e:
        print(f"Warning: Could not load benchmark data from {filepath}: {e}")
    return data

def compare_performance(baseline: Dict[str, float], current: Dict[str, float], 
                       threshold: float = 0.2) -> Tuple[bool, List[str]]:
    """
    Compare performance between baseline and current results.
    
    Args:
        baseline: Baseline benchmark results
        current: Current benchmark results  
        threshold: Regression threshold (20% by default)
        
    Returns:
        Tuple of (has_regression, list_of_issues)
    """
    issues = []
    has_regression = False
    
    for test_name, current_time in current.items():
        if test_name in baseline:
            baseline_time = baseline[test_name]
            if baseline_time > 0:
                ratio = current_time / baseline_time
                if ratio > (1 + threshold):
                    regression_pct = (ratio - 1) * 100
                    issues.append(f"REGRESSION: {test_name} is {regression_pct:.1f}% slower "
                                f"({current_time:.3f}s vs {baseline_time:.3f}s)")
                    has_regression = True
                elif ratio < 0.8:  # Significant improvement
                    improvement_pct = (1 - ratio) * 100
                    issues.append(f"IMPROVEMENT: {test_name} is {improvement_pct:.1f}% faster "
                                f"({current_time:.3f}s vs {baseline_time:.3f}s)")
        else:
            issues.append(f"NEW TEST: {test_name} ({current_time:.3f}s)")
    
    # Check for missing tests
    for test_name in baseline:
        if test_name not in current:
            issues.append(f"MISSING TEST: {test_name} (was {baseline[test_name]:.3f}s)")
    
    return has_regression, issues

def main():
    """Main performance comparison function."""
    if len(sys.argv) != 3:
        print("Usage: python3 compare_performance.py <baseline.csv> <current.csv>")
        sys.exit(1)
    
    baseline_file = sys.argv[1]
    current_file = sys.argv[2]
    
    print("🔍 Solar System Suite Performance Regression Detection")
    print(f"📊 Baseline: {baseline_file}")
    print(f"📊 Current:  {current_file}")
    print()
    
    # Load data
    baseline_data = load_benchmark_data(baseline_file)
    current_data = load_benchmark_data(current_file)
    
    if not baseline_data:
        print("⚠️  No baseline data available - skipping regression check")
        print("✅ Performance check passed (no baseline to compare)")
        return
    
    if not current_data:
        print("❌ No current benchmark data found")
        sys.exit(1)
    
    # Compare performance
    has_regression, issues = compare_performance(baseline_data, current_data)
    
    # Report results
    print(f"📈 Analyzed {len(current_data)} current tests vs {len(baseline_data)} baseline tests")
    print()
    
    if issues:
        for issue in issues:
            print(f"  {issue}")
        print()
    
    if has_regression:
        print("❌ Performance regression detected!")
        print("   Consider optimizing the affected code or updating the baseline if intentional.")
        sys.exit(1)
    else:
        print("✅ No performance regressions detected")
        if any("IMPROVEMENT" in issue for issue in issues):
            print("🚀 Performance improvements detected!")

if __name__ == "__main__":
    main()
