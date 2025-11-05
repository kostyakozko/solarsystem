#!/bin/bash
# Deployment validation script for Solar System Suite

set -e

INSTALL_DIR="${1:-./install}"

echo "╭─────────────────────────────────────────────────────────╮"
echo "│   Solar System Suite - Deployment Validation           │"
echo "╰─────────────────────────────────────────────────────────╯"
echo ""
echo "Installation directory: $INSTALL_DIR"
echo ""

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Check main launcher
echo "Checking main launcher..."
if [ -x "$INSTALL_DIR/solar_system_launcher" ]; then
    echo -e "${GREEN}✓${NC} Main launcher found"
else
    echo -e "${RED}✗${NC} Main launcher missing"
    exit 1
fi

# Check executables
echo ""
echo "Checking executables..."
for exe in solar_system solar_system_fetch solar_system_launcher solar_system_realtime solar_system_web; do
    if [ -x "$INSTALL_DIR/bin/$exe" ]; then
        echo -e "${GREEN}✓${NC} $exe found"
    else
        echo -e "${RED}✗${NC} $exe missing"
        exit 1
    fi
done

# Check libraries
echo ""
echo "Checking libraries..."
for lib in libsolar_core.a libsolar_jpl.a libsolar_utils.a; do
    if [ -f "$INSTALL_DIR/lib/$lib" ]; then
        echo -e "${GREEN}✓${NC} $lib found"
    else
        echo -e "${RED}✗${NC} $lib missing"
        exit 1
    fi
done

# Test launcher
echo ""
echo "Testing functionality..."
if "$INSTALL_DIR/solar_system_launcher" --status > /dev/null 2>&1; then
    echo -e "${GREEN}✓${NC} Launcher functional"
else
    echo -e "${RED}✗${NC} Launcher not functional"
    exit 1
fi

# Test simulation
if "$INSTALL_DIR/bin/solar_system" --help > /dev/null 2>&1; then
    echo -e "${GREEN}✓${NC} Simulation functional"
else
    echo -e "${RED}✗${NC} Simulation not functional"
    exit 1
fi

# Test fetch
if "$INSTALL_DIR/bin/solar_system_fetch" --test-storage > /dev/null 2>&1; then
    echo -e "${GREEN}✓${NC} Fetch functional"
else
    echo -e "${RED}✗${NC} Fetch not functional"
    exit 1
fi

echo ""
echo "╭─────────────────────────────────────────────────────────╮"
echo "│   ✅ Deployment validation passed!                      │"
echo "│   Installation is ready for use                         │"
echo "╰─────────────────────────────────────────────────────────╯"
