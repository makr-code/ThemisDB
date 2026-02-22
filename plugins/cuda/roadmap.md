# CUDA Acceleration Plugin – Roadmap

## Current Status

**Status:** 📋 Template / Example – production CUDA backend lives in `src/acceleration/`

| Component | Location | Status |
|-----------|----------|--------|
| CUDA backend (built-in) | `src/acceleration/cuda_backend.cpp` | 🔧 Requires build flag |
| CUDA kernels | `src/acceleration/cuda/vector_kernels.cu` | 🔧 Requires build flag |
| External plugin template | `plugins/cuda/` (this directory) | 📋 Template only |

Build flag: `-DTHEMIS_ENABLE_CUDA=ON`

---

## Short-term Goals (next 1–2 sprints)

- [ ] Validate that `CMakeLists.txt.example` and `cuda_plugin.cpp.example` compile cleanly with CUDA Toolkit 12.x.
- [ ] Document minimum driver / toolkit version requirements in this README.
- [ ] Add health-check / capability-query entry point to the plugin interface.

## Mid-term Goals (1–3 months)

- [ ] Promote the built-in CUDA backend to a proper loadable plugin (`.so`/`.dll`) so it can be shipped independently.
- [ ] Add benchmark suite: throughput (vectors/sec) vs. CPU baseline.
- [ ] Implement mixed-precision (FP16 / BF16) vector operations.
- [ ] Add support for multi-GPU setups (device selection, peer-to-peer transfers).

## Long-term Goals (3–12 months)

- [ ] Unified hardware-acceleration plugin interface shared by CUDA, Vulkan, HIP, Metal, OpenCL backends.
- [ ] Auto-tuning kernel parameters (block size, shared memory) per GPU architecture.
- [ ] Integration with RAPIDS cuVS / FAISS-GPU for approximate nearest-neighbour search.
- [ ] Formal performance targets: ≥ 8× speedup over CPU baseline on RTX-class GPUs.

## Milestones

| Milestone | Target | Status |
|-----------|--------|--------|
| Standalone CUDA plugin DLL | TODO | 🔲 Planned |
| FP16 support | TODO | 🔲 Planned |
| Multi-GPU support | TODO | 🔲 Planned |
| ANN integration (cuVS) | TODO | 🔲 Planned |

## Dependencies

- CUDA Toolkit ≥ 11.8 (runtime + nvcc)
- ThemisDB `IComputeBackend` interface (`include/acceleration/compute_backend.h`)
- `BackendRegistry` (`src/acceleration/backend_registry.cpp`)

## Open Questions

- [ ] Should external CUDA plugin DLLs replace or supplement the built-in backend?
- [ ] Which CUDA architectures (sm\_xx) should be compiled by default?

---

*See also: [`future_enhancements.md`](future_enhancements.md) · [`PLANNED_ACCELERATION_PLUGINS.md`](../PLANNED_ACCELERATION_PLUGINS.md)*
