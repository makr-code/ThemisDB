# Wave D D4-00 — Final Complete Dependency List

**Date:** 2026-08-17  
**Build Attempt:** 6 (FINAL)  
**Status:** 🟡 IN PROGRESS (with community-release preset)

## Complete System Package Installation

```bash
# One-command installation of ALL required packages for Wave D build:

sudo apt-get update && sudo apt-get install -y \
  libfmt-dev \
  libspdlog-dev \
  nlohmann-json3-dev \
  libmimalloc-dev \
  libyaml-cpp-dev \
  libcurl4-openssl-dev \
  libboost-dev \
  libtbb-dev \
  librocksdb-dev \
  libsnappy-dev
```

## Package Breakdown

### Core Libraries (Required)
1. **libfmt-dev** — C++ formatting library
2. **libspdlog-dev** — Structured logging
3. **nlohmann-json3-dev** — JSON parsing
4. **libmimalloc-dev** — Memory allocator

### Build Infrastructure (Required)
5. **libyaml-cpp-dev** — YAML configuration
6. **libcurl4-openssl-dev** — HTTP/TLS client
7. **libboost-dev** — Boost library suite

### Parallelization & Storage (Required)
8. **libtbb-dev** — Threading Building Blocks (parallel execution)
9. **librocksdb-dev** — RocksDB embedded database
10. **libsnappy-dev** — Snappy compression (RocksDB dependency)

## Build Configuration

**Recommended Preset:** `community-release` (not `community-release-allow-missing-rocksdb`)

Reason: The "allow-missing-rocksdb" preset is misleading:
- Allows CMake configuration without RocksDB
- BUT: Source code contains unconditional `#include <rocksdb/db.h>` directives
- Result: Build fails at compilation time even with preset flag
- Solution: Use standard `community-release` preset which requires all dependencies

## CMake Commands

### Configuration (after installing all packages)
```bash
cmake --preset community-release
```

### Build Execution
```bash
cmake --build --preset community-release --parallel 16
```

Build directory: `/home/runner/work/ThemisDB/ThemisDB/build-community-release`

## Why This Many Dependencies?

The Wave D build includes:
- **Storage layer** — RocksDB + Snappy compression
- **Parallelization** — TBB for multi-threaded processing
- **Configuration** — YAML for config files
- **Networking** — libcurl for HTTP/TLS
- **Logging** — spdlog for structured logs
- **JSON** — nlohmann-json for data interchange
- **Formatting** — fmt for string formatting
- **Memory** — mimalloc for optimized allocation
- **Boost** — Utilities library suite

## Future Reference

For v2.4.1 and beyond, consider:
1. Docker image with all dependencies pre-installed
2. Update SETUP.md with this complete list
3. Provide one-liner installation script
4. Document preset differences and when to use each

## Timeline Summary

- Attempt 1: yaml-cpp missing
- Attempt 2: libcurl missing
- Attempt 3: libboost missing
- Attempt 4: libtbb missing
- Attempt 5: RocksDB + Snappy missing + preset confusion
- Attempt 6: All 10 packages + correct preset (IN PROGRESS)

**Total time to resolve all dependencies:** ~50 minutes
**Total time to identify dependencies:** 4 failed builds
**Key lesson:** The preset name was misleading; the actual preset allows RocksDB-missing at CMake time but not at compile time
