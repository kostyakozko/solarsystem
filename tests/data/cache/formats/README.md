# Cache Format Samples

This directory contains cache files in different formats for testing format compatibility and conversion.

## Formats

- **JSON**: Human-readable JSON format (ephemeris_cache.json)
- **Binary**: Optimized binary format (ephemeris_cache.bin)
- **CSV**: Comma-separated values format (ephemeris_cache.csv)
- **Legacy**: Older format versions for backward compatibility testing

## Binary Format Specification

The binary cache format uses little-endian byte order:

```
Header (20 bytes):
- Magic: 4 bytes ("SOLR")
- Version: 4 bytes (uint32)
- Body Count: 4 bytes (uint32)
- Timestamp: 8 bytes (uint64)

Body Data (per body, 60 bytes):
- ID: 4 bytes (uint32)
- Mass: 8 bytes (double)
- Position X: 8 bytes (double)
- Position Y: 8 bytes (double)
- Position Z: 8 bytes (double)
- Velocity X: 8 bytes (double)
- Velocity Y: 8 bytes (double)
- Velocity Z: 8 bytes (double)
```

Total file size = 20 + (60 * body_count) bytes
