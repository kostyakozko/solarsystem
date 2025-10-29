# Solar System Suite Dependencies

## Required Dependencies

### Core Dependencies (Already Required)
- **CMake 3.15+**: Build system
- **C++20 Compiler**: GCC 7+, Clang 5+, or MSVC 2019+
- **curl**: For JPL HORIZONS API data fetching

### Security Dependencies (Task 25)
- **OpenSSL 1.1.1+**: For encryption, hashing, and secure network operations

## Installation Instructions

### macOS

```bash
# Install using Homebrew
brew install cmake openssl curl

# If OpenSSL is not found by CMake, set the path:
export OPENSSL_ROOT_DIR=/opt/homebrew/opt/openssl@3
# or for Intel Macs:
export OPENSSL_ROOT_DIR=/usr/local/opt/openssl@3
```

### Ubuntu/Debian

```bash
# Install required packages
sudo apt-get update
sudo apt-get install -y \
    cmake \
    build-essential \
    libssl-dev \
    libcurl4-openssl-dev

# Verify OpenSSL installation
openssl version
```

### Fedora/RHEL/CentOS

```bash
# Install required packages
sudo dnf install -y \
    cmake \
    gcc-c++ \
    openssl-devel \
    libcurl-devel
```

### Windows

```bash
# Using vcpkg
vcpkg install openssl curl

# Or using Chocolatey
choco install cmake openssl curl
```

## Verification

After installation, verify dependencies:

```bash
# Check CMake
cmake --version

# Check OpenSSL
openssl version

# Check curl
curl --version

# Check compiler
g++ --version  # or clang++ --version
```

## Optional Dependencies

### Development Tools
- **clang-format**: Code formatting
- **Doxygen**: API documentation generation
- **gcov/lcov**: Code coverage analysis

### Testing Tools
- **valgrind**: Memory leak detection (Linux only)
- **lldb/gdb**: Debugging

## Dependency Versions

| Dependency | Minimum Version | Recommended Version |
|------------|----------------|---------------------|
| CMake      | 3.15           | 3.20+              |
| GCC        | 7.0            | 11.0+              |
| Clang      | 5.0            | 14.0+              |
| OpenSSL    | 1.1.1          | 3.0+               |
| curl       | 7.58           | 7.80+              |

## Build Configuration

The build system will automatically detect installed dependencies. If a dependency is not found, CMake will provide clear error messages with installation instructions.

To explicitly specify dependency locations:

```bash
cmake -DOPENSSL_ROOT_DIR=/path/to/openssl \
      -DCURL_ROOT=/path/to/curl \
      ..
```

## Troubleshooting

### OpenSSL Not Found

If CMake cannot find OpenSSL:

```bash
# macOS
export OPENSSL_ROOT_DIR=$(brew --prefix openssl@3)

# Linux - check installation
dpkg -l | grep libssl-dev  # Ubuntu/Debian
rpm -qa | grep openssl-devel  # Fedora/RHEL
```

### curl Not Found

```bash
# Check if curl development files are installed
# Ubuntu/Debian
dpkg -l | grep libcurl

# Fedora/RHEL
rpm -qa | grep libcurl
```

## License Compatibility

All dependencies use permissive licenses compatible with the Solar System Suite:
- OpenSSL: Apache License 2.0 (OpenSSL 3.0+) or dual OpenSSL/SSLeay license (1.1.1)
- curl: MIT/X derivative license
- CMake: BSD 3-Clause License
