# Body Selection Standardization

## Overview

The Solar System Suite now provides standardized body selection across all applications to ensure consistent behavior and predictable performance characteristics.

## Standardized Body Sets

### Essential Bodies (9 bodies)
- **Contents**: Sun + 8 planets (Mercury, Venus, Earth, Mars, Jupiter, Saturn, Uranus, Neptune)
- **Performance**: Fastest - optimized for real-time applications and quick simulations
- **Use Cases**: Real-time monitoring, web interfaces, quick demonstrations
- **Applications**: `solar_system_realtime`, `solar_system_web` (default)

### Important Bodies (18 bodies)
- **Contents**: Essential bodies + major moons + dwarf planets
- **Includes**: Sun, 8 planets, Moon, Io, Europa, Ganymede, Callisto, Titan, Pluto, Charon, Triton
- **Performance**: Balanced - good compromise between completeness and performance
- **Use Cases**: Educational demonstrations, balanced simulations
- **Applications**: `solar_system_launcher` (default)

### Complete Bodies (27 bodies)
- **Contents**: All available celestial bodies including spacecraft
- **Includes**: All essential and important bodies plus additional moons, dwarf planets, and spacecraft
- **Performance**: Comprehensive but slower - full solar system simulation
- **Use Cases**: Research, comprehensive analysis, complete solar system modeling
- **Applications**: `solar_system` (default)

## Configuration Options

### Command Line Options

All applications now support the `--body-set` option:

```bash
# Use essential bodies (fastest)
./solar_system --body-set essential

# Use important bodies (balanced)
./solar_system_realtime --body-set important

# Use complete bodies (comprehensive)
./solar_system_launcher --body-set complete
```

### Application Defaults

Each application has a recommended default body set based on its typical use case:

| Application | Default Body Set | Reason |
|-------------|------------------|---------|
| `solar_system` | Complete | Comprehensive simulation and analysis |
| `solar_system_realtime` | Essential | Real-time performance requirements |
| `solar_system_launcher` | Important | Balanced demonstrations |
| `solar_system_web` | Essential | Web performance optimization |
| `solar_system_fetch` | Essential | Data validation testing |

### Custom Body Selection

You can still specify individual bodies using the existing `--bodies` option:

```bash
# Monitor specific bodies
./solar_system_realtime --bodies Sun,Earth,Moon,Mars

# This overrides the --body-set option
```

## Performance Characteristics

### Body Count vs Performance

| Body Set | Count | Typical Performance | Memory Usage | Use Case |
|----------|-------|-------------------|--------------|----------|
| Essential | 9 | < 0.1s simulation | ~2MB | Real-time, web |
| Important | 18 | 0.1-0.3s simulation | ~4MB | Balanced usage |
| Complete | 27 | 0.3-0.5s simulation | ~6MB | Research, analysis |

### Recommendations

- **Real-time applications**: Use Essential set for smooth performance
- **Educational tools**: Use Important set for good balance
- **Research simulations**: Use Complete set for comprehensive data
- **Web interfaces**: Use Essential set for responsive user experience

## Migration Guide

### For Existing Users

The standardization maintains backward compatibility:

1. **No command-line changes needed**: Applications work with existing commands
2. **Consistent behavior**: All applications now use predictable body selections
3. **Performance improvements**: Real-time applications are now faster by default
4. **Override capability**: Use `--body-set` to change defaults when needed

### For Developers

When integrating with the Solar System Suite:

```cpp
// Use standardized body factory methods
Bodies::BodyFactory factory;

// Create bodies using recommended set for your application
auto bodies = factory.create_default_bodies();

// Or specify a particular set
auto essential_bodies = factory.create_bodies_for_set(
    Bodies::BodyFactory::DefaultBodySet::ESSENTIAL);

// Use BodySelector with standardized sets
auto bodies = BodySelector()
    .body_set(Bodies::BodyFactory::DefaultBodySet::IMPORTANT)
    .build();
```

## Configuration Files

Body set preferences can be specified in configuration files:

```json
{
  "simulation": {
    "body_set": "important"
  },
  "realtime": {
    "body_set": "essential"
  }
}
```

## Troubleshooting

### Common Issues

1. **Performance too slow**: Try using `--body-set essential`
2. **Missing bodies in results**: Use `--body-set complete` or specify bodies with `--bodies`
3. **Inconsistent results between applications**: Check that both use the same body set

### Verification

To check which bodies are included in each set:

```bash
# List bodies in each set (when implemented)
./solar_system --list-body-sets

# Verify current configuration
./solar_system --verbose
```

## Future Enhancements

- Configuration file support for persistent body set preferences
- Custom body set definitions
- Performance profiling for optimal set selection
- Dynamic body set adjustment based on system capabilities
