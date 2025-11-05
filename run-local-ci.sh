#!/bin/bash
# Main entry point for local CI testing
# Runs the complete CI test suite in a Docker container matching GitHub Actions

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
DOCKER_IMAGE="solar-system-local-ci:latest"
DOCKERFILE="docker/Dockerfile.local-ci"
CONTAINER_NAME="solar-system-ci-$$"

# Parse command line arguments
INTERACTIVE=false
CLEAN=false
BUILD_ONLY=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -i|--interactive)
            INTERACTIVE=true
            shift
            ;;
        -c|--clean)
            CLEAN=true
            shift
            ;;
        -b|--build-only)
            BUILD_ONLY=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  -i, --interactive    Enter interactive shell instead of running tests"
            echo "  -c, --clean          Clean Docker image and rebuild from scratch"
            echo "  -b, --build-only     Only build the Docker image, don't run tests"
            echo "  -h, --help           Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                   # Run complete CI test suite"
            echo "  $0 --interactive     # Enter container for debugging"
            echo "  $0 --clean           # Rebuild Docker image and run tests"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

echo "╭─────────────────────────────────────────────────────────╮"
echo "│   Solar System Suite - Local CI Testing                │"
echo "╰─────────────────────────────────────────────────────────╯"
echo ""

# Check if Docker is available
if ! command -v docker &> /dev/null; then
    echo -e "${RED}✗ Docker is not installed or not in PATH${NC}"
    echo ""
    echo "Please install Docker:"
    echo "  macOS: brew install --cask docker"
    echo "  Linux: https://docs.docker.com/engine/install/"
    exit 1
fi

# Check if Docker daemon is running
if ! docker info &> /dev/null; then
    echo -e "${RED}✗ Docker daemon is not running${NC}"
    echo ""
    echo "Please start Docker and try again."
    exit 1
fi

# Clean if requested
if [ "$CLEAN" = true ]; then
    echo -e "${YELLOW}Cleaning Docker image...${NC}"
    docker rmi "$DOCKER_IMAGE" 2>/dev/null || true
    echo ""
fi

# Build Docker image
echo -e "${BLUE}Building Docker image...${NC}"
if docker build -t "$DOCKER_IMAGE" -f "$DOCKERFILE" .; then
    echo -e "${GREEN}✓ Docker image built successfully${NC}"
else
    echo -e "${RED}✗ Docker image build failed${NC}"
    exit 1
fi
echo ""

if [ "$BUILD_ONLY" = true ]; then
    echo "Docker image built. Exiting (--build-only specified)."
    exit 0
fi

# Run container
if [ "$INTERACTIVE" = true ]; then
    echo -e "${BLUE}Starting interactive shell...${NC}"
    echo ""
    echo "You are now in the CI environment."
    echo "To run tests manually: ./docker/test-runner.sh"
    echo "To exit: type 'exit' or press Ctrl+D"
    echo ""

    docker run --rm -it \
        --name "$CONTAINER_NAME" \
        -v "$(pwd):/workspace" \
        -w /workspace \
        "$DOCKER_IMAGE" \
        /bin/bash
else
    echo -e "${BLUE}Running CI test suite...${NC}"
    echo ""

    # Make test runner executable
    chmod +x docker/test-runner.sh

    # Run tests
    if docker run --rm \
        --name "$CONTAINER_NAME" \
        -v "$(pwd):/workspace" \
        -w /workspace \
        "$DOCKER_IMAGE" \
        ./docker/test-runner.sh; then

        echo ""
        echo "╭─────────────────────────────────────────────────────────╮"
        echo "│   ✅ Local CI tests passed!                             │"
        echo "│   Your changes are ready to push to GitHub.            │"
        echo "╰─────────────────────────────────────────────────────────╯"
        exit 0
    else
        echo ""
        echo "╭─────────────────────────────────────────────────────────╮"
        echo "│   ❌ Local CI tests failed                              │"
        echo "│   Please fix the issues before pushing.                │"
        echo "╰─────────────────────────────────────────────────────────╯"
        echo ""
        echo "To debug interactively:"
        echo "  $0 --interactive"
        exit 1
    fi
fi
