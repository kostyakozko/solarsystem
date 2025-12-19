# Post-installation script for Solar System Suite
# This script configures the installed launcher to find other executables

set(INSTALL_PREFIX "/Users/kostiantyn.kozko/tmp/solarsystem/install")
set(BIN_DIR "bin")

message(STATUS "Configuring Solar System Suite installation...")
message(STATUS "Install directory: ${INSTALL_PREFIX}")

# Verify installation
if(EXISTS "${INSTALL_PREFIX}/solar_system_launcher")
    message(STATUS "✓ Main launcher installed: ${INSTALL_PREFIX}/solar_system_launcher")
else()
    message(WARNING "✗ Main launcher not found at: ${INSTALL_PREFIX}/solar_system_launcher")
endif()

if(EXISTS "${INSTALL_PREFIX}/${BIN_DIR}/solar_system")
    message(STATUS "✓ Simulation executable: ${INSTALL_PREFIX}/${BIN_DIR}/solar_system")
else()
    message(WARNING "✗ Simulation executable not found")
endif()

if(EXISTS "${INSTALL_PREFIX}/${BIN_DIR}/solar_system_fetch")
    message(STATUS "✓ Data fetcher executable: ${INSTALL_PREFIX}/${BIN_DIR}/solar_system_fetch")
else()
    message(WARNING "✗ Data fetcher executable not found")
endif()

# Create usage instructions
file(WRITE "${INSTALL_PREFIX}/USAGE.txt" 
"Solar System Suite - Installation Complete

Main Entry Point:
  ./solar_system_launcher --help

Quick Start:
  ./solar_system_launcher --status              # Check system status
  ./solar_system_launcher --fetch --update      # Update JPL data
  ./solar_system_launcher --simulate            # Run simulation

Individual Tools (in bin/ directory):
  ./bin/solar_system                            # Direct simulation
  ./bin/solar_system_fetch                      # Direct data management
  ./bin/solar_system_launcher                   # Unified interface

Environment Setup:
  Add ${INSTALL_PREFIX} to your PATH to use 'solar_system_launcher' from anywhere
  Add ${INSTALL_PREFIX}/${BIN_DIR} to your PATH to use individual tools

For more information, see README.md in share/solar_system/
")

message(STATUS "✓ Installation configured successfully")
message(STATUS "✓ Usage instructions created: ${INSTALL_PREFIX}/USAGE.txt")
message(STATUS "")
message(STATUS "To use Solar System Suite:")
message(STATUS "  cd ${INSTALL_PREFIX}")
message(STATUS "  ./solar_system_launcher --help")
