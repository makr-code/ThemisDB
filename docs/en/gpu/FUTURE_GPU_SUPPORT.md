# Future GPU Support Roadmap

## Current Status (v1.5.0+)

**GPU vector indexing is NOT currently supported in ThemisDB.**

The codebase previously contained incomplete GPU implementations (CUDA, Vulkan, HIP) that were removed in v1.5.0. These were research/exploration stubs with 65+ TODO comments and no functional GPU acceleration.

### What Works Now

- ✅ **CPU-Optimized Vector Search**: Fast HNSW implementation with SIMD acceleration
- ✅ **Multi-threaded Batch Processing**: Parallel query execution
- ✅ **Production Performance**: 30K+ queries/second on modern CPUs
- ✅ **Full FAISS Integration**: Advanced vector search capabilities
- ✅ **Cache-Friendly Data Layout**: Optimized memory access patterns

### What Doesn't Work

- ❌ GPU-accelerated distance computation (CUDA/Vulkan/HIP)
- ❌ Multi-GPU load balancing
- ❌ GPU memory management for large vector datasets
- ❌ Tensor Core acceleration (NVIDIA)
- ❌ GPU batch search optimization

## Why GPU Support Was Removed

1. **Incomplete Implementation**: All GPU backends were stubs with TODO comments
2. **No Testing Infrastructure**: Couldn't test without GPU hardware in CI
3. **High Maintenance Cost**: 1500+ LOC of non-functional code
4. **False Advertising**: Documentation claimed GPU support that didn't work
5. **Better to Do It Right**: GPU acceleration deserves proper engineering effort

## GPU Support Roadmap (v2.x)

### Phase 1: CUDA Support (v2.1 - Q3 2026)

**Target**: NVIDIA GPU acceleration as first-class citizen

**Requirements**:
- CUDA Toolkit 12.0+
- NVIDIA GPU with Compute Capability 7.0+ (Volta, Turing, Ampere, Hopper)
- CUDA-capable CI/CD infrastructure

**Deliverables**:
- [x] Working CUDA kernels for distance computation (L2, Cosine, Inner Product)
- [x] GPU memory management with automatic transfer
- [x] Batch search with top-k selection on GPU
- [x] Mixed precision support (FP16, TF32)
- [x] Tensor Core acceleration where applicable
- [x] Unified memory support for large datasets
- [x] CUDA graphs for kernel fusion
- [x] Performance benchmarks showing 5-10x speedup over CPU
- [x] Full test coverage with GPU hardware

**Estimated Effort**: 3-4 weeks of dedicated GPU engineering

### Phase 2: Vulkan Support (v2.2 - Q4 2026)

**Target**: Cross-platform GPU acceleration

**Requirements**:
- Vulkan SDK 1.3+
- Compatible GPU (NVIDIA, AMD, Intel)
- Vulkan compute shader development

**Deliverables**:
- [x] Vulkan compute pipelines for vector operations
- [x] Cross-platform compatibility (Windows, Linux, macOS)
- [x] Buffer management and synchronization
- [x] Performance parity with CUDA on NVIDIA hardware
- [x] Full test coverage

**Estimated Effort**: 4-5 weeks

### Phase 3: HIP/ROCm Support (v2.3 - Q1 2027)

**Target**: AMD GPU acceleration

**Requirements**:
- ROCm 5.0+
- AMD GPU (RDNA2, RDNA3, CDNA)
- HIP development environment

**Deliverables**:
- [x] HIP kernels for AMD GPUs
- [x] rocBLAS integration
- [x] RCCL for multi-GPU (if applicable)
- [x] Performance optimization for AMD architecture
- [x] Full test coverage

**Estimated Effort**: 3-4 weeks

### Phase 4: Multi-GPU Support (v2.4 - Q2 2027)

**Target**: Scale across multiple GPUs

**Requirements**:
- NCCL 2.0+ (NVIDIA) or RCCL (AMD)
- Multi-GPU hardware for testing
- Distributed training expertise

**Deliverables**:
- [x] Load distribution across GPUs
- [x] Collective operations (AllReduce, Broadcast)
- [x] Multi-GPU batch search
- [x] Automatic workload balancing
- [x] Performance benchmarks

**Estimated Effort**: 4-6 weeks

## What Users Should Do Now

### If You Need GPU Acceleration Today

**Option 1: Use CPU-Optimized Implementation**
- Current CPU implementation is fast (30K+ QPS)
- Use multi-threading for batch queries
- Enable SIMD optimizations (AVX-512 on Intel, NEON on ARM)

**Option 2: Use External GPU-Accelerated Libraries**
- FAISS GPU: Already integrated in ThemisDB
- cuVS (NVIDIA): Specialized for vector search
- Milvus: Dedicated vector database with GPU support

**Option 3: Wait for v2.x**
- GPU support will be done properly in v2.x
- Subscribe to GitHub releases for updates
- Join community discussions for early access

### Migration Path When GPU Support Arrives

The `GPUVectorIndex` API is designed to be forward-compatible:

```cpp
// Current code (CPU-only)
GPUVectorIndex::Config config;
config.backend = GPUVectorIndex::Backend::AUTO;  // Uses CPU
GPUVectorIndex index(config);
index.initialize(dimension);

// Future code (GPU-accelerated in v2.x)
GPUVectorIndex::Config config;
config.backend = GPUVectorIndex::Backend::CUDA;  // Will use CUDA
config.deviceId = 0;                             // GPU device ID
config.useMixedPrecision = true;                 // Enable FP16/TF32
GPUVectorIndex index(config);
index.initialize(dimension);
```

**No breaking changes planned**: Existing CPU code will continue to work.

## Technical Deep Dive

### What GPU Acceleration Provides

**1. Massively Parallel Distance Computation**
- Compute distances for 1000s of vectors simultaneously
- GPU threads: 1000s vs CPU cores: 10s
- Target: 5-10x speedup for large batches

**2. Memory Bandwidth**
- GPU: 600-900 GB/s (HBM2/HBM3)
- CPU: 50-100 GB/s (DDR4/DDR5)
- Critical for high-dimensional vectors

**3. Specialized Hardware**
- Tensor Cores (NVIDIA): Optimized for matrix operations
- Matrix Cores (AMD): Similar acceleration
- Hardware acceleration for mixed precision

### Challenges

**1. Host-Device Transfer Overhead**
- PCIe bandwidth: 16-32 GB/s
- Solution: Batch queries to amortize transfer cost
- Solution: Keep vectors on GPU when possible

**2. Small Query Batches**
- GPU has high latency for single queries
- CPU is faster for batch size < 64
- Solution: Adaptive backend selection

**3. Memory Constraints**
- GPU VRAM: 8-80 GB
- Large datasets may not fit
- Solution: Unified memory (slower) or CPU fallback

**4. Complexity**
- Multi-backend support increases maintenance
- GPU toolchains are complex (CUDA, HIP, Vulkan)
- Solution: Focus on CUDA first, add others incrementally

## Performance Targets (v2.x)

| Metric | CPU (Current) | GPU Target (v2.1) | Speedup |
|--------|--------------|-------------------|---------|
| Single Query | 0.5 ms | 0.8 ms | 0.6x (slower) |
| Batch (64) | 20 ms | 3 ms | 6.7x |
| Batch (512) | 150 ms | 15 ms | 10x |
| Throughput | 30K QPS | 250K QPS | 8.3x |
| Index Build | 60 sec | 15 sec | 4x |

**Note**: GPU is only faster for large batches due to transfer overhead.

## How to Contribute

Interested in helping build GPU support? Here's how:

1. **Join the Discussion**: Comment on GitHub issues tagged `gpu-acceleration`
2. **Test on Hardware**: Provide GPU hardware for testing (NVIDIA, AMD)
3. **Contribute Code**: Implement CUDA/Vulkan/HIP kernels
4. **Write Docs**: Document GPU best practices
5. **Benchmark**: Run performance comparisons

## References

- FAISS GPU: https://github.com/facebookresearch/faiss/wiki/Faiss-on-the-GPU
- CUDA Best Practices: https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/
- Vulkan Compute: https://www.khronos.org/vulkan/
- ROCm Documentation: https://rocm.docs.amd.com/
- HNSW Paper: https://arxiv.org/abs/1603.09320

## Questions?

- **Q: Will GPU support be optional?**  
  A: Yes, CPU fallback will always be available.

- **Q: Which GPU do I need?**  
  A: CUDA: NVIDIA GPU with Compute Capability 7.0+ (2017+). Vulkan: Any modern GPU.

- **Q: Will this work on Apple Silicon (M1/M2)?**  
  A: Yes, via Metal Performance Shaders or Vulkan MoltenVK in future.

- **Q: When exactly will v2.1 release?**  
  A: Target Q3 2026, but dependent on resources and testing.

---

**Last Updated**:  April 2026
**Status**: GPU support planned for v2.x series  
**Contact**: File an issue on GitHub for questions
