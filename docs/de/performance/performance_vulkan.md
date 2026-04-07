# Vulkan Compute Backend

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** ⚡ Performance

---

## 📑 Table of Contents

- [Status](#status--partial-implementation-shaders-ready)
- [Architecture](#architecture)
- [Implementation](#implementation)

---

## Status: 🚧 Partial Implementation (Shaders Ready)

Vulkan compute shaders are implemented and ready. Full C++ backend integration requires Vulkan SDK.

## Features

### Implemented
- ✅ **Compute Shaders** - GLSL shaders for L2 and Cosine distance
- ✅ **Shader Source** - Located in `src/acceleration/vulkan/shaders/`
- ✅ **Backend Stub** - C++ skeleton ready for integration

### Pending
- ⏳ **Vulkan Loader** - Dynamic library loading
- ⏳ **Compute Pipeline** - Pipeline creation and management
- ⏳ **Buffer Management** - Staging and device buffers
- ⏳ **Command Buffers** - Compute command recording
- ⏳ **Synchronization** - Fences and semaphores

## Compute Shaders

### L2 Distance Shader

**File:** `src/acceleration/vulkan/shaders/l2_distance.comp`

```glsl
#version 450
layout(local_size_x = 16, local_size_y = 16) in;

// Computes Euclidean distance
// Result: sqrt(sum((q - v)^2))
```

**Compilation:**
```bash
glslangValidator -V l2_distance.comp -o l2_distance.spv
# or
glslc l2_distance.comp -o l2_distance.spv
```

### Cosine Distance Shader

**File:** `src/acceleration/vulkan/shaders/cosine_distance.comp`

```glsl
#version 450
layout(local_size_x = 16, local_size_y = 16) in;

// Computes cosine distance
// Result: 1 - (dot(q, v) / (||q|| * ||v||))
```

## Hardware Requirements

**Minimum:**
- Vulkan 1.2+ capable GPU
- Vulkan SDK installed
- Compute queue support
- 4GB VRAM

**Supported Platforms:**
- ✅ Windows 10/11
- ✅ Linux (Ubuntu 20.04+)
- ✅ Android (API 29+)
- ⚠️ macOS (via MoltenVK)

**GPU Vendors:**
- ✅ NVIDIA (all modern GPUs)
- ✅ AMD (RX 5000+)
- ✅ Intel (Xe Graphics)

## Build Instructions

```bash
# Install Vulkan SDK
# https://vulkan.lunarg.com/sdk/home

# Compile shaders
cd src/acceleration/vulkan/shaders
glslangValidator -V l2_distance.comp -o l2_distance.spv
glslangValidator -V cosine_distance.comp -o cosine_distance.spv

# Build with Vulkan support
cmake -S . -B build \
  -DTHEMIS_ENABLE_VULKAN=ON \
  -DVULKAN_SDK=/path/to/vulkan/sdk

cmake --build build
```

## Implementation Roadmap

### Phase 1: Core Integration (4 weeks)
- [ ] Load Vulkan library dynamically
- [ ] Create Vulkan instance
- [ ] Enumerate and select physical device
- [ ] Create logical device with compute queue
- [ ] Load SPIR-V shaders from embedded resources

### Phase 2: Compute Pipeline (2 weeks)
- [ ] Create descriptor set layouts
- [ ] Create compute pipelines for L2/Cosine
- [ ] Implement buffer creation (staging + device)
- [ ] Implement memory transfer
- [ ] Create command buffer recording

### Phase 3: Operations (2 weeks)
- [ ] Implement computeDistances()
- [ ] Implement batchKnnSearch()
- [ ] Add top-k selection shader
- [ ] Performance optimization
- [ ] Multi-queue support

## Architecture

```
┌──────────────────────────────────────┐
│   VulkanVectorBackend (C++)          │
├──────────────────────────────────────┤
│  • VkInstance                        │
│  • VkPhysicalDevice (GPU selection)  │
│  • VkDevice (logical device)         │
│  • VkQueue (compute queue)           │
│  • VkCommandPool                     │
│  • VkPipeline (L2/Cosine shaders)    │
├──────────────────────────────────────┤
│  Buffer Management:                  │
│  • Staging buffers (CPU-visible)     │
│  • Device buffers (GPU-only)         │
│  • Memory transfer                   │
└──────────────────────────────────────┘
        ↓
┌──────────────────────────────────────┐
│   SPIR-V Compute Shaders             │
│  • l2_distance.spv                   │
│  • cosine_distance.spv               │
│  • topk_selection.spv (planned)      │
└──────────────────────────────────────┘
```

## Expected Performance

**Estimated** (based on Vulkan compute benchmarks):

| Hardware | Throughput | vs CUDA | vs CPU |
|----------|------------|---------|--------|
| NVIDIA RTX 4090 | ~30,000 q/s | 85% | 17x |
| AMD RX 7900 XTX | ~28,000 q/s | 80% | 16x |
| Intel Arc A770 | ~18,000 q/s | 51% | 10x |

**Advantages over CUDA:**
- ✅ Cross-platform (Windows/Linux/Android)
- ✅ Multi-vendor GPU support (NVIDIA/AMD/Intel)
- ✅ Native on Linux
- ✅ Lower driver overhead on AMD

**Disadvantages:**
- ⚠️ Slightly lower performance than CUDA on NVIDIA
- ⚠️ More complex API
- ⚠️ Less mature ecosystem

## Usage Example (Future)

```cpp
auto& registry = BackendRegistry::instance();
registry.loadPlugin("./plugins/themis_accel_vulkan.so");

auto* backend = registry.getBackend(BackendType::VULKAN);
if (backend && backend->initialize()) {
    auto caps = backend->getCapabilities();
    std::cout << "Vulkan Device: " << caps.deviceName << std::endl;
    std::cout << "VRAM: " << caps.maxMemoryBytes / (1024*1024*1024) << " GB" << std::endl;
    
    // Use for vector operations
    auto results = backend->batchKnnSearch(...);
}
```

## Shader Development

**Workgroup Size:**
- Current: 16x16 (256 threads)
- Optimal for most GPUs
- Can be tuned per-device

**Memory Access Pattern:**
- Coalesced: ✅ Yes (linear access per workgroup)
- Shared Memory: Future optimization
- Push Constants: For dimension/count parameters

**Testing Shaders:**
```bash
# Validate shader
glslangValidator -V shader.comp

# Disassemble SPIR-V
spirv-dis shader.spv

# Optimize
spirv-opt shader.spv -O -o shader_opt.spv
```

## Debugging

**Vulkan Validation Layers:**
```cpp
const char* validationLayers[] = {
    "VK_LAYER_KHRONOS_validation"
};

VkInstanceCreateInfo createInfo = {};
createInfo.enabledLayerCount = 1;
createInfo.ppEnabledLayerNames = validationLayers;
```

**RenderDoc Integration:**
- Capture compute dispatches
- Inspect buffer contents
- Profile shader execution

## Security

Same security model as CUDA:
- Plugin signature verification
- SHA-256 hash checking
- Trusted issuer validation

**Additional Vulkan Security:**
- Validation layers in debug builds
- Memory bounds checking
- Descriptor validation

---

**Next Steps:**
1. Implement Vulkan loader and device selection
2. Add compute pipeline creation
3. Integrate with backend registry
4. Performance benchmarking vs CUDA

**Last Updated:** April 2026  
**Version:** 0.5 (Shaders Only)  
**Target:** Q1 2026 (Full Implementation)
