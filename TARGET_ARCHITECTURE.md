# TARGET_ARCHITECTURE.md

# ThemisDB Target Architecture
## Hybrid Knowledge Retrieval Architecture

**Status:** Draft  
**Date:** 2026-06-08

---

## 1. Overview

The target architecture of ThemisDB is a layered hybrid retrieval and reasoning system composed of four primary layers:

1. ANN Frontdoor
2. Tensor Mid-Layer
3. Graph Truth Layer
4. LLM / LoRA Final Layer

This architecture is intended to support scalable, explainable, distributed, and adaptive RAG.

A core design constraint is that the architecture is **hybrid not only logically, but also operationally**.
Different layers have different execution characteristics, and the system should not assume that all retrieval or reasoning paths become better merely by moving them to GPU execution.

---

## 2. Layer Model

## 2.1 ANN Frontdoor

### Technologies
- HNSW
- DiskANN

### Responsibilities
- fast semantic candidate retrieval
- top-k shortlist generation
- retrieval over documents, chunks, entities, adapters, packages, and shard summaries
- hot/cold separation

### Output
- semantic candidate set for tensor refinement

### Execution Model
The ANN Frontdoor is the most natural acceleration boundary in the layered architecture.
It is the preferred zone for:
- SIMD-heavy CPU search
- GPU-assisted dense vector search
- batched top-k generation
- candidate shortlist production at large scale

Its role is to accelerate candidate discovery, not to establish semantic truth.

---

## 2.2 Tensor Mid-Layer

### Responsibilities
- candidate compression
- redundancy reduction
- routing
- tensor fingerprints
- structural summaries
- relational approximation
- shard relevance estimation
- adapter/package similarity

### Output
- condensed candidate space
- tensor summaries
- routing/prioritization signals

### Execution Model
The Tensor Mid-Layer is a selective acceleration layer.
It is a realistic target for GPU-assisted execution when operations are:
- batched
- bounded in shape
- numerically dense
- reusable across many candidate items

Typical examples include:
- tensor similarity
- contraction-based routing
- summary generation
- fingerprint comparison
- shard relevance scoring

However, the Tensor Mid-Layer is **not defined as universally GPU-resident**.
It must also support:
- CPU SIMD execution
- mmap-backed summaries
- zero-copy or low-copy host-side access
- controlled materialization before exact validation

Its job is to refine, compress, and prioritize candidate space — not to replace exact graph semantics.

---

## 2.3 Graph Truth Layer

### Responsibilities
- exact relation validation
- provenance
- evidence chains
- ACL / permissions
- policy-aware constraints
- exact multi-hop validation

### Output
- validated evidence set
- provenance-aware context

### Execution Model
The Graph Truth Layer is **CPU-first by architectural intent**.
This layer is where:
- exactness matters
- provenance must remain inspectable
- ACL and governance checks must remain explicit
- policy-aware constraints must not be hidden behind approximate kernels

GPU participation may still exist for bounded or auxiliary graph kernels, such as:
- frontier-style expansion
- batched graph neighborhood evaluation
- fixed-shape graph math over prepared structures

But the final truth-bearing graph layer is not intended to become a generic GPU traversal engine.
Irregular, pointer-heavy, provenance-sensitive, and governance-bearing flows remain more naturally aligned with CPU execution.

---

## 2.4 LLM / LoRA Final Layer

### Responsibilities
- final grounded generation
- domain adaptation via LoRA/AdaLoRA
- structured prompt assembly
- adaptive answer generation

### Input Sources
- vector candidates
- tensor summaries
- graph evidence
- trust/provenance metadata

### Output
- grounded answer
- optionally justification metadata

### Execution Model
The final generation layer is typically the most GPU-dependent layer, but its operational efficiency depends strongly on upstream selectivity.
The better the ANN and Tensor layers reduce irrelevant candidate mass, and the better the Graph Truth Layer constrains evidence, the more efficiently the LLM / LoRA layer can use its context and VRAM budget.

---

## 3. Retrieval Pipeline

### Current Classical Pattern
`query -> embedding -> top-k chunks -> prompt -> answer`

### Target Pattern
`query -> ANN frontdoor -> tensor compression/routing -> graph validation/evidence -> LLM/LoRA generation`

### Operational Interpretation
This target pattern should also be read as an execution-boundary model:
- ANN = candidate generation
- Tensor = candidate compression / routing / approximation
- Graph = exact validation / provenance / governance
- LLM = final grounded synthesis

---

## 4. Long-Term Extensions

## 4.1 Unified Knowledge Tensor Layer
Shared tensor-based representation substrate across:
- graph knowledge
- embeddings
- documents
- process models
- adapter knowledge
- provenance

## 4.2 Tensor-native Graph Reasoning
Approximate reasoning support through:
- factorized relation propagation
- tensor contraction
- approximate relational inference

This extension is intended for approximation, prioritization, and search-space reduction.
It is not a mandate to replace exact graph validation with tensor inference.

## 4.3 Federated / Cross-shard Tensor Summaries
Distributed architecture based on:
- shard-level tensor summaries
- selective exact loading
- reduced fan-out and network transfer

---

## 5. Architectural Principles

1. ANN is not the truth layer.
2. Tensor is not a graph replacement.
3. Graph remains exact and evidence-bearing.
4. LLM is not the retrieval source of truth.
5. Approximation may prioritize, but not replace exact correctness where governance matters.
6. GPU acceleration is selective and layer-sensitive, not universal.
7. Graph truth and governance semantics remain explicit even when earlier layers are accelerated.

---

## 6. Non-Goals

- replacing graph with tensors
- replacing ANN with tensors
- replacing exact policy checks with approximations
- making every subsystem tensor-native prematurely
- assuming that every graph or retrieval path should be GPU-native

---

## 7. Success Criteria

- reduced retrieval latency
- smaller prompt contexts
- better evidence quality
- lower cross-shard traffic
- better adapter and package lifecycle handling
- stronger provenance integration
- explicit and benchmarked CPU/GPU execution boundaries

---

## 8. Recommended Follow-Ups

- gap analysis
- per-layer inventory
- issue-based implementation plan
- ADRs per major architectural decision
- explicit CPU/GPU execution-boundary guidance in roadmap and benchmarking artifacts
