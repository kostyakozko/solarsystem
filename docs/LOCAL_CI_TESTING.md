# Local CI Testing Guide

## Overview

The Solar System Suite includes a local CI testing system that allows you to run the complete GitHub Actions CI test suite on your local machine using Docker. This ensures your changes will pass CI before you push them.

## Quick Start

```bash
# Run the complete CI test suite
./run-local-ci.sh

# Enter interactive shell for debugging
./run-local-ci.sh --interactive

# Rebuild Docker image from scratch
./run-local-ci.sh --clean
```

## Prerequisites

### Required Software
- **Docker**: Install Docker Desktop (macOS) or Docker Engine (Linux)
  - macOS: `brew install --cask docker`
  - Linux: https://docs.docker.com/engine/install/

### System Requirements
- **Disk Space**: ~2GB for Docker image
- **Memory**: 4GB RAM recommended
- **CPU**: Multi-core recommended for faster builds

## Usage

### Running Tests

**Basic Usage**:
```bash
./run-local-ci.sh
```

This will:
1. Build the Docker image (if not already built)
2. Run the complete CI test suite
3. Display results with pass/fail status
4. Exit with appropriate status code

**Output**:
```
╭─────────────────────────────────────────────────────────╮
│   Solar System Suite - Local CI Testing                │
╰─────────────────────────────────────────────────────────╯

Building Docker image...
✓ Docker image built successfully

Running CI test suite...

[1/6] Configuring CMake...
✓ CMake configuration successful

[2/6] Building project...
✓ Build successful

[3/6] Installing...
✓ Installation successful

[4/6] Running Unit Tests...
✓ Unit tests passed

[5/6] Running Integration Tests...
✓ Integration tests passed

[6/6] Running Performance Benchmarks...
✓ Benchmarks passed

╭─────────────────────────────────────────────────────────╮
│   ✅ Local CI tests passed!                             │
│   Your changes are ready to push to GitHub.            │
╰─────────────────────────────────────────────────────────╯
```

### Interactive Debugging

**Enter Container Shell**:
```bash
./run-local-ci.sh --interactive
```

This opens an interactive bash shell inside the CI environment where you can:
- Run tests manually: `./docker/test-runner.sh`
- Debug build issues: `cmake -B build && cmake --build build`
- Inspect test failures: `cd build && ctest --verbose`
- Explore the environment: `ls`, `env`, etc.

**Exit**: Type `exit` or press `Ctrl+D`

### Rebuilding Docker Image

**Clean Rebuild**:
```bash
./run-local-ci.sh --clean
```

Use this when:
- Dependencies have changed
- Dockerfile has been updated
- You want to ensure a fresh environment

### Build Docker Image Only

**Build Without Running Tests**:
```bash
./run-local-ci.sh --build-only
```

Useful for:
- Pre-building the image for faster test runs later
- Verifying Dockerfile changes
- CI/CD pipeline preparation

## Command Reference

### run-local-ci.sh Options

| Option | Description |
|--------|-------------|
| (none) | Run complete CI test suite |
| `-i, --interactive` | Enter interactive shell for debugging |
| `-c, --clean` | Clean and rebuild Docker image |
| `-b, --build-only` | Build Docker image without running tests |
| `-h, --help` | Show help message |

### test-runner.sh (Inside Container)

The test runner script executes the same steps as GitHub Actions CI:

1. **Configure CMake**: `cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTING=ON`
2. **Build**: `cmake --build build -j$(nproc)`
3. **Install**: `cmake --build build --target install`
4. **Unit Tests**: `ctest -L "unit" --timeout 60`
5. **Integration Tests**: `ctest -L "integration" --timeout 180`
6. **Benchmarks**: `ctest -L "benchmark" --timeout 300`

## Environment Details

### Docker Image

**Base Image**: `ubuntu:22.04` (matches GitHub Actions `ubuntu-latest`)

**Installed Packages**:
- cmake
- build-essential (gcc, g++, make)
- curl
- doxygen
- graphviz
- lcov
- git
- bc
- libssl-dev
- libcurl4-openssl-dev
- zlib1g-dev

**Environment Variables**:
- `BUILD_TYPE=Release`
- `CC=gcc`
- `CXX=g++`
- `GITHUB_ACTIONS=true`
- `CI=true`

### Volume Mounting

The current directory is mounted to `/workspace` in the container:
- **Read/Write**: Source code changes are visible in the container
- **Isolation**: Build artifacts stay in the container
- **Persistence**: Changes to source files persist on the host

## Troubleshooting

### Docker Not Found

**Error**: `Docker is not installed or not in PATH`

**Solution**:
```bash
# macOS
brew install --cask docker

# Linux (Ubuntu/Debian)
sudo apt-get install docker.io

# Linux (Fedora)
sudo dnf install docker
```

### Docker Daemon Not Running

**Error**: `Docker daemon is not running`

**Solution**:
- **macOS**: Start Docker Desktop application
- **Linux**: `sudo systemctl start docker`

### Permission Denied

**Error**: `permission denied while trying to connect to the Docker daemon socket`

**Solution** (Linux):
```bash
# Add your user to docker group
sudo usermod -aG docker $USER

# Log out and log back in, or run:
newgrp docker
```

### Build Failures

**Issue**: Tests pass locally but fail in Docker

**Debug Steps**:
1. Enter interactive mode: `./run-local-ci.sh --interactive`
2. Run tests manually: `./docker/test-runner.sh`
3. Check build output: `cmake --build build --verbose`
4. Inspect test logs: `cd build && ctest --verbose`

### Disk Space Issues

**Error**: `no space left on device`

**Solution**:
```bash
# Clean up Docker resources
docker system prune -a

# Remove unused images
docker image prune -a

# Check disk usage
docker system df
```

### Slow Performance

**Issue**: Tests run slowly in Docker

**Solutions**:
1. **Increase Docker Resources**: Docker Desktop → Preferences → Resources
   - CPUs: 4+ cores
   - Memory: 4GB+
   - Disk: 20GB+

2. **Use Build Cache**: Don't use `--clean` unless necessary

3. **Parallel Builds**: Already using `-j$(nproc)` for maximum parallelism

## CI Parity Verification

### Matching GitHub Actions

The local CI environment is designed to exactly match GitHub Actions:

| Aspect | GitHub Actions | Local CI |
|--------|---------------|----------|
| OS | ubuntu-latest (22.04) | ubuntu:22.04 |
| Compiler | gcc/g++ | gcc/g++ |
| CMake Config | Release, ENABLE_TESTING=ON | Same |
| Test Categories | unit, integration, benchmark | Same |
| Test Timeouts | 60s, 180s, 300s | Same |
| Environment | GITHUB_ACTIONS=true | Same |

### Verification Steps

1. **Run Locally**: `./run-local-ci.sh`
2. **Push to GitHub**: `git push`
3. **Compare Results**: Check GitHub Actions logs

Results should be identical. If not, please report an issue.

## Best Practices

### Before Pushing

```bash
# Always run local CI before pushing
./run-local-ci.sh

# If tests pass, push with confidence
git push
```

### During Development

```bash
# Quick iteration: use interactive mode
./run-local-ci.sh --interactive

# Inside container:
./docker/test-runner.sh  # Run full suite
cd build && ctest -L unit  # Run only unit tests
```

### Debugging Failures

```bash
# 1. Enter interactive mode
./run-local-ci.sh --interactive

# 2. Run tests with verbose output
cd build
ctest --verbose --output-on-failure

# 3. Run specific test
ctest -R TestName --verbose

# 4. Debug with gdb (if needed)
gdb ./tests/unit/test_name
```

### Performance Testing

```bash
# Run benchmarks separately
./run-local-ci.sh --interactive
cd build
ctest -L benchmark --verbose
```

## Integration with Development Workflow

### Pre-commit Hook (Optional)

Create `.git/hooks/pre-push`:
```bash
#!/bin/bash
echo "Running local CI tests before push..."
./run-local-ci.sh
```

Make it executable:
```bash
chmod +x .git/hooks/pre-push
```

### IDE Integration

**VS Code**: Add to `.vscode/tasks.json`:
```json
{
  "label": "Run Local CI",
  "type": "shell",
  "command": "./run-local-ci.sh",
  "problemMatcher": []
}
```

## Advanced Usage

### Custom Build Types

```bash
# Debug build
./run-local-ci.sh --interactive
export BUILD_TYPE=Debug
./docker/test-runner.sh
```

### Specific Test Categories

```bash
./run-local-ci.sh --interactive
cd build

# Unit tests only
ctest -L unit

# Integration tests only
ctest -L integration

# Specific test
ctest -R test_name --verbose
```

### Artifact Collection

Build artifacts are in the container at:
- `/workspace/build` - Build directory
- `/workspace/install` - Installation directory

To copy artifacts out:
```bash
# While container is running (in another terminal)
docker cp CONTAINER_ID:/workspace/build/file.txt ./
```

## Maintenance

### Updating Dependencies

When GitHub Actions dependencies change:

1. Update `docker/Dockerfile.local-ci`
2. Rebuild: `./run-local-ci.sh --clean`
3. Verify: `./run-local-ci.sh`

### Updating Test Configuration

When test configuration changes:

1. Update `docker/test-runner.sh`
2. Test: `./run-local-ci.sh`

## Support

### Getting Help

- **Documentation**: This file
- **GitHub Issues**: Report problems
- **CI Logs**: Compare with GitHub Actions

### Common Questions

**Q: Do I need to rebuild the image every time?**
A: No, only when dependencies or Dockerfile changes.

**Q: Can I run this on Windows?**
A: Yes, with Docker Desktop and WSL2.

**Q: How long does it take?**
A: First run: ~5-10 minutes (build + tests)
    Subsequent runs: ~2-3 minutes (tests only)

**Q: Does this modify my source code?**
A: No, source code is mounted read-write but tests don't modify it.

---

**Last Updated**: 2025-11-04
**Version**: 1.0
**Status**: Production Ready
