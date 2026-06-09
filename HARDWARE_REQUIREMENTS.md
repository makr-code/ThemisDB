# HARDWARE_REQUIREMENTS.md

# ThemisDB Hardware Requirements
## For Hybrid Knowledge Retrieval Architecture

**Status:** Draft  
**Date:** 2026-06-09

---

## 1. Purpose

This document defines hardware-oriented architectural considerations for ThemisDB's target layered retrieval architecture:

- ANN Frontdoor (HNSW / DiskANN)
- Tensor Mid-Layer
- Graph Truth Layer
- LLM / LoRA Final Layer

The goal is to ensure that architectural decisions remain grounded in realistic deployment, scaling, and performance constraints.

---

## 2. Why Hardware Matters

The target architecture is hardware-sensitive by design.

Different layers stress different resources:

- **HNSW** is RAM- and CPU-locality-sensitive
- **DiskANN** is NVMe / SSD and I/O-sensitive
- **Tensor Mid-Layer** depends on RAM, SIMD, mmap, storage locality, and in some cases GPU support
- **Graph Truth Layer** depends on CPU efficiency, metadata locality, and network efficiency in distributed deployments
- **LLM / LoRA Final Layer** is primarily GPU / VRAM sensitive, but also depends on adapter load and context management

A core planning principle for ThemisDB is therefore **selective acceleration rather than blanket acceleration**.
Not every layer benefits equally from GPU execution, and some exactness- or governance-heavy flows remain better aligned with CPU-first execution.

---

## 3. Hardware Profiles

## 3.1 Development Profile

### Purpose
Local development, functional testing, architectural prototyping.

### Baseline
- CPU: 8–16 cores
- RAM: 32–64 GB
- Storage: 1 fast NVMe SSD
- GPU: optional mid-range GPU
- Network: standard developer workstation network

### Typical Use
- limited HNSW
- reduced datasets
- small tensor summaries
- CPU-first graph validation
- local LLM experimentation

---

## 3.2 Production Profile

### Purpose
Single-instance production deployment with ANN + Tensor + Graph + LLM integration.

### Baseline
- CPU: 16–32+ cores
- RAM: 128 GB+
- Storage: high-performance NVMe
- GPU: at least one inference-capable GPU with sufficient VRAM
- Network: reliable low-latency datacenter networking

### Typical Use
- HNSW for hot data
- DiskANN optional for cold data
- tensor summary storage and loading
- graph evidence validation
- production-grade LLM / LoRA inference

---

## 3.3 High-Performance / Federated Profile

### Purpose
Distributed ThemisDB, cross-shard tensor summaries, heavy RAG, larger models, advanced adaptation.

### Baseline
- CPU: 32+ cores
- RAM: 256 GB+
- Storage: multiple NVMe devices
- GPU: multiple GPUs or dedicated inference/training nodes
- Network: low-latency, high-bandwidth interconnect

### Typical Use
- shard summary exchange
- ANN at larger scale
- distributed tensor summary workflows
- federated retrieval planning
- larger LLM workloads and adapter fleets

---

## 4. Layer-Specific Hardware Considerations

## 4.1 ANN Frontdoor

### HNSW
Best suited for:
- RAM-heavy hot retrieval
- low latency
- interactive workloads

### DiskANN
Best suited for:
- large corpora
- SSD-backed ANN
- lower RAM pressure
- persistent large-scale candidate retrieval

### Hardware Factors
- RAM capacity
- CPU cache locality
- SSD read performance
- memory bandwidth

### Execution Model Guidance
The ANN Frontdoor is the most natural place for GPU assistance when workloads are sufficiently batched and vector-heavy.
Typical GPU-suitable work includes:
- dense vector similarity
- batched top-k candidate generation
- candidate reranking on fixed-size vector sets

CPU-first execution remains appropriate when:
- datasets are too small to amortize transfer cost
- candidate sets are already highly filtered
- latency targets are dominated by orchestration rather than math throughput

---

## 4.2 Tensor Mid-Layer

### Hardware Factors
- RAM for working summaries and fingerprints
- SIMD/vectorized CPU support
- fast NVMe for mmap and lazy loading
- optional GPU for selected tensor operations
- data locality and memory layout

### Key Requirement
Tensor artifacts should be designed for:
- zero-copy where possible
- mmap-friendly formats
- immutable summary artifacts
- controlled materialization

### Execution Model Guidance
The Tensor Mid-Layer is a realistic target for selective GPU acceleration, but not necessarily for universal GPU-only execution.
GPU-suitable operations typically include:
- tensor similarity and contraction on bounded working sets
- summary generation over large batched candidate sets
- routing signal generation and shard relevance scoring
- batch fingerprint comparison

CPU- or mixed-mode execution remains preferable when:
- tensor artifacts are mmap-backed and cheap to access from host memory
- summaries are small enough that SIMD on CPU already saturates performance needs
- transfer overhead would exceed arithmetic savings
- exact graph/provenance checks must immediately follow tensor refinement

---

## 4.3 Graph Truth Layer

### Hardware Factors
- CPU efficiency
- RAM locality for graph metadata and relation traversal
- storage locality for provenance and evidence data
- network efficiency for distributed graph validation

### Key Requirement
Exact validation should minimize cross-shard and cross-storage penalties.

### Execution Model Guidance
The Graph Truth Layer should be treated as **CPU-first by default**.
This layer is responsible for exactness-bearing operations such as:
- exact relation validation
- ACL / permission enforcement
- provenance and evidence-chain traversal
- policy-aware constraint checks
- exact multi-hop validation

Selective GPU use may still be appropriate for bounded graph kernels such as:
- batched frontier expansion
- BFS-like candidate exploration
- fixed-shape adjacency operations

However, GPU execution should not be treated as the default for the final truth layer, because irregular traversal, pointer-heavy metadata access, and governance checks often favor CPU locality and control flow.

---

## 4.4 LLM / LoRA Final Layer

### Hardware Factors
- GPU compute
- VRAM size
- adapter load overhead
- quantization strategy
- context window cost

### Key Requirement
LLM and adapter infrastructure must align with deployment profiles rather than assuming a single universal hardware target.

### Execution Model Guidance
The final generation layer is usually the clearest GPU-dependent layer, but its efficiency still depends on upstream retrieval quality.
This means GPU sizing here must be planned together with:
- ANN shortlist quality
- tensor compression effectiveness
- graph evidence selectivity
- prompt assembly overhead

Better upstream filtering may reduce required context and improve effective GPU utilization more than raw model scaling alone.

---

## 5. CPU/GPU Boundary Model

ThemisDB should explicitly distinguish between **candidate-generation acceleration** and **exactness-bearing validation**.

### GPU-First or GPU-Friendly Zones
Usually appropriate for acceleration when batch size and data layout justify it:
- ANN candidate search
- dense vector distance computation
- tensor contraction / similarity / routing
- bounded frontier-style graph kernels
- batched LLM / adapter inference

### CPU-First Zones
Usually appropriate where irregular control flow, policy logic, or provenance fidelity dominate:
- exact graph validation
- evidence-chain reconstruction
- ACL / policy checks
- provenance-sensitive joins
- distributed coordination and shard-truth reconciliation

### Hybrid Principle
The recommended execution pattern is:
- **GPU** for shortlist generation, compression, scoring, and bounded batched math
- **CPU** for exact validation, governance, provenance, and final truth-bearing checks

This boundary should be treated as an architectural invariant unless benchmark evidence clearly shows otherwise.

---

## 6. Storage and Tiering

Recommended data placement model:

### Hot Tier
- active ANN structures
- high-value tensor summaries
- current adapters
- frequently queried metadata

### Warm Tier
- mmap-backed summaries
- less active ANN data
- model metadata
- package manifests

### Cold Tier
- large archives
- rebuildable derived artifacts
- infrequently accessed summaries
- object-store / slower storage candidates

---

## 7. Special Technical Concerns

## 7.1 Zero-copy / mmap
The tensor and adapter layers should strongly favor mmap-capable, immutable artifact design where possible.

## 7.2 Quantization
Not only LLM weights, but also tensor summaries and fingerprints may be quantized, provided evaluation confirms acceptable loss behavior.

## 7.3 GPU Offload Boundaries
GPU use should be selective. Operations that cause excessive PCIe round-trips may negate gains.

This is especially relevant when:
- graph traversal alternates rapidly between metadata access and compute
- frontier updates require repeated host↔device synchronization
- candidate sets are too small to amortize transfer costs
- exact validation immediately follows approximate ranking

## 7.4 Break-even Analysis
The architecture should define break-even points for:
- HNSW vs DiskANN
- dense vs factorized tensor representations
- summary-first vs direct graph retrieval
- RAM-resident vs SSD-backed artifact access
- CPU SIMD vs GPU dispatch for tensor refinement
- batched graph kernels vs CPU exact traversal
- CPU cache-local graph traversal vs GPU frontier dispatch
- mmap-backed summary access vs GPU upload and execution
- summary-first routing vs direct exact graph fetch
- local exact validation vs distributed fan-out

## 7.5 Practical Graph Acceleration Limits

ThemisDB should explicitly recognize that not all graph-related work is a good fit for GPU execution.

### GPU-friendly graph-related paths
Potentially acceleration-worthy:
- bounded frontier expansion
- batched neighborhood exploration
- prepared fixed-shape adjacency operations
- dense tensor or sketch-based candidate reduction before exact graph load

### CPU-first graph-related paths
Typically better aligned with CPU execution:
- irregular pointer-heavy graph traversal
- provenance-sensitive evidence-chain reconstruction
- ACL / permission enforcement
- policy-aware graph validation
- exact multi-hop traversal over heterogeneous metadata
- coordination-heavy distributed graph truth reconciliation

### Why this matters
These CPU-first paths often depend on:
- cache locality
- branch-heavy execution
- metadata-sensitive traversal order
- low-latency access to many small structures
- tight integration with correctness and governance checks

In such workloads, host↔device synchronization and PCIe transfer overhead may dominate any theoretical GPU arithmetic advantage.

---

## 8. Documentation and Planning Implications

All planning documents and implementation roadmaps should reflect the following concrete assumptions:

1. ANN Frontdoor is the primary acceleration candidate.
2. Tensor Mid-Layer is selectively GPU-accelerated, not universally GPU-resident.
3. Graph Truth Layer remains CPU-first unless a bounded kernel is explicitly proven beneficial.
4. LLM / LoRA throughput must be evaluated together with upstream retrieval compression.
5. Distributed tensor placement decisions must be validated against both hardware locality and network break-even thresholds.

These assumptions should guide benchmarking, planner design, module decomposition, and issue prioritization.

---

## 9. Recommended Next Steps

1. Define benchmark matrix per hardware profile.
2. Establish ANN/HNSW/DiskANN break-even thresholds.
3. Define zero-copy and mmap artifact policy.
4. Align tensor summary formats with storage tiers.
5. Separate dev, prod, and federated hardware assumptions in planning.
6. Document CPU/GPU offload boundaries in target architecture and roadmap artifacts.
7. Map execution boundaries to concrete modules (`index`, `gpu`, `acceleration`, `graph`, `query`, `sharding`).
8. Define explicit CPU-first rules for irregular graph truth workloads.
9. Benchmark host↔device transfer break-even points for tensor refinement and graph-adjacent kernels.
10. Evaluate whether shard summaries can reduce distributed exact-graph fan-out before broad GPU investment.

---

## 10. Executive Statement

**ThemisDB's target architecture is hardware-aware by necessity. ANN, tensor, graph, and LLM layers each have distinct hardware sensitivities, and the long-term architecture must be designed with explicit CPU/GPU boundaries rather than assuming that every retrieval or reasoning path benefits equally from GPU execution. The most realistic model is a hybrid one: GPU for candidate generation and bounded tensor/math acceleration, CPU for exact graph truth, provenance, governance, and distributed coordination.**
