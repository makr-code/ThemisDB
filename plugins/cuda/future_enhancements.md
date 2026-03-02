# CUDA Acceleration Plugin – Future Enhancements

> **Note:** Items here are *not* on the active roadmap. They are ideas and backlog entries for later consideration. See [`roadmap.md`](roadmap.md) for committed near-term work.

---

## Scope

- This plugin directory is an extension template for external CUDA kernels; the production CUDA backend lives in `src/acceleration/`.
- Enhancements cover integration patterns (kernel registration API), kernel packaging (shared library manifest), and GPU capability negotiation at runtime.
- Out of scope: changes to the in-tree CUDA backend in `src/acceleration/`; this plugin only defines the external extension contract.
- Targets NVIDIA Ampere (sm_80) and Hopper (sm_90) architectures; older architectures (< sm_70) are best-effort.

## Design Constraints

- [ ] Kernel registration MUST complete within 10 ms at process startup, including CUDA context initialisation.
- [ ] Each external kernel MUST be accompanied by a SHA-256 manifest entry; loading fails if checksum does not match.
- [ ] Kernels MUST declare their minimum compute capability; the loader MUST reject kernels on incompatible devices.
- [ ] No external kernel may allocate persistent device memory beyond its declared `max_device_bytes` manifest field.
- [ ] All kernel dispatch calls MUST return within 0.5 ms of CPU-side invocation (excluding actual GPU execution time).
- [ ] Template plugin MUST compile with `-Wall -Wextra -Werror` and zero warnings on GCC 13 and Clang 17.

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `IExternalKernelPlugin` | `CudaAccelerationManager` | `register()`, `dispatch()`, `unload()` |
| `KernelManifest` (JSON schema) | Plugin loader | Name, compute capability, SHA-256, `max_device_bytes` |
| `CudaDeviceCapabilityQuery` | `IExternalKernelPlugin` impl | Returns `sm_major`, `sm_minor`, free VRAM |
| `IKernelResultBuffer` | `CudaAccelerationManager` | Zero-copy device→host transfer handle |
| OpenTelemetry `Tracer` | Dispatch path | Span per kernel invocation with GPU time attribute |

## Idea Backlog

### Performance

- [ ] **Tensor Core utilisation** – leverage Tensor Cores for GEMM-heavy workloads on Ampere/Hopper GPUs.
- [ ] **Persistent kernels** – keep kernels resident to reduce launch overhead for streaming workloads.
- [ ] **CUDA Graphs** – record and replay kernel sequences for minimal CPU overhead.
- [ ] **Stream-based pipeline** – overlap data transfer (H→D) and compute.

### Algorithmic Extensions

- [ ] **IVF-Flat / HNSW on GPU** – GPU-native approximate nearest-neighbour indices.
- [ ] **Geospatial kernels** – WGS84 distance and containment checks on GPU.
- [ ] **Graph traversal kernels** – BFS/DFS on ThemisDB graph collections.

### Deployment

- [ ] **NVIDIA Triton Inference Server integration** – serve embedding models via Triton.
- [ ] **Container image with pre-built CUDA plugin** – Docker layer for easy deployment.
- [ ] **MIG (Multi-Instance GPU) support** – run on partitioned GPU slices.

### Testing / Validation

- [ ] **GPU/CPU parity tests** – deterministic FP tolerance ≤ 1 × 10⁻⁶ for all kernels.
- [ ] **Property-based tests** – random input generation for kernel correctness.

---

## Test Strategy

- Unit tests for `IExternalKernelPlugin` registration and dispatch using a stub CUDA runtime; no GPU hardware required.
- GPU/CPU parity tests: every kernel result must match CPU reference implementation within FP tolerance ≤ 1 × 10⁻⁶.
- Manifest validation tests: assert that loading a kernel with a tampered SHA-256 fails with `KERNEL_CHECKSUM_MISMATCH`.
- Capability rejection tests: assert that a sm_80 kernel is refused on a sm_70 device.
- Dispatch overhead benchmark: 1,000 sequential dispatches; mean overhead ≤ 0.5 ms each, measured with `std::chrono::high_resolution_clock`.

## Performance Targets

- Kernel registration (including CUDA context init) ≤ 10 ms at process startup on any CUDA-capable device.
- CUDA kernel dispatch CPU-side overhead ≤ 0.5 ms per invocation (time from `dispatch()` call to kernel launch on stream).
- Device→host result transfer ≤ 1 ms for result buffers ≤ 4 MB via `IKernelResultBuffer`.
- Template plugin build time ≤ 30 s (full rebuild, single core) to keep iteration fast for extension authors.

## Security / Reliability

- Kernel shared libraries MUST be authenticated via SHA-256 manifest before `dlopen`; loading aborts on mismatch.
- No untrusted kernel code may be executed; the manifest is signed by the ThemisDB plugin signing key.
- External kernels MUST NOT exceed their declared `max_device_bytes`; the loader enforces this at registration time.
- CUDA runtime errors MUST be caught and converted to typed `CudaPluginError` exceptions; no raw `cudaError_t` surfaces to callers.
- Plugin unload (`IExternalKernelPlugin::unload()`) MUST synchronise all CUDA streams and free all device allocations before returning.

## Research / References

- J. Johnson, M. Douze, and H. Jégou, "Billion-scale similarity search with GPUs," *IEEE Trans. Big Data*, vol. 7, no. 3, pp. 535–547, Jul. 2021. DOI: [10.1109/TBDATA.2019.2921572](https://doi.org/10.1109/TBDATA.2019.2921572)
- Y. A. Malkov and D. A. Yashunin, "Efficient and robust approximate nearest neighbor search using hierarchical navigable small world graphs," *IEEE Trans. Pattern Anal. Mach. Intell.*, vol. 42, no. 4, pp. 824–836, Apr. 2020. DOI: [10.1109/TPAMI.2018.2889473](https://doi.org/10.1109/TPAMI.2018.2889473)
- S. Chetlur et al., "cuDNN: Efficient primitives for deep learning," arXiv:1410.0759, 2014. [https://arxiv.org/abs/1410.0759](https://arxiv.org/abs/1410.0759)
- V. Volkov and J. W. Demmel, "Benchmarking GPUs to tune dense linear algebra," in *Proc. SC '08: ACM/IEEE Conf. Supercomputing*, 2008, pp. 1–11. DOI: [10.1109/SC.2008.5213359](https://doi.org/10.1109/SC.2008.5213359)
- N. Satish, M. Harris, and M. Garland, "Designing efficient sorting algorithms for manycore GPUs," in *Proc. IEEE International Symp. Parallel & Distributed Processing (IPDPS)*, 2009, pp. 1–10. DOI: [10.1109/IPDPS.2009.5161005](https://doi.org/10.1109/IPDPS.2009.5161005)
