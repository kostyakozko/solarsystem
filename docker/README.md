# Docker Files for Solar System Suite

## Local CI Testing

This directory contains Docker configuration for running the complete CI test suite locally.

### Files

- **Dockerfile.local-ci** - Docker image matching GitHub Actions CI environment
- **test-runner.sh** - Test execution script (runs inside container)

### Usage

From the project root:

```bash
# Run complete CI test suite
./run-local-ci.sh

# Interactive debugging
./run-local-ci.sh --interactive

# Rebuild from scratch
./run-local-ci.sh --clean
```

### Documentation

See [docs/LOCAL_CI_TESTING.md](../docs/LOCAL_CI_TESTING.md) for complete documentation.

### Environment

- **Base**: Ubuntu 22.04 (matches GitHub Actions)
- **Compiler**: GCC/G++
- **Dependencies**: Matches `.github/workflows/ci.yml` exactly

### Maintenance

When updating:
1. Keep in sync with `.github/workflows/ci.yml`
2. Test with `./run-local-ci.sh --clean`
3. Verify results match GitHub Actions
