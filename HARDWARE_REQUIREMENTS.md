# HARDWARE_REQUIREMENTS.md

# ThemisDB Hardware Requirements
## For Hybrid Knowledge Retrieval Architecture

**Status:** Active baseline  
**Date:** 2026-07-15

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

---

## 3. Hardware Profiles

The canonical built-in profile IDs used by EPIC 2 evaluation code are:

- `development`
- `production`
- `high_performance_federated`

They are defined in:

- `src/evaluation/include/hardware_profile.h`
- `src/evaluation/src/hardware_profile.cc`

## 3.1 Development Profile

### Purpose
Local development, functional testing, architectural prototyping.

### Baseline
- CPU: 8–16 cores
- RAM: 32–64 GiB
- Storage: 1 × 512 GiB–1 TiB fast NVMe SSD
- GPU: optional 0–12 GiB mid-range GPU
- Network: 1–10 GbE workstation-class network

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
- CPU: 16–32 cores
- RAM: 128–256 GiB
- Storage: 2–4 × high-performance NVMe drives, 2–4 TiB total
- GPU: 1–2 inference-capable GPUs, 24–48 GiB VRAM
- Network: 10–25 GbE low-latency datacenter networking

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
- CPU: 32–64 cores
- RAM: 256–512 GiB
- Storage: 4–8 NVMe devices, 4–16 TiB total
- GPU: 2–4 GPUs, 48–80 GiB VRAM
- Network: 100–200 Gbps low-latency fabric or equivalent interconnect

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

---

## 4.3 Graph Truth Layer

### Hardware Factors
- CPU efficiency
- RAM locality for graph metadata and relation traversal
- storage locality for provenance and evidence data
- network efficiency for distributed graph validation

### Key Requirement
Exact validation should minimize cross-shard and cross-storage penalties.

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

---

## 5. Storage and Tiering

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

### Tiering Rules Used by the Hardware Profile Registry

- **Development:** live hot/warm/cold rebalancing is disabled; cold storage can remain local.
- **Production:** live rebalancing is enabled; cold storage expects remote/object-backed capacity.
- **High-Performance / Federated:** live rebalancing is enabled and cold storage expects remote/object-backed capacity.

---

## 6. Special Technical Concerns

## 6.1 Zero-copy / mmap
The tensor and adapter layers should strongly favor mmap-capable, immutable artifact design where possible.

## 6.2 Quantization
Not only LLM weights, but also tensor summaries and fingerprints may be quantized, provided evaluation confirms acceptable loss behavior.

## 6.3 GPU Offload Boundaries
GPU use should be selective. Operations that cause excessive PCIe round-trips may negate gains.

## 6.4 Break-even Analysis
The architecture should define break-even points for:
- HNSW vs DiskANN
- dense vs factorized tensor representations
- summary-first vs direct graph retrieval
- RAM-resident vs SSD-backed artifact access

---

## 7. Recommended Next Steps

1. Define benchmark matrix per hardware profile.
2. Establish ANN/HNSW/DiskANN break-even thresholds.
3. Define zero-copy and mmap artifact policy.
4. Align tensor summary formats with storage tiers.
5. Separate dev, prod, and federated hardware assumptions in planning.

---

## 8. Executive Statement

**ThemisDB's target architecture is hardware-aware by necessity. ANN, tensor, graph, and LLM layers each have distinct hardware sensitivities, and the long-term architecture must be designed with explicit deployment profiles, tiered storage, and benchmark-driven break-even decisions.**
