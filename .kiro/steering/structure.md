# Solar System Suite Project Structure

## Root Directory Layout

```
solarsystem/
├── 📁 lib/                          # Modular static libraries
├── 📁 apps/                         # Specialized applications
├── 📁 docs/                         # Comprehensive documentation
├── 📁 tests/                        # Testing framework
├── 📁 examples/                     # Usage examples
├── 📁 cmake/                        # Build configuration
├── 📁 install/                      # Default installation target
├── 📁 build/                        # Out-of-source build directory
├── 📁 cache/                        # Runtime cache directory
├── CMakeLists.txt                   # Root build configuration
├── README.md                        # Main project documentation
└── .clang-format                    # Code formatting rules
```

## Library Architecture (`lib/`)

### Modular Design
```
lib/
├── solar_core/                      # Physics simulation engine
│   ├── include/                     # Public headers
│   ├── src/                         # Implementation files
│   └── CMakeLists.txt              # Build configuration
├── solar_jpl/                       # JPL HORIZONS integration
│   ├── include/                     # Public headers
│   ├── src/                         # Implementation files
│   ├── cache/                       # JPL data cache
│   └── CMakeLists.txt              # Build configuration
├── solar_utils/                     # Shared utilities
│   ├── include/                     # Public headers
│   ├── src/                         # Implementation files
│   └── CMakeLists.txt              # Build configuration
└── CMakeLists.txt                  # Library coordination
```

### Library Dependencies
- **solar_utils**: Base utilities (no dependencies)
- **solar_jpl**: JPL integration (depends on solar_utils)
- **solar_core**: Simulation engine (depends on solar_utils, solar_jpl)

## Application Architecture (`apps/`)

### Application Structure
```
apps/
├── solar_system/                    # Batch simulation
├── solar_system_fetch/              # Data management
├── solar_system_launcher/           # Unified interface
├── solar_system_realtime/           # Live tracking
├── solar_system_web/                # Web visualization
│   ├── src/                         # C++ web server
│   ├── web/                         # HTML/JS/CSS files
│   └── static/                      # Static assets
└── CMakeLists.txt                  # Application coordination
```

### Application Roles
- **launcher**: Entry point and workflow coordinator
- **fetch**: JPL data management and caching
- **solar_system**: High-performance batch simulation
- **realtime**: Continuous live tracking
- **web**: Interactive web interface with time travel

## Documentation Structure (`docs/`)

```
docs/
├── api/                             # Generated API documentation
│   ├── html/                        # Doxygen HTML output
│   └── latex/                       # Doxygen LaTeX output
├── architecture/                    # System design documents
├── developer/                       # Development guides
├── user-guide/                      # User documentation
├── examples/                        # Usage examples
└── diagrams/                        # Architecture diagrams
```

## Testing Framework (`tests/`)

```
tests/
├── unit/                            # Unit tests for libraries
├── integration/                     # End-to-end testing
├── benchmarks/                      # Performance testing
├── utils/                           # Test utilities
├── scripts/                         # Test automation
└── CMakeLists.txt                  # Test configuration
```

## Installation Layout

### Default Installation (`install/`)
```
install/
├── solar_system_launcher            # Main entry point
├── bin/                             # All executables
│   ├── solar_system
│   ├── solar_system_fetch
│   ├── solar_system_launcher
│   ├── solar_system_realtime
│   └── solar_system_web
├── lib/                             # Static libraries
├── include/                         # Header files
├── share/solar_system/              # Documentation and web files
│   └── web/                         # Web interface files
├── cache/                           # Runtime cache directory
└── USAGE.txt                        # Installation guide
```

## File Naming Conventions

### Source Files
- **Headers**: `.h` extension (C++ headers)
- **Implementation**: `.cpp` extension
- **CMake**: `CMakeLists.txt` (exact case)

### Application Naming
- **Executables**: `solar_system_*` pattern
- **Libraries**: `solar_*` pattern (without system)
- **Directories**: Snake_case matching executable names

### Documentation
- **Markdown**: `.md` extension, UPPERCASE for major docs
- **Generated**: `html/` and `latex/` subdirectories
- **Examples**: Descriptive names in `examples/`

## Build Artifacts

### Generated Directories (Git Ignored)
```
build/                               # CMake build directory
install/                             # Installation output
cache/                               # Runtime cache files
docs/api/html/                       # Generated documentation
docs/api/latex/                      # Generated documentation
```

### Cache Files
- `ephemeris_cache.bin` - Binary cache (fastest)
- `ephemeris_data.json` - JSON cache (human-readable)
- `cache/` directory - Additional cache storage

## Development Workflow

### Standard Development Process
1. **Modify**: Edit source files in `lib/` or `apps/`
2. **Format**: Run `make format` or `./format-code.sh`
3. **Build**: `cd build && make`
4. **Test**: Run specific applications or test suite
5. **Install**: `make install` to update installation

### Adding New Features
- **New Library**: Add to `lib/` with CMakeLists.txt
- **New Application**: Add to `apps/` with CMakeLists.txt
- **New Tests**: Add to appropriate `tests/` subdirectory
- **Documentation**: Update relevant docs in `docs/`

## Key Configuration Files

- **CMakeLists.txt**: Build system configuration
- **.clang-format**: Code formatting rules (Google style, 100 columns)
- **README.md**: Main project documentation and quick start
- **Doxyfile**: API documentation generation settings
- **.gitignore**: Version control exclusions (build/, cache/, etc.)

## Data Flow Patterns

### Library Dependencies Flow
```
Applications → solar_core → solar_jpl → solar_utils
                     ↓           ↓           ↓
                 Simulation   JPL Data   Utilities
```

### Installation Flow
```
Source → Build → Install → Runtime
  ↓        ↓        ↓         ↓
lib/   build/   install/   cache/
apps/           bin/       data/
```
