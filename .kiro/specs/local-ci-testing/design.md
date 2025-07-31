# Design Document

## Overview

This design implements a local CI testing system using Docker that replicates the GitHub Actions environment. The system allows developers to run the complete CI test suite locally, providing fast feedback and debugging capabilities.

## Architecture

### Docker-based Testing Environment

The system uses Docker to create an isolated Ubuntu environment that matches the GitHub Actions CI configuration. This ensures consistent testing results between local development and the CI pipeline.

### Component Structure

```
local-ci/
├── Dockerfile              # Ubuntu environment matching CI
├── docker-compose.yml      # Service orchestration
├── scripts/
│   ├── run-local-ci.sh    # Main entry point
│   ├── setup-env.sh       # Environment setup
│   └── test-runner.sh     # Test execution
├── config/
│   ├── cmake-config.sh    # CMake configuration
│   └── test-config.sh     # Test configuration
└── .dockerignore          # Docker ignore rules
```

## Components and Interfaces

### Docker Environment

```dockerfile
FROM ubuntu:22.04

# Match GitHub Actions environment
ENV DEBIAN_FRONTEND=noninteractive
ENV CC=gcc
ENV CXX=g++
ENV GITHUB_ACTIONS=true

# Install dependencies matching CI
RUN apt-get update && apt-get install -y \
    cmake \
    build-essential \
    curl \
    doxygen \
    graphviz \
    lcov \
    git \
    && rm -rf /var/lib/apt/lists/*

# Set up working directory
WORKDIR /workspace
```

### Main CI Runner Script

```bash
#!/bin/bash
# run-local-ci.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Configuration
DOCKER_IMAGE="solar-system-ci:latest"
CONTAINER_NAME="solar-system-ci-runner"

# Build Docker image if needed
build_image() {
    echo "Building CI Docker image..."
    docker build -t "$DOCKER_IMAGE" "$SCRIPT_DIR"
}

# Run CI tests in container
run_tests() {
    echo "Running CI tests in Docker container..."

    docker run --rm \
        --name "$CONTAINER_NAME" \
        -v "$PROJECT_ROOT:/workspace" \
        -v "$PROJECT_ROOT/local-ci/results:/results" \
        -e "BUILD_TYPE=Release" \
        -e "ENABLE_TESTING=ON" \
        "$DOCKER_IMAGE" \
        /workspace/local-ci/scripts/test-runner.sh
}

# Main execution
main() {
    echo "Solar System Suite - Local CI Testing"
    echo "====================================="

    # Check if Docker is available
    if ! command -v docker &> /dev/null; then
        echo "Error: Docker is not installed or not in PATH"
        exit 1
    fi

    # Build image if it doesn't exist or if --rebuild is specified
    if [[ "$1" == "--rebuild" ]] || ! docker image inspect "$DOCKER_IMAGE" &> /dev/null; then
        build_image
    fi

    # Run tests
    run_tests

    echo "Local CI testing completed!"
}

main "$@"
```

### Test Runner

```bash
#!/bin/bash
# test-runner.sh

set -e

echo "Setting up build environment..."
source /workspace/local-ci/scripts/setup-env.sh

echo "Configuring CMake..."
cmake -B build \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DENABLE_TESTING=ON \
    -DENABLE_PROFILING=OFF

echo "Building project..."
cmake --build build --config "$BUILD_TYPE" -j"$(nproc)"

echo "Installing project..."
cmake --build build --target install

echo "Running unit tests..."
cd build
ctest -L "unit" --output-on-failure --timeout 60

echo "Running integration tests..."
ctest -L "integration" --output-on-failure --timeout 180

echo "Running performance benchmarks..."
ctest -L "benchmark" --output-on-failure --timeout 300 || true

echo "Testing installation..."
cd /workspace/install
./solar_system_launcher --status
./bin/solar_system --help
./bin/solar_system_fetch --test-storage

echo "All tests completed successfully!"
```

## Data Models

### CI Configuration

```yaml
# docker-compose.yml
version: '3.8'

services:
  ci-runner:
    build:
      context: .
      dockerfile: Dockerfile
    volumes:
      - ../:/workspace
      - ./results:/results
    environment:
      - BUILD_TYPE=Release
      - ENABLE_TESTING=ON
      - CC=gcc
      - CXX=g++
      - GITHUB_ACTIONS=true
    working_dir: /workspace
    command: /workspace/local-ci/scripts/test-runner.sh
```

### Test Results Structure

```
results/
├── test-results.xml       # CTest XML output
├── build-log.txt         # Build output
├── test-log.txt          # Test execution log
├── benchmark-results/    # Performance data
└── coverage-report/      # Code coverage (if enabled)
```

## Error Handling

### Docker Environment Errors
- Check Docker installation and permissions
- Validate Docker image build process
- Handle container startup failures
- Provide clear error messages for common issues

### Build and Test Errors
- Capture and display build failures clearly
- Show test failure details with context
- Preserve error logs for debugging
- Exit with appropriate error codes

### Resource Management
- Clean up Docker containers after use
- Handle interrupted executions gracefully
- Manage disk space for Docker images
- Prevent resource leaks

## Testing Strategy

### Validation Tests
- Verify Docker environment matches CI exactly
- Test that local results match GitHub Actions
- Validate all test categories run correctly
- Ensure artifacts are generated properly

### Integration Testing
- Test with different source code states
- Verify behavior with failing tests
- Test resource cleanup and error handling
- Validate cross-platform Docker compatibility

## Implementation Approach

### Phase 1: Basic Docker Environment
1. Create Dockerfile matching Ubuntu CI environment
2. Install dependencies matching GitHub Actions
3. Set up basic build and test execution
4. Verify environment parity with CI

### Phase 2: Test Execution Framework
1. Implement test runner script
2. Add proper error handling and logging
3. Create result collection and reporting
4. Test all test categories (unit, integration, benchmarks)

### Phase 3: Developer Experience
1. Create simple entry point script
2. Add Docker image caching and optimization
3. Implement interactive debugging capabilities
4. Add comprehensive documentation

### Phase 4: Integration and Validation
1. Test with actual CI failures
2. Verify results match GitHub Actions exactly
3. Optimize performance and resource usage
4. Create troubleshooting guides

## Performance Considerations

- Use multi-stage Docker builds for efficiency
- Implement Docker layer caching for dependencies
- Optimize build parallelization with available CPU cores
- Minimize container startup time
- Efficient volume mounting for source code

## Security Considerations

- Run containers with minimal privileges
- Avoid exposing unnecessary ports
- Use read-only mounts where possible
- Validate input parameters and paths
- Clean up temporary files and containers
