# Docker Files for Solar System Suite

## Local CI Testing

This directory contains Docker configuration for running the complete CI test suite locally.

### Files

- **Dockerfile.local-ci** - Linux Docker image matching GitHub Actions CI environment
- **test-runner.sh** - Test execution script (runs inside Linux container)

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

### Environment

- **Base**: Ubuntu 22.04 (matches GitHub Actions)
- **Compiler**: GCC/G++
- **Dependencies**: Matches `.github/workflows/ci.yml` exactly

### Documentation

See [docs/LOCAL_CI_TESTING.md](../docs/LOCAL_CI_TESTING.md) for complete documentation.

## Windows Development

### Option 1: WSL (Recommended)

WSL provides the easiest way to build on Windows using the same Linux toolchain as CI:

```powershell
# Install WSL2 with Ubuntu (PowerShell as Administrator)
wsl --install -d Ubuntu
```

After restart, open Ubuntu and install dependencies:

```bash
sudo apt-get update
sudo apt-get install -y cmake build-essential pkg-config \
    libssl-dev libcurl4-openssl-dev zlib1g-dev libboost-dev \
    libcairo2-dev libpng-dev

# Build the project
cd /mnt/c/path/to/solarsystem
cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTING=ON
cmake --build build -j$(nproc)
```

### Option 2: Native Windows Build

For native MSVC builds, see `requirements-windows.txt` and use:

```powershell
.\scripts\build-windows.ps1 -Test
```

## Cross-Platform Dependencies with vcpkg

The project includes a `vcpkg.json` manifest for cross-platform dependency management:

```bash
# Linux/macOS
vcpkg install --triplet x64-linux   # or x64-osx

# Windows
vcpkg install --triplet x64-windows
```

Or use manifest mode (automatic):
```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
```

## Maintenance

When updating:
1. Keep in sync with `.github/workflows/ci.yml`
2. Test with `./run-local-ci.sh --clean`
3. Verify results match GitHub Actions
