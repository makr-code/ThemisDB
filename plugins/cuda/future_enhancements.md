# CUDA Acceleration Plugin – Future Enhancements

> **Note:** Items here are *not* on the active roadmap. They are ideas and backlog entries for later consideration. See [`roadmap.md`](roadmap.md) for committed near-term work.

---

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

## Research / References

- [ ] TODO: Add reference – *Efficient GPU Similarity Search* (DOI / arXiv placeholder)
- [ ] TODO: Add reference – *FAISS: A Library for Efficient Similarity Search* – arXiv:1702.08734
- [ ] TODO: Add reference – *cuVS: GPU-Accelerated Vector Search* (NVIDIA, URL placeholder)
- [ ] TODO: Add reference – *Tensor Core GEMM Optimisation* (DOI / arXiv placeholder)
