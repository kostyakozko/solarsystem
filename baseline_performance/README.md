# Performance Baselines

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
