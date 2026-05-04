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

## In Progress

- [~] Validate that `CMakeLists.txt.example` and `cuda_plugin.cpp.example` compile cleanly with CUDA Toolkit 12.x
- [~] Document minimum driver / toolkit version requirements

## Planned Features

- [ ] Standalone loadable CUDA plugin (`.so`/`.dll`) independent of the built-in backend (Target: Q3 2026)
- [ ] FP16 / BF16 mixed-precision vector operations (Target: Q3 2026)
- [ ] Multi-GPU support: device selection, peer-to-peer transfers (Target: Q4 2026)
- [ ] Integration with RAPIDS cuVS / FAISS-GPU for ANN search (Target: Q4 2026)
- [ ] Auto-tuning kernel parameters per GPU architecture (Target: 2027)
- [ ] Unified hardware-acceleration interface (CUDA, Vulkan, HIP, Metal, OpenCL) (Target: 2027)

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

## Implementation Phases

### Phase 1 – Standalone Plugin DLL
- [ ] Extract built-in CUDA backend into a loadable `cuda_plugin.so`/`.dll`
- [ ] Define and implement `IComputeBackend` plugin entry-point (`plugin_create` / `plugin_destroy`)
- [ ] CMake target for standalone plugin with correct RPATH / install rules
- [ ] Smoke test: load plugin via `PluginManager`, run a vector similarity query

### Phase 2 – FP16 / Mixed-Precision Support
- [ ] Implement FP16 and BF16 variants of core vector kernels (`vector_kernels.cu`)
- [ ] Precision selection via plugin configuration (`float`, `fp16`, `bf16`)
- [ ] Benchmark: FP16 throughput vs FP32 baseline on RTX-class GPU

### Phase 3 – Multi-GPU & ANN Integration
- [ ] Device enumeration and selection API (`--cuda-device=<id>`)
- [ ] Peer-to-peer transfers for sharded index across multiple GPUs
- [ ] cuVS / FAISS-GPU ANN integration: IVF-Flat and HNSW index types
- [ ] Multi-GPU CI tests skipped gracefully when only one GPU is present

### Phase 4 – Auto-tuning & Formal Perf Targets
- [ ] Auto-tuning: select optimal block size and shared-memory config per GPU arch
- [ ] Formal performance target: ≥ 8× speedup over CPU baseline on RTX-class GPUs
- [ ] Publish benchmark results in `benchmarks/cuda/`

---

## Dependencies

- CUDA Toolkit ≥ 11.8 (runtime + nvcc)
- ThemisDB `IComputeBackend` interface (`include/acceleration/compute_backend.h`)
- `BackendRegistry` (`src/acceleration/backend_registry.cpp`)

## Open Questions

- [ ] Should external CUDA plugin DLLs replace or supplement the built-in backend?
- [ ] Which CUDA architectures (sm\_xx) should be compiled by default?

---

## Production Readiness Checklist

| Item | Status |
|------|--------|
| `CMakeLists.txt.example` validated with CUDA 12.x | ❌ Pending |
| Standalone loadable plugin DLL | ❌ Not implemented |
| FP16 / BF16 support | ❌ Not implemented |
| Multi-GPU support | ❌ Not implemented |
| ANN (cuVS / FAISS-GPU) integration | ❌ Not implemented |
| Benchmark results published | ❌ Pending |

## Known Issues & Limitations

- Template (`plugins/cuda/`) not yet validated against CUDA Toolkit 12.x
- Multi-GPU paths are tested only on single-GPU CI runners; correctness unverified
- Standalone plugin DLL does not exist; production CUDA lives in the built-in backend only
- No formal performance targets documented yet

---

*See also: [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md) · [`PLANNED_ACCELERATION_PLUGINS.md`](../PLANNED_ACCELERATION_PLUGINS.md)*
