# GPU-Optimized Vector Indexing for Hybrid Database Retrieval

**Status**: Draft  
**Version**: 0.2 (migrated from docs/research)  
**Last Updated**: 2026-04-19  
**Target Venue**: arXiv (cs.DB / cs.DC)

---

## Abstract

This paper evaluates GPU-native vector indexing strategies for database-integrated retrieval workloads. It focuses on practical trade-offs between brute-force kernels, IVF-style approximate search, PQ compression, and multi-GPU scaling. The contribution is a systems-oriented blueprint grounded in existing ThemisDB acceleration modules and benchmark plans, with explicit boundaries between implemented capabilities and planned high-impact optimizations.

## I. Introduction

Vector retrieval performance becomes a bottleneck when query volume and embedding corpus size increase. CPUs provide stable baseline behavior but often underutilize parallel opportunities in large-batch search.

### Contributions

1. A unified taxonomy of GPU indexing options in ThemisDB context.
2. A repository-grounded baseline for current acceleration support.
3. A staged evaluation protocol for production readiness decisions.

## II. Related Work

- GPU ANN search (IVF, graph-based approaches, brute-force baselines).
- Memory-bandwidth-centric acceleration patterns.
- Multi-GPU partitioning and throughput scaling.

## III. System Model / Architecture

- Query path: AQL/vector retrieval -> acceleration backend -> top-k merge.
- Backends: CUDA kernels + FAISS GPU integration + CPU fallback.
- Deployment model: single-GPU baseline with optional multi-GPU partitioning.

## IV. Method / Design

- Compare exact and approximate regimes under common metric suite.
- Evaluate index build/search separately.
- Quantify quality-speed-memory frontier with operational thresholds.

## V. Implementation Evidence (Repository-Grounded)

| Evidence ID | File | Scope | What It Proves | Status |
|-------------|------|-------|----------------|--------|
| E1 | `src/acceleration/cuda/vector_kernels.cu` | CUDA kernels | GPU distance/top-k primitives exist | Ready |
| E2 | `src/acceleration/faiss_gpu_backend.cpp` | backend integration | FAISS GPU retrieval path exists | Ready |
| E3 | `docs/research/GPU_VECTOR_INDEXING_RESEARCH.md` | research basis | Existing state-of-the-art and planning analysis | Ready |
| E4 | `benchmarks/ann/README.md` | benchmark protocol | ANN benchmark framing and metrics are defined | Ready |
| E5 | `benchmarks/performance_optimizations/ARCHITECTURE.md` | integration constraints | Performance optimization module boundaries documented | Ready |

## VI. Experimental Methodology

### A. Setup
- Fixed vector dimensions (e.g., 384/768/1536)
- Dataset scales: 1M, 10M, 100M vectors
- Single- and multi-GPU profiles where available

### B. Workloads
- W1: exact brute-force batch search
- W2: IVF search with nprobe sweep
- W3: PQ-assisted retrieval with varying compression

### C. Metrics
- Search latency (p50/p95/p99)
- Throughput (queries/s)
- Recall@k
- Build time and memory footprint
- Performance/Watt (optional where hardware telemetry exists)

## VII. Results (Planned Consolidation)

Current repository evidence indicates partial production readiness for CUDA + FAISS GPU paths; full comparative curves across all workload scales remain to be consolidated.

## VIII. Discussion

GPU acceleration is most impactful at medium-to-large batch sizes and higher corpus scales. Operational fallback to CPU is still important for portability and failure resilience.

### Claim Boundaries

**Supported claims:**
- CUDA kernel and FAISS GPU integration paths are present.
- ANN benchmarking methodology is already scaffolded.

**Deferred claims:**
- Definitive multi-GPU linear scaling beyond current validated setups.
- Uniform superiority across all query-size distributions.

## IX. Reproducibility & Artifact

- Code anchors: acceleration and benchmark modules listed above.
- Re-run guidance: ANN benchmark suite + backend toggles.
- Remaining task: publish a consolidated result table by dataset size and index type.

## X. Limitations, Risk, Ethics

- Hardware dependency can bias conclusions.
- Approximate methods require transparent recall-loss reporting.
- Memory constraints and thermal throttling can affect reproducibility.

## XI. Conclusion

ThemisDB has a concrete foundation for GPU-first vector retrieval. The next step is systematic benchmark consolidation and policy-driven index selection for production profiles.

## References

- See migrated source bibliography and notes: `docs/research/GPU_VECTOR_INDEXING_RESEARCH.md`.
