---
name: "[LoRA Phase 10.2] DirectX 12 Compute Pipeline Integration"
about: Implement DirectX 12 compute pipeline for GPU-accelerated LoRA training on Windows (Priority #2)
title: "[LoRA Phase 10.2] Implement DirectX 12 Compute Pipeline Integration"
labels: ['llm', 'lora', 'gpu-acceleration', 'directx', 'windows', 'performance', 'enhancement', 'phase-10']
assignees: ''
---

## 📋 Description

**Priority**: Phase 10.2 - Priority #2 (Windows Optimization)  
**Prerequisites**: Phase 1-9 complete (DirectX shaders ready, interfaces complete)  
**Estimated Effort**: 3 weeks  
**Status Document**: `LORA_GPU_PHASE10_PLAN.md`

Implement DirectX 12 compute pipeline integration for native Windows GPU acceleration. DirectX shaders (HLSL) are already complete; this phase focuses on D3D12 device setup, compute pipeline state objects, and shader dispatch integration.

## 🎯 Goals

- [ ] Complete DirectX 12 compute pipeline implementation
- [ ] Native Windows GPU acceleration (all vendors: NVIDIA, AMD, Intel)
- [ ] Achieve ~45x speedup over CPU baseline
- [ ] Integration with DirectML for future ML operations
- [ ] Lower driver overhead compared to Vulkan on Windows

## 📝 Tasks

### 1. DirectX 12 Device Setup
- [ ] Implement `DirectXContext` class for device management
- [ ] Initialize D3D12 device and command queue
- [ ] Setup DirectX Agility SDK integration
- [ ] Create command allocator and command list
- [ ] Implement GPU fence synchronization

**Files**:
- `include/llm/lora_framework/directx_context.h`
- `src/llm/lora_framework/directx_context.cpp`

### 2. DirectX Resource Management
- [ ] Implement `DirectXBuffer` class wrapping ID3D12Resource
- [ ] Create upload heaps for CPU→GPU data transfer
- [ ] Create default heaps for GPU-only buffers
- [ ] Implement resource state transitions and barriers
- [ ] Support GPU memory pooling and reuse

**Files**:
- `include/llm/lora_framework/directx_buffer.h`
- `src/llm/lora_framework/directx_buffer.cpp`

### 3. Compute Shader Compilation and Loading
- [ ] Integrate DXC (DirectX Shader Compiler) for HLSL→DXIL
- [ ] Load compiled shader bytecode (CSO files)
- [ ] Create root signature from shader reflection
- [ ] Support runtime shader compilation (optional)
- [ ] Implement shader caching for faster startup

**Files**:
- `include/llm/lora_framework/directx_shader.h`
- `src/llm/lora_framework/directx_shader.cpp`
- `CMakeLists.txt` (add DXC shader compilation)

### 4. Compute Pipeline State Objects (PSO)
- [ ] Implement `DirectXComputePipeline` class
- [ ] Create compute PSO with shader and root signature
- [ ] Setup descriptor heaps (CBV/SRV/UAV)
- [ ] Implement root parameter binding
- [ ] Support pipeline state caching

**Files**:
- `include/llm/lora_framework/directx_pipeline.h`
- `src/llm/lora_framework/directx_pipeline.cpp`

### 5. Command List Recording and Execution
- [ ] Record compute shader dispatch commands
- [ ] Set pipeline state and root signature
- [ ] Bind descriptor tables and root constants
- [ ] Dispatch compute shader with thread groups
- [ ] Execute command list with GPU synchronization

**Files**:
- Update `src/llm/lora_framework/kernels/directx_kernels.cpp`

### 6. Descriptor Management
- [ ] Create descriptor heaps for compute shaders
- [ ] Implement descriptor handle allocation
- [ ] Create UAV/SRV descriptors for buffers
- [ ] Bind descriptors to root signature
- [ ] Support descriptor table ranges

**Files**:
- `include/llm/lora_framework/directx_descriptors.h`
- `src/llm/lora_framework/directx_descriptors.cpp`

### 7. Integration with GPUTensor
- [ ] Replace stub implementations in `directx_kernels.cpp`
- [ ] Implement `dispatch_matmul_directx()` with PSO
- [ ] Implement element-wise operations dispatch
- [ ] Implement transpose and gradient kernels dispatch
- [ ] Update `gpu_tensor.cpp` to use DirectX backend

**Files**:
- `src/llm/lora_framework/kernels/directx_kernels.cpp` (update)
- `src/llm/lora_framework/gpu_tensor.cpp` (update)

### 8. DirectML Integration (Future)
- [ ] Setup DirectML device and command recorder
- [ ] Implement DirectML operator support (optional)
- [ ] Integration with Windows ML acceleration
- [ ] Benchmark DirectML vs custom compute shaders

**Files**:
- `include/llm/lora_framework/directml_ops.h` (new, optional)

### 9. Performance Optimization
- [ ] Optimize thread group sizes for operations
- [ ] Implement command list reuse and batching
- [ ] Add PSO caching for faster startup
- [ ] Optimize resource barriers and transitions
- [ ] Profile with PIX for Windows

### 10. Testing and Validation
- [ ] Unit tests for DirectX device and resource management
- [ ] Tests for compute PSO creation and execution
- [ ] Numerical accuracy tests (DirectX vs CPU)
- [ ] Performance benchmarks (compare to CUDA/Vulkan)
- [ ] Test on different GPU vendors (NVIDIA, AMD, Intel)
- [ ] Memory leak detection with DirectX debug layer

**Files**:
- `tests/test_directx_backend.cpp` (new)
- Update `tests/test_gpu_tensor.cpp` with DirectX tests

### 11. Documentation
- [ ] API documentation for DirectX backend
- [ ] Windows-specific setup and installation guide
- [ ] Performance tuning guide for DirectX
- [ ] Troubleshooting DirectX issues
- [ ] Update Phase 10 plan with completion status

## ✅ Acceptance Criteria

- [ ] DirectX 12 compute pipeline fully functional on Windows
- [ ] All tensor operations work with DirectX backend
- [ ] **Performance**: Training step ~3.5ms (matching CUDA/HIP/Vulkan)
- [ ] **Accuracy**: Numerical error < 1e-5 vs CPU reference
- [ ] **Memory**: VRAM usage similar to other backends
- [ ] Works on all GPU vendors (NVIDIA, AMD, Intel)
- [ ] Zero CPU round-trips during training
- [ ] No memory leaks detected by DirectX debug layer
- [ ] Integration with GPUTensor API seamless

## 🔗 Dependencies

- DirectX 12 with Shader Model 6.0+
- Windows 10 version 1809+ or Windows 11
- DirectX Agility SDK (for latest features)
- DXC (DirectX Shader Compiler)
- Existing DirectX shaders (complete): `matmul.hlsl`, `elementwise.hlsl`, `gradient.hlsl`
- VRAMAllocator and GPUMemoryManager (complete)
- GPUTensor class (complete)

## 📊 Performance Targets

| Operation | Target Time | Current (CPU) | Speedup |
|-----------|-------------|---------------|---------|
| MatMul (768×768) | 0.1ms | 10ms | 100x |
| Element-wise | 0.02ms | 2ms | 100x |
| Forward pass | 1ms | 50ms | 50x |
| Backward pass | 2ms | 100ms | 50x |
| **Full training step** | **3.5ms** | **160ms** | **~45x** |

## 📚 References

- DirectX 12 Programming Guide: https://docs.microsoft.com/en-us/windows/win32/direct3d12/
- DirectX Agility SDK: https://devblogs.microsoft.com/directx/directx12agility/
- HLSL Documentation: https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/
- Phase 10 Plan: `LORA_GPU_PHASE10_PLAN.md`
- Existing Shaders: `src/acceleration/directx/shaders/lora/*.hlsl`

## 💡 Implementation Notes

### DirectX Pipeline Architecture
```
DirectXContext → DirectXComputePipeline → DirectXBuffer
     ↓                    ↓                    ↓
ID3D12Device      PipelineState         ID3D12Resource
     ↓                    ↓                    ↓
CommandQueue       Dispatch              GPU Memory
```

### Root Signature Design
```hlsl
// Root constants for dimensions
RootConstants(num32BitConstants=4, b0)
// UAV for output buffer
DescriptorTable(UAV(u0, numDescriptors=1))
// SRV for input buffers
DescriptorTable(SRV(t0, numDescriptors=2))
```

### Thread Group Sizes
- Matrix multiplication: [16, 16, 1] (matching shader)
- Element-wise: [256, 1, 1]
- Transpose: [16, 16, 1] with group shared memory

### Windows-Specific Benefits
- Lower driver overhead compared to Vulkan
- Better integration with Windows ecosystem
- Support for all GPU vendors with single API
- DirectML integration for future optimizations

---

**Status**: Not Started  
**Estimated Completion**: Week 6 of Phase 10  
**Depends On**: Vulkan pipeline (Phase 10.1) for cross-validation
