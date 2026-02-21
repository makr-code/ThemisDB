# Acceleration Module Roadmap

## Current Status
Pre-production — infrastructure scaffolding present; CUDA and Vulkan backends under active development.

## Completed ✅
- [x] Directory structure for CUDA and Vulkan backends
- [x] Vector similarity search acceleration stubs
- [x] Geospatial query acceleration stubs
- [x] Parallel graph algorithm stubs
- [x] Matrix operations for embeddings (scaffolding)
- [x] Documentation cross-references (CUDA_BACKEND.md, VULKAN_BACKEND.md)

## In Progress 🚧
- [ ] CUDA kernel implementations for vector similarity (Target: Q2 2026)
- [ ] Vulkan compute shader pipeline for cross-platform GPU (Target: Q2 2026)
- [ ] Integration with geo module GPU backend (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] CUDA-accelerated ANN (Approximate Nearest Neighbor) search
- [ ] CUDA geospatial distance and containment kernels
- [ ] Vulkan fallback for non-NVIDIA hardware
- [ ] Runtime device detection and capability negotiation
- [ ] Benchmark harness for CUDA vs CPU performance comparison

### Long-term (6-12 months)
- [ ] ROCm/HIP support for AMD GPU acceleration
- [ ] Multi-GPU sharding for large embedding datasets
- [ ] Tensor Core utilization for matrix operations (FP16/BF16)
- [ ] CUDA graph capture for recurring query workloads
- [ ] OpenCL backend for broad hardware compatibility

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [ ] Integration tests
- [ ] Performance benchmarks
- [ ] Security audit
- [ ] Documentation complete
- [ ] API stability guaranteed

## Known Issues & Limitations
- CUDA and Vulkan backends are currently stub/scaffolding implementations
- Actual GPU kernels have not yet been written; all operations fall through to CPU
- No runtime device capability detection yet
- Multi-GPU support not implemented

## Breaking Changes
- GPU kernel APIs are not yet stable; function signatures may change before v1.0
