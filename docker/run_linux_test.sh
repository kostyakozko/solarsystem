#!/bin/bash

# Script to run Linux cross-platform compatibility testing using Docker

set -e

echo "🐧 Solar System Suite - Linux Cross-Platform Testing"
echo "=================================================="
echo

# Check if Docker is available
if ! command -v docker &> /dev/null; then
    echo "❌ Docker is not installed or not in PATH"
    echo "   Please install Docker to run Linux compatibility tests"
    exit 1
fi

echo "🔍 Checking Docker status..."
if ! docker info &> /dev/null; then
    echo "❌ Docker daemon is not running"
    echo "   Please start Docker and try again"
    exit 1
fi

echo "✅ Docker is available and running"
echo

# Build the Docker image
echo "🔨 Building Linux test environment..."
docker build -f docker/Dockerfile.linux-test -t solar-system-linux-test . || {
    echo "❌ Failed to build Docker image"
    exit 1
}

echo "✅ Linux test environment built successfully"
echo

# Run the compatibility tests
echo "🧪 Running Linux compatibility tests..."
echo "   This may take several minutes..."
echo

# Create a container and run tests, then copy results back
CONTAINER_NAME="solar-system-test-$(date +%s)"

# Run the container
docker run --name "$CONTAINER_NAME" solar-system-linux-test || {
    echo "❌ Linux compatibility tests failed"
    FAILED=true
}

# Copy test results back to host
echo
echo "📋 Copying test results from container..."

# Create results directory
mkdir -p linux_test_results

# Copy results from container
docker cp "$CONTAINER_NAME:/workspace/build/linux_test_results.log" linux_test_results/ 2>/dev/null || echo "   No detailed test log found"
docker cp "$CONTAINER_NAME:/workspace/build/linux_compatibility_report.md" linux_test_results/ 2>/dev/null || echo "   No compatibility report found"

# Copy any additional test artifacts
docker cp "$CONTAINER_NAME:/workspace/build/Testing/" linux_test_results/ 2>/dev/null || echo "   No CTest results found"

# Clean up container
docker rm "$CONTAINER_NAME" &>/dev/null

echo "✅ Test results copied to linux_test_results/"
echo

# Show summary
if [ -f "linux_test_results/linux_compatibility_report.md" ]; then
    echo "📊 Linux Compatibility Summary:"
    echo "================================"
    cat linux_test_results/linux_compatibility_report.md
else
    echo "⚠️  No compatibility report generated"
fi

echo
echo "📁 Generated files:"
echo "  - linux_test_results/linux_test_results.log (detailed test output)"
echo "  - linux_test_results/linux_compatibility_report.md (summary report)"
echo "  - linux_test_results/Testing/ (CTest results)"

if [ "$FAILED" = true ]; then
    echo
    echo "❌ Linux compatibility tests completed with failures"
    echo "   Review the logs in linux_test_results/ for details"
    exit 1
else
    echo
    echo "✅ Linux compatibility tests completed successfully"
fi
