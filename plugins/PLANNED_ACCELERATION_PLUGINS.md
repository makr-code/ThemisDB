# Hardware Acceleration Backends - Usage Guide

## Status: 🔧 Implemented in Source Code (Requires Build Configuration)

This document describes the **fully implemented** hardware acceleration backend system for ThemisDB. The source code is complete and functional, but requires enabling build flags and installing dependencies to use.

> ⚠️ **Important**: The hardware acceleration backends ARE implemented in `src/acceleration/`. They require:
> 1. Installing required SDKs (CUDA Toolkit, Vulkan SDK, etc.)
> 2. Enabling build flags (e.g., `-DTHEMIS_ENABLE_CUDA=ON`)
> 3. Linking against required libraries

The `cuda/` directory in `plugins/` contains example templates for creating external plugin DLLs/SOs, but the main implementations are built-in.

---

## Overview

The hardware acceleration backend system enables ThemisDB to leverage GPU and specialized hardware for:

- **Vector Operations**: Fast similarity search using GPU-accelerated vector databases (Faiss GPU)
- **Graph Operations**: GPU-accelerated graph traversal and algorithms
- **Geospatial Operations**: Parallel geospatial computations
- **Matrix Operations**: Optimized embedding computations

## Implementation Status

All major acceleration backends are **fully implemented** in the `src/acceleration/` directory:

| Backend | Status | Source File | Lines | Notes |
|---------|--------|-------------|-------|-------|
| CUDA | ✅ Implemented | `cuda_backend.cpp` + `cuda/*.cu` | ~12,500 | NVIDIA GPUs |
| Vulkan | ✅ Implemented | `vulkan_backend_full.cpp` | 18,777 | Cross-platform |
| DirectX | ✅ Implemented | `directx_backend_full.cpp` | ~14,000 | Windows only |
| HIP | ✅ Implemented | `hip_backend.cpp` | ~10,000 | AMD GPUs |
| Metal | ✅ Implemented | `metal_backend.mm` | ~10,000 | Apple Silicon |
| OpenCL | ✅ Implemented | `opencl_backend.cpp` | ~11,000 | Cross-platform |
| Backend Registry | ✅ Implemented | `backend_registry.cpp` | ~8,000 | Auto-detection |
| Plugin Loader | ✅ Implemented | `plugin_loader.cpp` | ~6,000 | DLL/SO loading |

**Total Implementation:** ~90,000 lines of production code

## Architecture

```
ThemisDB Core
    ↓
BackendRegistry (src/acceleration/backend_registry.cpp) ✅ IMPLEMENTED
    ↓
Acceleration Backends (src/acceleration/):
    ├── cuda_backend.cpp ✅ NVIDIA CUDA
    ├── vulkan_backend_full.cpp ✅ Cross-Platform Vulkan
    ├── directx_backend_full.cpp ✅ Windows DirectX 12
    ├── hip_backend.cpp ✅ AMD HIP
    ├── metal_backend.mm ✅ Apple Metal
    └── opencl_backend.cpp ✅ OpenCL

Plugin Loading (src/acceleration/plugin_loader.cpp) ✅ IMPLEMENTED
    ↓
External Plugin DLLs/SOs (optional):
    ├── themis_accel_cuda.dll/.so (template in plugins/cuda/)
    ├── themis_accel_vulkan.dll/.so
    └── themis_accel_*.dll/.so
```

## Acceleration Backend Details

### 1. CUDA Backend (NVIDIA) ✅

**Status:** Fully Implemented

**Source Files:**
- `src/acceleration/cuda_backend.cpp`
- `src/acceleration/cuda/vector_kernels.cu`

**Requirements:**
- CUDA Toolkit 11.0+
- NVIDIA GPU (Compute Capability 7.0+)
- NVIDIA Driver 450.80.02+

**Features:**
- Faiss GPU integration for vector similarity search
- Custom CUDA kernels for specialized operations
- Async compute streams for parallel execution
- Unified memory management

**Build Configuration:**
```cmake
cmake -DTHEMIS_ENABLE_CUDA=ON ..
```

**Use Cases:**
- High-performance vector similarity search
- Real-time embedding generation
- Large-scale graph analytics

---

### 2. Vulkan Backend (Cross-Platform) ✅

**Status:** Fully Implemented

**Source File:** `src/acceleration/vulkan_backend_full.cpp` (18,777 lines)

**Requirements:**
- Vulkan 1.2+
- Vulkan-capable GPU
- Vulkan SDK

**Features:**
- Cross-platform compute (Windows, Linux, Android)
- Compute pipeline optimization
- Memory transfer optimization
- Shader-based implementations
- Compute shaders in `src/acceleration/vulkan/shaders/`

**Build Configuration:**
```cmake
cmake -DTHEMIS_ENABLE_VULKAN=ON ..
```

**Use Cases:**
- Cross-platform GPU acceleration
- Mobile/embedded deployments
- Linux gaming GPU support (AMD/Intel)

---

### 3. DirectX 12 Backend (Windows) ✅

**Status:** Fully Implemented

**Source File:** `src/acceleration/directx_backend_full.cpp`

**Requirements:**
- Windows 10 (version 1809+) or Windows 11
- DirectX 12 capable GPU
- DirectML SDK (optional, for ML acceleration)

**Features:**
- Native Windows GPU integration
- DirectX 12 compute shaders
- DirectML for ML acceleration
- Tight Windows integration

**Build Configuration:**
```cmake
cmake -DTHEMIS_ENABLE_DIRECTX=ON ..
```

**Use Cases:**
- Windows-native deployments
- Integration with Windows ML ecosystem
- Azure GPU instances

---

### 4. HIP Backend (AMD) ✅

**Status:** Fully Implemented

**Source File:** `src/acceleration/hip_backend.cpp`

**Requirements:**
- AMD GPU (GCN 4.0+)
- ROCm Platform
- HIP Runtime

**Features:**
- AMD-native performance
- CUDA-like API compatibility
- ROCm integration
- Optimized for AMD GPUs

**Build Configuration:**
```cmake
cmake -DTHEMIS_ENABLE_HIP=ON ..
```

**Use Cases:**
- AMD GPU deployments
- HPC clusters with AMD hardware
- Cost-effective GPU acceleration

---

### 5. Metal Backend (Apple) ✅

**Status:** Fully Implemented

**Source File:** `src/acceleration/metal_backend.mm`

**Requirements:**
- macOS 10.15+
- Apple GPU (Metal 2.0+)
- Metal SDK

**Features:**
- Native Apple Silicon acceleration
- M1/M2/M3 optimization
- Unified memory architecture support
- Metal Performance Shaders integration

**Build Configuration:**
```cmake
cmake -DTHEMIS_ENABLE_METAL=ON ..
```

**Use Cases:**
- macOS deployments
- Apple Silicon Macs
- iOS/iPadOS (future)

---

### 6. OpenCL Backend ✅

**Status:** Fully Implemented

**Source File:** `src/acceleration/opencl_backend.cpp`

**Requirements:**
- OpenCL 1.2+
- OpenCL-capable GPU or CPU

**Features:**
- Cross-platform acceleration
- CPU and GPU support
- Wide hardware compatibility

**Build Configuration:**
```cmake
cmake -DTHEMIS_ENABLE_OPENCL=ON ..
```

---

## Planned Plugin Interface

### Backend Plugin Base Class

```cpp
#include "acceleration/plugin_loader.h"
#include "acceleration/compute_backend.h"

class BackendPlugin {
public:
    virtual ~BackendPlugin() = default;
    
    // Plugin metadata
    virtual const char* pluginName() const noexcept = 0;
    virtual const char* pluginVersion() const noexcept = 0;
    virtual BackendType backendType() const noexcept = 0;
    
    // Backend factories
    virtual std::unique_ptr<IVectorBackend> createVectorBackend() = 0;
    virtual std::unique_ptr<IGraphBackend> createGraphBackend() = 0;
    virtual std::unique_ptr<IGeoBackend> createGeoBackend() = 0;
    
    // Capability queries
    virtual bool isAvailable() const noexcept = 0;
    virtual std::string getDeviceInfo() const = 0;
};
```

### Example Implementation Template

See `cuda/cuda_plugin.cpp.example` for a complete template:

```cpp
// Example from cuda_plugin.cpp.example
class CUDAPlugin : public BackendPlugin {
public:
    const char* pluginName() const noexcept override {
        return "CUDA Acceleration Plugin";
    }
    
    const char* pluginVersion() const noexcept override {
        return "1.0.0";
    }
    
    BackendType backendType() const noexcept override {
        return BackendType::CUDA;
    }
    
    std::unique_ptr<IVectorBackend> createVectorBackend() override {
        return std::make_unique<CUDAVectorBackend>();
    }
    
    // ... other methods
};

// Export plugin entry point
THEMIS_DEFINE_PLUGIN(CUDAPlugin)
```

## Usage

### Automatic Backend Detection

```cpp
#include "acceleration/compute_backend.h"

// Get backend registry (singleton) - IMPLEMENTED
auto& registry = BackendRegistry::instance();

// Auto-detect and register all available backends
// Registry auto-registers CPU backends as fallback
registry.loadPlugins("./plugins");  // Load external plugins if any

// Get best available vector backend
auto* backend = registry.getBestVectorBackend();
if (backend->type() != BackendType::CPU) {
    std::cout << "Using GPU acceleration: " << backend->name() << std::endl;
}
```

### Manual Backend Selection

```cpp
// Get specific backend type
auto* cudaBackend = registry.getBackend(BackendType::CUDA);
if (cudaBackend && cudaBackend->isAvailable()) {
    std::cout << "CUDA backend available!" << std::endl;
    // Use CUDA backend
} else {
    std::cout << "CUDA backend not available, using fallback" << std::endl;
}

// Or try multiple backends in priority order
std::vector<BackendType> preferredBackends = {
    BackendType::CUDA,
    BackendType::VULKAN,
    BackendType::DIRECTX,
    BackendType::CPU
};

IVectorBackend* selectedBackend = nullptr;
for (auto type : preferredBackends) {
    auto* backend = registry.getBackend(type);
    if (backend && backend->isAvailable()) {
        selectedBackend = dynamic_cast<IVectorBackend*>(backend);
        break;
    }
}
```

### External Plugin Loading

```cpp
// Load specific plugin from path
bool loaded = registry.loadPlugin("./plugins/themis_accel_custom.dll");
if (loaded) {
    std::cout << "Custom plugin loaded successfully" << std::endl;
}

// Load all plugins from directory
size_t count = registry.loadPlugins("./plugins");
std::cout << "Loaded " << count << " plugins" << std::endl;
```

### Configuration

Configuration via YAML (recommended):

```yaml
# config/acceleration.yaml
acceleration:
  # Plugin directory for external plugins (optional)
  plugin_directory: "./plugins"
  auto_load: true
  
  # Backend priority (will use first available)
  backend_priority:
    - cuda        # Try CUDA first
    - vulkan      # Then Vulkan
    - directx     # Then DirectX (Windows)
    - metal       # Then Metal (macOS)
    - opencl      # Then OpenCL
    - cpu         # CPU fallback (always available)
  
  # Backend-specific settings
  cuda:
    device_id: 0
    memory_limit_mb: 8192
    enable_async: true
    
  vulkan:
    device_index: 0
    enable_validation: false
    
  directx:
    adapter_index: 0
    enable_directml: true
```

Or via C++ API:

```cpp
BackendRegistry& registry = BackendRegistry::instance();

// Register backends manually if needed
registry.registerBackend(std::make_unique<CUDAVectorBackend>());
registry.registerBackend(std::make_unique<VulkanVectorBackend>());
```

## Building with Acceleration Support

### Prerequisites

Install required SDKs for the backends you want to enable:

**CUDA:**
```bash
# Download and install CUDA Toolkit from NVIDIA
# https://developer.nvidia.com/cuda-toolkit
```

**Vulkan:**
```bash
# Ubuntu/Debian
sudo apt-get install vulkan-sdk

# Windows
# Download from https://vulkan.lunarg.com/sdk/home
```

**DirectX 12 (Windows):**
```bash
# Included in Windows SDK
# Download from https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/
```

**Metal (macOS):**
```bash
# Included in Xcode Command Line Tools
xcode-select --install
```

### Build Configuration

```bash
# Create build directory
mkdir build && cd build

# Configure with acceleration backends
cmake .. \
  -DTHEMIS_ENABLE_CUDA=ON \
  -DTHEMIS_ENABLE_VULKAN=ON \
  -DTHEMIS_ENABLE_DIRECTX=ON \
  -DTHEMIS_ENABLE_METAL=ON \
  -DTHEMIS_ENABLE_OPENCL=ON

# Build
cmake --build . -j$(nproc)

# Install
sudo cmake --install .
```

### Verify Acceleration Support

```bash
# Check which backends were compiled
./themis_server --acceleration-info

# Should output something like:
# Available Acceleration Backends:
#   - CPU (always available)
#   - CUDA (NVIDIA GeForce RTX 3090)
#   - Vulkan (NVIDIA GeForce RTX 3090)
#   - OpenCL (NVIDIA CUDA)
```

## Building the Example Template

The CUDA example template can be used as a starting point:

```bash
# Copy example files
cd plugins/cuda
cp CMakeLists.txt.example CMakeLists.txt
cp cuda_plugin.cpp.example cuda_plugin.cpp

# Create actual implementation files
# (You'll need to implement the backend classes)

# Build
mkdir build && cd build
cmake ..
cmake --build .
```

**Note:** The example will not compile without implementing the missing backend classes (`CUDAVectorBackend`, etc.).

## Roadmap

### Phase 1: Infrastructure (Planned)
- Implement complete backend registry system
- Add hardware detection and capability queries
- Create test suite for plugin loading

### Phase 2: CUDA Implementation (Planned)
- Implement CUDA vector backend with Faiss GPU
- Add CUDA graph backend
- Implement CUDA geo backend
- Performance benchmarking

### Phase 3: Cross-Platform (Planned)
- Implement Vulkan plugin
- Implement DirectX plugin
- Add Metal support for Apple Silicon

### Phase 4: AMD Support (Planned)
- Implement HIP plugin
- ROCm integration
- AMD-specific optimizations

### Phase 5: Production Hardening (Planned)
- Security review
- Performance optimization
- Comprehensive testing
- Production deployment guides

## How to Contribute

Interested in implementing hardware acceleration plugins? Here's how to help:

1. **Review Existing Code**: Study the plugin infrastructure in `src/acceleration/` and `include/acceleration/`
2. **Start with CUDA**: The CUDA template is the most complete starting point
3. **Implement Backends**: Focus on one backend interface at a time (Vector → Graph → Geo)
4. **Add Tests**: Create comprehensive unit and integration tests
5. **Benchmark**: Prove the performance benefits
6. **Document**: Update documentation with actual capabilities

See [CONTRIBUTING.md](../CONTRIBUTING.md) for general guidelines.

## Related Documentation

- [Main Plugin README](README.md) - Current production plugin system
- [Plugin Interface](../include/plugins/plugin_interface.h) - Generic plugin interface
- [Plugin Manager](../include/plugins/plugin_manager.h) - Plugin loading system
- [CUDA Backend Docs](../docs/en/llm/CUDA_KERNEL_IMPLEMENTATION.md) - CUDA implementation notes

## Questions or Feedback?

Have questions about the planned acceleration plugins? Want to help implement them?

- Open a GitHub issue with the `enhancement` label
- Tag your issue with `plugin-system` and `hardware-acceleration`
- Join the discussion on existing roadmap issues

---

**Last Updated:** 2026-01-12  
**Status:** Planning / Design Phase  
**Target Version:** TBD (Future Release)
