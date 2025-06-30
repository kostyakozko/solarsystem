# Solar System Suite Installation Guide

## System Requirements

### Minimum Requirements
- **Operating System**: macOS 10.12+, Linux (Ubuntu 18.04+, CentOS 7+), Windows 10+
- **CPU**: x86_64 architecture
- **RAM**: 512 MB available memory
- **Storage**: 100 MB free space
- **Network**: Internet connection for JPL data fetching

### Recommended Requirements
- **CPU**: Multi-core processor (for parallel JPL fetching)
- **RAM**: 2 GB available memory
- **Storage**: 1 GB free space (for cache and logs)
- **Network**: Broadband connection

## Prerequisites

### macOS
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake curl
```

### Ubuntu/Debian
```bash
# Update package list
sudo apt update

# Install build tools and dependencies
sudo apt install -y build-essential cmake libcurl4-openssl-dev git

# For documentation generation (optional)
sudo apt install -y doxygen graphviz
```

### CentOS/RHEL/Fedora
```bash
# CentOS/RHEL 7+
sudo yum groupinstall -y "Development Tools"
sudo yum install -y cmake3 libcurl-devel git

# Fedora
sudo dnf groupinstall -y "Development Tools"
sudo dnf install -y cmake libcurl-devel git
```

### Windows
1. **Install Visual Studio 2017+** with C++ support
2. **Install CMake** from https://cmake.org/download/
3. **Install Git** from https://git-scm.com/download/win
4. **Install curl** (usually included with Git)

## Installation Methods

### Method 1: Standard Installation (Recommended)

```bash
# Clone the repository
git clone <repository-url>
cd solarsystem

# Create build directory
mkdir build && cd build

# Configure build
cmake ..

# Build (use appropriate number of cores)
make -j$(nproc)  # Linux/macOS
# OR
make -j4        # Specific core count

# Install to default location (./install/)
make install
```

### Method 2: Custom Installation Directory

```bash
# Set custom installation directory
export SOLAR_SYSTEM_INSTALL_DIR=/opt/solar_system

# Configure and build
mkdir build && cd build
cmake ..
make -j$(nproc)
make install

# Verify installation
ls -la /opt/solar_system/
```

### Method 3: System-Wide Installation

```bash
# Install to system directories (requires sudo)
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
make -j$(nproc)
sudo make install

# Add to PATH (add to ~/.bashrc or ~/.zshrc)
export PATH="/usr/local/bin:$PATH"
```

### Method 4: Development Build

```bash
# Debug build with development features
mkdir build-dev && cd build-dev
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_PROFILING=ON ..
make -j$(nproc)

# No installation needed for development
# Run directly from build directory
./apps/solar_system_web/solar_system_web --help
```

## Installation Verification

### Basic Verification
```bash
# Navigate to installation directory
cd /path/to/install

# Check system status
./solar_system_launcher --status

# Expected output:
# === Solar System Suite Status ===
# ✓ Data Status: READY
# ✓ Available Applications: 5 applications found
```

### Comprehensive Testing
```bash
# Test data fetching
./solar_system_fetch --test-storage

# Test simulation
./solar_system --date 2025-01-01

# Test web server (in background)
./solar_system_web --port 8080 &
curl http://localhost:8080/api/status
pkill -f solar_system_web
```

## Post-Installation Setup

### Environment Configuration

**Add to PATH** (optional but recommended):
```bash
# Add to ~/.bashrc, ~/.zshrc, or ~/.profile
export PATH="/path/to/solar_system/install:$PATH"

# Reload shell configuration
source ~/.bashrc  # or ~/.zshrc
```

**Create Aliases** (optional):
```bash
# Add convenient aliases
alias ss-status='solar_system_launcher --status'
alias ss-update='solar_system_fetch --update'
alias ss-web='solar_system_web'
alias ss-realtime='solar_system_realtime'
```

### Initial Data Setup

```bash
# Update JPL data for current year
./solar_system_launcher --fetch --update

# This will:
# 1. Fetch data for all 27 celestial bodies
# 2. Create binary and JSON cache files
# 3. Validate data integrity
# 4. Take 30-60 seconds on first run
```

### Web Server Setup

**Development Setup**:
```bash
# Start web server for local use
./solar_system_web --port 8080

# Access at: http://localhost:8080
```

**Production Setup**:
```bash
# Create systemd service (Linux)
sudo tee /etc/systemd/system/solar-system-web.service > /dev/null <<EOF
[Unit]
Description=Solar System Web Server
After=network.target

[Service]
Type=simple
User=solar
WorkingDirectory=/opt/solar_system
ExecStart=/opt/solar_system/bin/solar_system_web --port 8080
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF

# Enable and start service
sudo systemctl enable solar-system-web
sudo systemctl start solar-system-web
```

## Directory Structure After Installation

```
install/
├── 🎛️ solar_system_launcher          # Main entry point
├── 📁 bin/                           # All executables
│   ├── solar_system                  # Batch simulation
│   ├── solar_system_fetch            # Data management
│   ├── solar_system_launcher         # Unified interface
│   ├── solar_system_realtime         # Live tracking
│   └── solar_system_web              # Web server
├── 📁 share/solar_system/            # Documentation and web files
│   ├── web/                          # Web interface files
│   │   ├── index.html                # Main web interface
│   │   ├── simple.html               # Lightweight interface
│   │   └── *.js                      # JavaScript files
│   └── README.md                     # Installation guide
└── 📄 USAGE.txt                      # Quick usage guide
```

## Configuration Options

### Build Configuration

**CMake Options**:
```bash
# Custom installation directory
cmake -DSOLAR_SYSTEM_INSTALL_DIR=/custom/path ..

# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release with debug info
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..

# Enable profiling
cmake -DENABLE_PROFILING=ON ..
```

**Compiler Optimization**:
```bash
# Maximum optimization (default for Release)
cmake -DCMAKE_CXX_FLAGS="-O3 -march=native -mtune=native" ..

# Conservative optimization (for older CPUs)
cmake -DCMAKE_CXX_FLAGS="-O2" ..
```

### Runtime Configuration

**Web Server Options**:
```bash
# Custom port
./solar_system_web --port 3000

# Custom web root
./solar_system_web --web-root /path/to/web/files

# Verbose logging
./solar_system_web --verbose

# Disable CORS (for local development)
./solar_system_web --no-cors
```

**Cache Configuration**:
```bash
# Custom cache location (if implemented)
export SOLAR_SYSTEM_CACHE_DIR=/var/cache/solar_system

# Cache validation frequency
./solar_system_fetch --validate  # Manual validation
```

## Deployment Scenarios

### Scenario 1: Personal Desktop

**Installation**:
```bash
# Install to user directory
export SOLAR_SYSTEM_INSTALL_DIR=$HOME/solar_system
mkdir build && cd build
cmake ..
make -j$(nproc) && make install
```

**Usage**:
- Run applications directly from installation directory
- Use web interface for exploration and education
- Update data monthly

### Scenario 2: Educational Institution

**Installation**:
```bash
# System-wide installation
sudo mkdir -p /opt/solar_system
sudo chown $USER:$USER /opt/solar_system
export SOLAR_SYSTEM_INSTALL_DIR=/opt/solar_system
mkdir build && cd build
cmake ..
make -j$(nproc) && make install
```

**Setup**:
- Create shared user account for web server
- Set up automatic data updates via cron
- Configure firewall for web access

### Scenario 3: Research Environment

**Installation**:
```bash
# High-performance build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native -flto" ..
make -j$(nproc) && make install
```

**Configuration**:
- Use batch processing scripts
- Set up data pipelines
- Monitor performance metrics

### Scenario 4: Web Service

**Installation**:
```bash
# Production web service
export SOLAR_SYSTEM_INSTALL_DIR=/srv/solar_system
mkdir build && cd build
cmake ..
make -j$(nproc) && make install
```

**Deployment**:
- Use reverse proxy (nginx/Apache)
- Set up SSL certificates
- Configure monitoring and logging
- Implement backup strategies

## Troubleshooting Installation

### Common Build Issues

**CMake version too old**:
```bash
# Error: CMake 3.15 or higher is required
# Solution: Install newer CMake
wget https://github.com/Kitware/CMake/releases/download/v3.25.0/cmake-3.25.0-linux-x86_64.sh
chmod +x cmake-3.25.0-linux-x86_64.sh
sudo ./cmake-3.25.0-linux-x86_64.sh --prefix=/usr/local --skip-license
```

**Compiler not found**:
```bash
# Error: No CMAKE_CXX_COMPILER could be found
# Solution: Install build tools
sudo apt install build-essential  # Ubuntu/Debian
sudo yum groupinstall "Development Tools"  # CentOS/RHEL
```

**curl development headers missing**:
```bash
# Error: Could NOT find CURL
# Solution: Install curl development package
sudo apt install libcurl4-openssl-dev  # Ubuntu/Debian
sudo yum install libcurl-devel  # CentOS/RHEL
```

### Runtime Issues

**Permission denied**:
```bash
# Error: Permission denied when running applications
# Solution: Check file permissions
chmod +x /path/to/install/bin/*
```

**Port already in use**:
```bash
# Error: Failed to bind socket to port 8080
# Solution: Use different port or kill existing process
./solar_system_web --port 8081
# OR
sudo lsof -i :8080  # Find process using port
kill <PID>
```

**Cache directory not writable**:
```bash
# Error: Failed to write cache files
# Solution: Check directory permissions
mkdir -p ~/.solar_system_cache
chmod 755 ~/.solar_system_cache
```

## Uninstallation

### Remove Installation
```bash
# Remove installation directory
rm -rf /path/to/solar_system/install

# Remove from PATH (edit ~/.bashrc or ~/.zshrc)
# Remove the export PATH line

# Remove cache files (optional)
rm -f ephemeris_cache.bin ephemeris_data.json
```

### Clean Build
```bash
# Remove build directory
rm -rf build/

# Remove any generated files
git clean -fdx  # If using git (be careful!)
```

## Maintenance

### Regular Updates
```bash
# Update JPL data monthly
./solar_system_fetch --update

# Validate cache integrity quarterly
./solar_system_fetch --validate

# Check system status regularly
./solar_system_launcher --status
```

### Performance Monitoring
```bash
# Monitor cache performance
time ./solar_system_fetch --update  # Should be ~0.008s after first run

# Monitor web server performance
curl -w "@curl-format.txt" -o /dev/null -s http://localhost:8080/api/status
```

### Backup Strategy
```bash
# Backup cache files
cp ephemeris_cache.bin ephemeris_cache.bin.backup
cp ephemeris_data.json ephemeris_data.json.backup

# Backup configuration
tar -czf solar_system_backup.tar.gz /path/to/install/
```

This installation guide provides comprehensive coverage for all deployment scenarios. For specific issues not covered here, refer to the troubleshooting section or consult the developer documentation.
