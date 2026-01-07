# Build Information and Module Reporting Feature

## Overview

ThemisDB now displays comprehensive build configuration and module information at server startup and provides programmatic access via REST API endpoints.

## Purpose

This feature addresses the requirement to clearly show:
1. **Edition Information** - Which edition (Community/Enterprise/Hyperscaler) is running
2. **Compiled Modules** - Which modules are compiled into the binary (THEMIS_* flags)
3. **Runtime Configuration** - Which modules are enabled via configuration

## Features

### 1. Startup Information Display

When the ThemisDB server starts, it displays:

```
===============================================================================
                      THEMIS DATABASE BUILD CONFIGURATION                       
===============================================================================

EDITION INFORMATION:
  Edition:            Community (COMMUNITY)
  GPU VRAM Limit:     24 GB
  Max Shard Nodes:    1

BUILD INFORMATION:
  Compiler:           GCC 11.4.0
  Build Type:         Release
  Build Timestamp:    Jan  7 2026 10:30:45
  Version:            1.3.5

COMPILED MODULES:
  Total Modules:      45
  Compiled In:        12
  Not Compiled:       33

  Enabled Modules:
    ✓ Storage Engine              - RocksDB-based storage with MVCC and transactions
    ✓ Vector Index                - HNSW vector search with optional GPU acceleration
    ✓ Graph Index                 - Graph analytics and traversal
    ✓ Secondary Index             - Fast secondary index lookups
    ✓ gRPC Protocol               - gRPC for inter-shard communication
    ✓ mimalloc Allocator          - High-performance memory allocator (+20-40% boost)
    ✓ OpenTelemetry Tracing       - Distributed tracing and observability

  Disabled Modules:
    ✗ CUDA Backend                - NVIDIA CUDA acceleration
    ✗ HIP Backend                 - AMD HIP/ROCm acceleration
    ✗ LLM Integration             - Large Language Model integration (llama.cpp)
    ✗ Voice Assistant             - Speech-to-Text and Text-to-Speech
    ...

===============================================================================
```

### 2. REST API Endpoints

#### GET /version

Returns comprehensive version and build information in JSON format:

```json
{
  "version": "1.3.5",
  "edition": {
    "name": "COMMUNITY",
    "type": "Community",
    "gpu_max_vram_gb": 24,
    "sharding_max_nodes": 1
  },
  "build": {
    "compiler": "GCC",
    "compiler_version": "11.4.0",
    "build_type": "Release",
    "timestamp": "Jan  7 2026 10:30:45"
  },
  "modules": {
    "compiled_in": [
      {
        "name": "Storage Engine",
        "description": "RocksDB-based storage with MVCC and transactions"
      },
      ...
    ],
    "not_compiled": [
      {
        "name": "CUDA Backend",
        "description": "NVIDIA CUDA acceleration"
      },
      ...
    ],
    "total": 45,
    "compiled_count": 12,
    "disabled_count": 33
  }
}
```

#### GET /api/capabilities

Now includes edition and build information along with feature capabilities:

```json
{
  "edition": {
    "name": "COMMUNITY",
    "type": "Community",
    "gpu_max_vram_gb": 24,
    "sharding_max_nodes": 1
  },
  "build": {
    "compiler": "GCC",
    "version": "11.4.0",
    "type": "Release"
  },
  "geo": {
    "enabled": true,
    "enterprise_compiled": false,
    ...
  },
  "vector": {
    "gpu_compiled": false
  },
  ...
}
```

## Technical Details

### Implementation

The feature is implemented in three main components:

1. **include/themis/build_info.h** - Header file defining the build information API
2. **src/utils/build_info.cpp** - Implementation collecting compile-time flags and edition info
3. **Integration in main_server.cpp and http_server.cpp** - Display and API endpoint integration

### Compile-Time Detection

The system detects compile-time flags using preprocessor directives:

```cpp
#ifdef THEMIS_ENABLE_CUDA
    config.modules.push_back({
        "CUDA Backend",
        true,  // compiled_in
        true,  // runtime_enabled (could be checked from config)
        "NVIDIA CUDA acceleration"
    });
#else
    config.modules.push_back({
        "CUDA Backend",
        false, // not compiled
        false,
        "NVIDIA CUDA acceleration"
    });
#endif
```

### Module Categories

The system tracks modules in several categories:

- **Core Modules** - Always present (Storage, Indexes)
- **GPU Acceleration** - CUDA, HIP, OpenCL, Vulkan, DirectX, Metal, OneAPI
- **LLM and Voice** - LLM integration, Voice Assistant, Whisper, Piper TTS
- **Content Processors** - Audio, image, video, geo, CAD processing
- **Network Protocols** - HTTP/2, HTTP/3, gRPC, WebSocket
- **Performance** - mimalloc, Huge Pages, RCU Index
- **Advanced Storage** - LIRS Cache, WiscKey, RaBitQ, DiskANN
- **Security** - HSM PKCS#11
- **Observability** - OpenTelemetry Tracing

## Usage Examples

### Checking Build Configuration via API

```bash
# Get version and build info
curl http://localhost:8765/version

# Get capabilities including edition
curl http://localhost:8765/api/capabilities
```

### Checking at Server Startup

Simply start the server and check the logs:

```bash
./themis_server --config config/config.yaml
```

The build configuration will be logged immediately after the version banner.

## Benefits

1. **Transparency** - Clear visibility into what features are compiled and available
2. **Debugging** - Easier troubleshooting when users report issues
3. **Edition Verification** - Quickly verify which edition is running
4. **Configuration Validation** - Check if required modules are available before configuring features
5. **Automation** - Scripts can query /version endpoint to verify deployment configuration

## Future Enhancements

Possible future improvements:

1. **Runtime Configuration Status** - Show which modules are enabled via config vs just compiled
2. **Module Dependencies** - Show dependencies between modules
3. **License Information** - Display license status for Enterprise/Hyperscaler editions
4. **Build Reproducibility** - Include git commit hash and build environment details
5. **Module Version Details** - Show version of individual third-party libraries (RocksDB, llama.cpp, etc.)
