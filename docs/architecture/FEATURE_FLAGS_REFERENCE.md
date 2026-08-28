# ThemisDB Feature Flags Reference

**Version**: v1.4.0+  
**Status**: Comprehensive Feature Documentation

## Quick Reference Table

| Category | Flag | Default | Edition | Size Impact |
|----------|------|---------|---------|------------|
| **Core** | `THEMIS_ENABLE_GRPC` | Community: OFF | ENTERPRISE/HYPERSCALER: ON | +50 MB |
| | `THEMIS_ENABLE_LLM` | Community: OFF | HYPERSCALER: ON | +150 MB |
| | `THEMIS_ENABLE_GPU` | Community: OFF | HYPERSCALER: ON | +200 MB |
| | `THEMIS_ENABLE_CUDA` | OFF | Custom | +100 MB |
| | `THEMIS_ENABLE_TRACING` | Community: OFF | ENTERPRISE/HYPERSCALER: ON | +30 MB |
| **Protocols** | `THEMIS_ENABLE_HTTP2` | ON (default) | All | +20 MB |
| | `THEMIS_ENABLE_HTTP3` | OFF (experimental) | Custom | +40 MB |
| | `THEMIS_ENABLE_WEBSOCKET` | ON (default) | All | +10 MB |
| | `THEMIS_ENABLE_MQTT` | ON (default) | All | +15 MB |
| | `THEMIS_ENABLE_POSTGRES_WIRE` | ON (default) | All | +5 MB |
| | `THEMIS_ENABLE_MCP` | OFF | Custom | +8 MB |
| | `THEMIS_ENABLE_SSE` | ON (default) | All | +3 MB |
| **Optimization** | `THEMIS_ENABLE_DISKANN` | OFF | HYPERSCALER | +80 MB |
| | `THEMIS_ENABLE_WISCKEY` | ON (default) | All | Built-in |
| | `THEMIS_ENABLE_ARM_SIMD` | Auto-detected | ARM targets | Built-in |
| | `THEMIS_ENABLE_QNAP_ARM` | OFF | Custom | Built-in |
| | `THEMIS_ENABLE_MIMALLOC` | ON | All | Built-in |
| **Build** | `THEMIS_BUILD_TESTS` | Debug: ON / Release: OFF | All | +200 MB |
| | `THEMIS_BUILD_BENCHMARKS` | OFF | All | +300 MB |
| | `THEMIS_STRICT_BUILD` | OFF | Development | No change |
| | `THEMIS_ENABLE_ASAN` | OFF | Development | No change |

---

## Core Features (LLM, GPU, Sharding)

### THEMIS_ENABLE_GRPC

**Purpose**: Enable inter-shard communication via gRPC  
**Type**: `ON | OFF`  
**Default**: 
- MINIMAL: `OFF`
- COMMUNITY: `OFF`
- ENTERPRISE: `ON` (forced)
- HYPERSCALER: `ON` (forced)

**Dependencies**:
- gRPC++ library
- protobuf compiler
- shard_rpc.proto compilation

**Source Files Included**:
```
src/server/wal_grpc_service.cpp
src/sharding/shard_rpc_client.cpp
src/sharding/shard_rpc_server.cpp
proto/shard_rpc.proto (generates .pb.cc/.h/.grpc.pb.cc/.h)
```

**Build Impact**:
- Compilation time: +15 seconds
- Binary size: +50 MB
- Runtime: ~5% memory overhead

**Usage Example**:
```bash
# Enable gRPC for distributed sharding
cmake -S . -B build -DTHEMIS_ENABLE_GRPC=ON
```

**Related Flags**:
- `THEMIS_ENABLE_LLM` - Can use gRPC for distributed inference
- `THEMIS_ENABLE_TRACING` - gRPC calls are traced

---

### THEMIS_ENABLE_LLM

**Purpose**: Enable built-in LLM (Large Language Model) integration  
**Type**: `ON | OFF`  
**Default**:
- MINIMAL: `OFF` (forced)
- COMMUNITY: `OFF`
- ENTERPRISE: `OFF`
- HYPERSCALER: `ON` (forced)

**Dependencies**:
- llama.cpp (must be cloned to `./llama.cpp`)
- CUDA Toolkit (if `THEMIS_ENABLE_CUDA=ON`)
- CUDA Compute Capability 6.1+ (for GPU acceleration)

**Source Files Included** (~20 files):
```
src/llm/llm_inference_engine.cpp
src/llm/llm_embeddings.cpp
src/llm/llm_semantic_search.cpp
src/llm/llm_text_generation.cpp
src/llm/context_manager.cpp
src/llm/token_processor.cpp
src/llm/memory_pool.cpp
src/llm/cache_manager.cpp
src/server/llm_api_handler.cpp
src/llm/kernel_fusion.cu (if THEMIS_ENABLE_CUDA=ON)
```

**Compile Definitions**:
```cpp
THEMIS_LLM_ENABLED=1
THEMIS_LLM_VERSION="1.4.0"
```

**Build Impact**:
- Compilation time: +60 seconds
- Binary size: +150 MB (llama.cpp linked)
- Runtime: 2-4 GB VRAM (depends on model)

**Configuration in Code**:
```cpp
#if defined(THEMIS_LLM_ENABLED)
  LLMInferenceEngine engine(config);
#endif
```

**Setup Required**:
```powershell
# Clone llama.cpp if not present
git clone https://github.com/ggerganov/llama.cpp.git

# Then configure with LLM
cmake -S . -B build -DTHEMIS_ENABLE_LLM=ON
```

**Related Flags**:
- `THEMIS_ENABLE_CUDA` - GPU acceleration for LLM
- `THEMIS_ENABLE_GPU` - FAISS GPU vector search
- `THEMIS_ENABLE_GRPC` - Distributed LLM inference

---

### THEMIS_ENABLE_GPU

**Purpose**: Enable GPU acceleration (CUDA/HIP backends)  
**Type**: `ON | OFF`  
**Default**:
- MINIMAL: `OFF` (forced)
- COMMUNITY: `OFF`
- ENTERPRISE: `OFF`
- HYPERSCALER: `ON` (forced)

**Dependencies**:
- CUDA Toolkit 11.0+ (NVIDIA) OR
- HIP 5.0+ (AMD) OR
- Fallback: CPU acceleration

**Source Files Included**:
```
src/acceleration/cuda_backend.cpp
src/acceleration/hip_backend.cpp
src/gpu/vector_kernels.cu
src/gpu/distance_metrics.cu
src/gpu/quantization.cu
src/gpu/graph_operations.cu
src/gpu/bloom_filters.cu
src/acceleration/gpu_memory_manager.cpp
src/acceleration/gpu_stream_manager.cpp
```

**Compile Definitions**:
```cpp
THEMIS_GPU_ENABLED=1
THEMIS_GPU_BACKEND="CUDA|HIP"  // Depending on THEMIS_ENABLE_CUDA or THEMIS_ENABLE_HIP
```

**Build Impact**:
- Compilation time: +40 seconds (CUDA kernel compilation)
- Binary size: +200 MB (CUDA runtime libs)
- Runtime: 1-8 GB VRAM (depends on workload)

**Related Flags**:
- `THEMIS_ENABLE_CUDA` - NVIDIA CUDA support (required for NVIDIA GPUs)
- `THEMIS_ENABLE_HIP` - AMD HIP support (required for AMD GPUs)
- `THEMIS_ENABLE_DISKANN` - GPU-accelerated DiskANN indexing
- `THEMIS_ENABLE_LLM` - Can use GPU for inference

---

### THEMIS_ENABLE_CUDA

**Purpose**: Enable NVIDIA CUDA GPU support (subset of `THEMIS_ENABLE_GPU`)  
**Type**: `ON | OFF`  
**Default**: `OFF`

**Dependencies**:
- NVIDIA CUDA Toolkit 12.4+ installed
- NVIDIA GPU with Compute Capability 6.1+ (Tesla P100+)
- `THEMIS_ENABLE_GPU=ON` (automatically set if CUDA is ON)

**Environment Setup**:
```powershell
# Windows
$env:CUDA_TOOLKIT_ROOT_DIR = "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.4"

# Linux
export CUDA_HOME=/usr/local/cuda
```

**Configuration**:
```bash
cmake -S . -B build `
  -DTHEMIS_ENABLE_GPU=ON `
  -DTHEMIS_ENABLE_CUDA=ON `
  -DCUDA_TOOLKIT_ROOT_DIR="C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.4"
```

**Compile Definitions**:
```cpp
THEMIS_CUDA_ENABLED=1
THEMIS_CUDA_COMPUTE_CAPABILITY="6.1,7.0,8.0"  // Depends on GPU
```

**Build Impact**:
- Compilation time: +80 seconds (NVCC kernel compilation)
- Binary size: +100 MB (CUDA runtime libraries)
- Runtime: GPU-specific

---

### THEMIS_ENABLE_TRACING

**Purpose**: Enable OpenTelemetry observability (tracing, metrics, logs)  
**Type**: `ON | OFF`  
**Default**:
- MINIMAL: `OFF` (forced)
- COMMUNITY: `OFF`
- ENTERPRISE: `ON` (forced)
- HYPERSCALER: `ON` (forced)

**Dependencies**:
- opentelemetry-cpp
- opentelemetry-exporter-otlp-http (or grpc)

**Source Files Included**:
```
src/observability/tracer.cpp
src/observability/meter.cpp
src/observability/logger.cpp
src/observability/span_context.cpp
src/observability/attribute_builder.cpp
src/observability/export_manager.cpp
src/observability/prometheus_exporter.cpp
src/observability/jaeger_exporter.cpp
```

**Compile Definitions**:
```cpp
THEMIS_TRACING_ENABLED=1
THEMIS_METRICS_ENABLED=1
THEMIS_OTEL_ENDPOINT="http://localhost:4317"  // Default
```

**Environment Variables**:
```bash
OTEL_EXPORTER_OTLP_ENDPOINT=http://otel-collector:4317
OTEL_EXPORTER_OTLP_PROTOCOL=grpc
OTEL_SERVICE_NAME=themis_server
OTEL_LOG_LEVEL=info
```

**Build Impact**:
- Compilation time: +10 seconds
- Binary size: +30 MB
- Runtime: ~2% CPU overhead (depends on trace sampling)

---

## Protocol Features

### THEMIS_ENABLE_HTTP2

**Purpose**: HTTP/2 Protocol Support (multiplexing, server push)  
**Type**: `ON | OFF`  
**Default**: `ON`

**Dependencies**:
- nghttp2 library

**Source Files**:
```
src/server/http2_session.cpp
src/server/http2_settings.cpp
src/server/http2_stream_manager.cpp
```

**Compile Definitions**:
```cpp
THEMIS_HTTP2_ENABLED=1
```

**Build Impact**: +20 MB

---

### THEMIS_ENABLE_HTTP3

**Purpose**: HTTP/3 Protocol Support (experimental, QUIC-based)  
**Type**: `ON | OFF`  
**Default**: `OFF`

**Dependencies**:
- nghttp3 library
- ngtcp2 library (QUIC transport)

**Status**: Experimental - use with caution

**Build Impact**: +40 MB

**Note**: HTTP/3 requires separate configuration from HTTP/2. Can coexist.

---

### THEMIS_ENABLE_WEBSOCKET

**Purpose**: WebSocket Protocol Support (real-time bidirectional)  
**Type**: `ON | OFF`  
**Default**: `ON`

**Source Files**:
```
src/server/websocket_session.cpp
src/server/websocket_frame_handler.cpp
src/server/websocket_message_router.cpp
```

**Build Impact**: +10 MB

---

### THEMIS_ENABLE_MQTT

**Purpose**: MQTT Protocol Support (IoT, pub/sub)  
**Type**: `ON | OFF`  
**Default**: `ON`

**Source Files**:
```
src/server/mqtt_session.cpp
src/server/mqtt_broker.cpp
src/server/mqtt_topic_tree.cpp
```

**Build Impact**: +15 MB

---

### THEMIS_ENABLE_POSTGRES_WIRE

**Purpose**: PostgreSQL Wire Protocol Compatibility  
**Type**: `ON | OFF`  
**Default**: `ON`

**Source Files**:
```
src/server/postgres_session.cpp
src/server/postgres_statement_handler.cpp
src/server/postgres_result_formatter.cpp
```

**Build Impact**: +5 MB

**Effect**: Allows PostgreSQL clients (psql, JDBC, etc.) to connect to ThemisDB

---

### THEMIS_ENABLE_MCP

**Purpose**: Model Context Protocol Support (Claude/LLM integrations)  
**Type**: `ON | OFF`  
**Default**: `OFF`

**Source Files**:
```
src/server/mcp_server.cpp
src/server/mcp_resource_handler.cpp
src/server/mcp_tool_handler.cpp
```

**Build Impact**: +8 MB

---

### THEMIS_ENABLE_SSE

**Purpose**: Server-Sent Events Support (real-time streaming)  
**Type**: `ON | OFF`  
**Default**: `ON`

**Source Files**:
```
src/server/sse_connection_manager.cpp
src/server/sse_event_serializer.cpp
```

**Build Impact**: +3 MB

---

## Optimization Features

### THEMIS_ENABLE_DISKANN

**Purpose**: GPU-Accelerated DiskANN Vector Indexing  
**Type**: `ON | OFF`  
**Default**: `OFF`

**Dependencies**:
- FAISS library (GPU vector search)
- THEMIS_ENABLE_GPU=ON (requires GPU)

**Source Files**:
```
src/index/diskann_gpu_index.cpp
src/index/diskann_graph_builder.cpp
src/index/diskann_searcher.cpp
```

**Build Impact**:
- Compilation time: +30 seconds
- Binary size: +80 MB
- Runtime: 2-4 GB VRAM (depends on index size)

**Note**: Extremely fast vector search. Requires significant GPU memory.

---

### THEMIS_ENABLE_WISCKEY

**Purpose**: WiscKey Log Separation (RocksDB optimization)  
**Type**: `ON | OFF`  
**Default**: `ON`

**Effect**: Separates key-value storage: keys in LSM-tree, values in separate log file. Reduces write amplification.

**Build Impact**: Built-in (no additional code)

---

### THEMIS_ENABLE_ARM_SIMD

**Purpose**: ARM SIMD Vectorization (AArch64/NEON)  
**Type**: `ON | OFF`  
**Default**: Auto-detected on ARM platforms

**Compiler Flags**:
```
-march=armv8-a+simd
-mtune=cortex-a72  // Example
```

**Build Impact**: Built-in (no additional code)

---

### THEMIS_ENABLE_QNAP_ARM

**Purpose**: QNAP NAS ARM CPU Baseline Support  
**Type**: `ON | OFF`  
**Default**: `OFF`

**Effect**: Enables compatibility mode for older QNAP ARM processors (e.g., Cortex-A53)

**Compiler Flags**:
```
-march=armv8-a
-mcpu=cortex-a53  // Conservative baseline
```

**Build Impact**: Built-in

---

### THEMIS_ENABLE_MIMALLOC

**Purpose**: Microsoft mimalloc Memory Allocator (performance)  
**Type**: `ON | OFF`  
**Default**: `ON`

**Effect**: Replaces standard malloc with mimalloc for better performance

**Build Impact**: Built-in (+5 MB)

---

## Build Options

### THEMIS_BUILD_TESTS

**Purpose**: Compile unit test executables  
**Type**: `ON | OFF`  
**Default**: `ON`

**Effect**: Compiles ~180 test executables to `build-msvc/Release/test_*.exe`

**Build Impact**: +200 MB, +30 seconds

**Run Tests**:
```bash
ctest -C Release -j 8 --output-on-failure
```

---

### THEMIS_BUILD_BENCHMARKS

**Purpose**: Compile performance benchmarks  
**Type**: `ON | OFF`  
**Default**: `ON`

**Effect**: Compiles ~72 benchmark executables to `build-msvc/Release/bench_*.exe`

**Build Impact**: +300 MB, +60 seconds

**Run Benchmarks**:
```bash
.\build-msvc\Release\bench_comprehensive.exe --benchmark_time_unit=ms
```

---

### THEMIS_STRICT_BUILD

**Purpose**: Treat all compiler warnings as errors  
**Type**: `ON | OFF`  
**Default**: `OFF`

**Compiler Flags Added**:
- MSVC: `/WX`
- GCC/Clang: `-Werror`

**Build Impact**: May fail build on warnings (good for CI/CD)

---

### THEMIS_ENABLE_ASAN

**Purpose**: AddressSanitizer (memory error detection)  
**Type**: `ON | OFF`  
**Default**: `OFF`

**Compiler Flags**:
```
-fsanitize=address
-fno-optimize-sibling-calls
-g
```

**Build Impact**:
- Compilation time: +20 seconds
- Binary size: +50 MB
- Runtime: ~3x slower (for debugging)

**Usage**:
```bash
cmake --preset windows-vs2022-debug
# Automatically includes ASAN
```

---

## Edition-Based Presets

### MINIMAL Edition

```bash
cmake -S . -B build \
  -DTHEMIS_EDITION=MINIMAL \
  -DTHEMIS_ENABLE_LLM=OFF \
  -DTHEMIS_ENABLE_GRPC=OFF \
  -DTHEMIS_ENABLE_GPU=OFF \
  -DTHEMIS_BUILD_TESTS=OFF \
  -DTHEMIS_BUILD_BENCHMARKS=OFF
```

**Typical Binaries**: 50-80 MB (smallest)

### COMMUNITY Edition (Default)

```bash
cmake -S . -B build \
  -DTHEMIS_EDITION=COMMUNITY
```

**Features**: All optional
**Typical Binaries**: 150-200 MB

### ENTERPRISE Edition

```bash
cmake -S . -B build \
  -DTHEMIS_EDITION=ENTERPRISE \
  -DTHEMIS_ENABLE_GRPC=ON \
  -DTHEMIS_ENABLE_TRACING=ON
```

**Typical Binaries**: 200-250 MB

### HYPERSCALER Edition

```bash
cmake -S . -B build \
  -DTHEMIS_EDITION=HYPERSCALER \
  -DTHEMIS_ENABLE_LLM=ON \
  -DTHEMIS_ENABLE_GPU=ON \
  -DTHEMIS_ENABLE_GRPC=ON \
  -DTHEMIS_ENABLE_TRACING=ON
```

**Typical Binaries**: 400-500 MB (maximum features)

---

## Common Configurations

### Development (Full Debug)
```bash
cmake --preset windows-vs2022-debug
```

### Production (Standard)
```bash
cmake --preset windows-vs2022-release
```

### Production (with LLM)
```bash
cmake -S . -B build -DTHEMIS_EDITION=HYPERSCALER -DTHEMIS_ENABLE_LLM=ON
```

### IoT/Embedded (Minimal)
```bash
cmake -S . -B build -DTHEMIS_EDITION=MINIMAL
```

### Docker Multi-Stage
```dockerfile
# Build stage
FROM ubuntu:22.04 as builder
RUN cmake -S . -B build -DTHEMIS_EDITION=HYPERSCALER
RUN cmake --build build

# Runtime stage (minimal)
FROM ubuntu:22.04
COPY --from=builder /build/themis_server /usr/bin/
```

---

## Dependency Tree

```
THEMIS_ENABLE_LLM
  ├─ llama.cpp (REQUIRED)
  └─ THEMIS_ENABLE_CUDA (OPTIONAL)
      └─ CUDA Toolkit (REQUIRED if ON)

THEMIS_ENABLE_GPU
  ├─ THEMIS_ENABLE_CUDA (NVIDIA) OR
  ├─ THEMIS_ENABLE_HIP (AMD) OR
  └─ Fallback: CPU (if neither)

THEMIS_ENABLE_GRPC
  ├─ gRPC++ (REQUIRED)
  ├─ protobuf (REQUIRED)
  └─ shard_rpc.proto (REQUIRED)

THEMIS_ENABLE_HTTP3
  ├─ nghttp3 (REQUIRED)
  └─ ngtcp2 (REQUIRED)

THEMIS_ENABLE_DISKANN
  ├─ THEMIS_ENABLE_GPU (REQUIRED)
  └─ FAISS (REQUIRED)
```

---

## Troubleshooting

### Feature Enabled But Not Loaded

**Problem**: Flag set to ON, but feature not available at runtime

**Check**:
```cpp
std::cout << "CUDA: " << (THEMIS_CUDA_ENABLED ? "YES" : "NO") << std::endl;
std::cout << "LLM: " << (THEMIS_LLM_ENABLED ? "YES" : "NO") << std::endl;
```

Or via command line:
```bash
./themis_server --version --features
```

### Binary Size Too Large

**Problem**: Binary > 500 MB

**Solution**: Use MINIMAL or COMMUNITY edition without unnecessary features

### Compile Takes Too Long

**Problem**: Build > 300 seconds

**Solution**:
1. Reduce features (disable LLM, GPU)
2. Increase parallel jobs: `--parallel 16`
3. Use Ninja instead of Visual Studio generator

### Missing Dependencies

**Problem**: "Could not find package X"

**Solution**:
1. Ensure vcpkg is initialized: `./bootstrap-vcpkg.bat`
2. Clear CMake cache: `cmake --fresh`
3. Check VCPKG_ROOT environment variable

---

## References

- [CMake Modular Architecture](CMAKE_MODULAR_ARCHITECTURE.md)
- [Windows Build Guide](../build-guide/BUILD_WINDOWS.md)
- [Linux Build Guide](../build-guide/BUILD_LINUX.md)
- [Feature Implementation Guide](../development/feature_implementation.md)
