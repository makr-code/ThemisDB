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

- J. Johnson, M. Douze, and H. Jégou, "Billion-scale similarity search with GPUs," *IEEE Trans. Big Data*, vol. 7, no. 3, pp. 535–547, Jul. 2021. DOI: [10.1109/TBDATA.2019.2921572](https://doi.org/10.1109/TBDATA.2019.2921572)
- Y. A. Malkov and D. A. Yashunin, "Efficient and robust approximate nearest neighbor search using hierarchical navigable small world graphs," *IEEE Trans. Pattern Anal. Mach. Intell.*, vol. 42, no. 4, pp. 824–836, Apr. 2020. DOI: [10.1109/TPAMI.2018.2889473](https://doi.org/10.1109/TPAMI.2018.2889473)
- S. Chetlur et al., "cuDNN: Efficient primitives for deep learning," arXiv:1410.0759, 2014. [https://arxiv.org/abs/1410.0759](https://arxiv.org/abs/1410.0759)
- V. Volkov and J. W. Demmel, "Benchmarking GPUs to tune dense linear algebra," in *Proc. SC '08: ACM/IEEE Conf. Supercomputing*, 2008, pp. 1–11. DOI: [10.1109/SC.2008.5213359](https://doi.org/10.1109/SC.2008.5213359)
- N. Satish, M. Harris, and M. Garland, "Designing efficient sorting algorithms for manycore GPUs," in *Proc. IEEE International Symp. Parallel & Distributed Processing (IPDPS)*, 2009, pp. 1–10. DOI: [10.1109/IPDPS.2009.5161005](https://doi.org/10.1109/IPDPS.2009.5161005)
