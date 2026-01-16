# Solar System Suite - Windows Build Script
# Requires: Visual Studio 2022 Build Tools, CMake, vcpkg
#
# Usage:
#   .\scripts\build-windows.ps1                    # Default build
#   .\scripts\build-windows.ps1 -BuildType Debug   # Debug build
#   .\scripts\build-windows.ps1 -SharedLibs        # Build shared libraries
#   .\scripts\build-windows.ps1 -Clean             # Clean build
#   .\scripts\build-windows.ps1 -Test              # Build and run tests

param(
    [string]$BuildType = "Release",
    [switch]$SharedLibs,
    [switch]$Clean,
    [switch]$Test,
    [switch]$Install,
    [string]$VcpkgRoot = $env:VCPKG_ROOT
)

$ErrorActionPreference = "Stop"

# Colors for output
function Write-Info { Write-Host "[INFO] $args" -ForegroundColor Cyan }
function Write-Success { Write-Host "[OK] $args" -ForegroundColor Green }
function Write-Warning { Write-Host "[WARN] $args" -ForegroundColor Yellow }
function Write-Error { Write-Host "[ERROR] $args" -ForegroundColor Red }

Write-Info "Solar System Suite - Windows Build"
Write-Info "==================================="

# Check prerequisites
Write-Info "Checking prerequisites..."

# Check CMake
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Error "CMake not found. Install via: choco install cmake"
    exit 1
}
Write-Success "CMake found: $(cmake --version | Select-Object -First 1)"

# Check vcpkg
if (-not $VcpkgRoot) {
    # Try common locations
    $commonPaths = @(
        "C:\vcpkg",
        "$env:USERPROFILE\vcpkg",
        "$env:LOCALAPPDATA\vcpkg"
    )
    foreach ($path in $commonPaths) {
        if (Test-Path "$path\vcpkg.exe") {
            $VcpkgRoot = $path
            break
        }
    }
}

if (-not $VcpkgRoot -or -not (Test-Path "$VcpkgRoot\vcpkg.exe")) {
    Write-Warning "vcpkg not found. Dependencies may not be available."
    Write-Warning "Install vcpkg: git clone https://github.com/microsoft/vcpkg.git C:\vcpkg"
    Write-Warning "Then run: C:\vcpkg\bootstrap-vcpkg.bat"
    $UseVcpkg = $false
} else {
    Write-Success "vcpkg found: $VcpkgRoot"
    $UseVcpkg = $true
}

# Set build directory
$BuildDir = "build-windows"
if ($SharedLibs) {
    $BuildDir = "build-windows-shared"
}

# Clean if requested
if ($Clean -and (Test-Path $BuildDir)) {
    Write-Info "Cleaning build directory..."
    Remove-Item -Recurse -Force $BuildDir
}

# Create build directory
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

# Configure CMake options
$CMakeArgs = @(
    "-B", $BuildDir,
    "-DCMAKE_BUILD_TYPE=$BuildType"
)

if ($UseVcpkg) {
    $CMakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$VcpkgRoot\scripts\buildsystems\vcpkg.cmake"
}

if ($SharedLibs) {
    $CMakeArgs += "-DBUILD_SHARED_LIBS=ON"
} else {
    $CMakeArgs += "-DBUILD_SHARED_LIBS=OFF"
}

if ($Test) {
    $CMakeArgs += "-DENABLE_TESTING=ON"
}

# Configure
Write-Info "Configuring CMake..."
Write-Info "cmake $($CMakeArgs -join ' ')"
& cmake @CMakeArgs
if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed"
    exit 1
}
Write-Success "Configuration complete"

# Build
Write-Info "Building..."
$BuildArgs = @(
    "--build", $BuildDir,
    "--config", $BuildType,
    "--parallel"
)
& cmake @BuildArgs
if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed"
    exit 1
}
Write-Success "Build complete"

# Run tests if requested
if ($Test) {
    Write-Info "Running tests..."
    & ctest --test-dir $BuildDir --build-config $BuildType --output-on-failure --parallel
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Tests failed"
        exit 1
    }
    Write-Success "All tests passed"
}

# Install if requested
if ($Install) {
    Write-Info "Installing..."
    & cmake --install $BuildDir --config $BuildType
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Installation failed"
        exit 1
    }
    Write-Success "Installation complete"
}

Write-Info ""
Write-Success "Build completed successfully!"
Write-Info "Build directory: $BuildDir"
Write-Info "Build type: $BuildType"
Write-Info "Shared libraries: $SharedLibs"
