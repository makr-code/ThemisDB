# GAP_ANALYSIS.md

# ThemisDB Gap Analysis
## From Current Hybrid RAG to Target Hybrid Knowledge Retrieval Architecture

**Status:** Draft  
**Date:** 2026-06-01

---

## 1. Purpose

This document describes the high-level architectural gaps between the current state of ThemisDB and the targeted layered architecture:

- ANN Frontdoor
- Tensor Mid-Layer
- Graph Truth Layer
- LLM / LoRA Final Layer

---

## 2. Summary

ThemisDB already contains strong capabilities in graph, vector retrieval, LoRA/training, provenance, and sharding. However, these capabilities are not yet fully integrated into a clearly layered end-to-end knowledge retrieval architecture.

### High-Level Gap Statement

The current system is a capable modular hybrid stack, but it is not yet a fully orchestrated layered knowledge architecture.

---

## 3. Layer-by-Layer Gaps

## 3.1 ANN Frontdoor Gaps

### Present
- vector-oriented retrieval logic
- embedding-aware retrieval concepts

### Missing / Incomplete
- explicit ANN-frontdoor abstraction
- unified HNSW/DiskANN strategy
- ANN for adapters/packages/shard summaries
- clear hot/cold retrieval planning

### Consequence
Semantic retrieval is available, but not yet formalized as the first universal retrieval gate.

---

## 3.2 Tensor Mid-Layer Gaps

### Present
- tensor-oriented subsystems and components
- LoRA/AdaLoRA alignment with tensor concepts
- summary/compression potential

### Missing / Incomplete
- explicit tensor mid-layer abstraction
- unified tensor fingerprints
- tensor routing and summary APIs
- cross-layer use of tensor structures in RAG
- federated shard summary model

### Consequence
Tensor capabilities exist but are not yet a coherent system-level compression and routing layer.

---

## 3.3 Graph Truth Layer Gaps

### Present
- graph relations
- constraints and provenance-friendly modeling
- evidence potential

### Missing / Incomplete
- consistent graph evidence assembly flow
- explicit graph-based validation stage in the target retrieval pipeline
- strong integration with tensor summary outputs

### Consequence
Graph is strong, but not yet fully formalized as the exact post-approximation validation layer.

---

## 3.4 LLM / LoRA Final Layer Gaps

### Present
- LLM/LoRA/training subsystems
- adaptation-oriented architecture
- registry and provenance-adjacent ideas

### Missing / Incomplete
- package-oriented adapter lifecycle
- explicit final-layer orchestration over layered context
- robust model-switch workflow
- compatibility matrix and rebuild-first logic

### Consequence
The final generation layer exists, but lifecycle and orchestration maturity remain incomplete.

---

## 4. Cross-Cutting Gaps

### 4.1 Governance
- insufficiently formalized approximation boundaries
- incomplete end-to-end provenance and package governance

### 4.2 Distributed Retrieval
- no full tensor-summary-first federated retrieval path
- cross-shard retrieval may remain more expensive than necessary

### 4.3 Observability
- layered tracing and reasoning observability need expansion

### 4.4 Lifecycle Management
- adapter/package/model lifecycle still needs stronger structure

---

## 5. Recommended Priorities

## Priority 1
- formalize ANN frontdoor
- identify quick wins in retrieval routing and candidate reduction

## Priority 2
- define tensor mid-layer abstractions
- introduce tensor fingerprints and summaries

## Priority 3
- formalize graph truth validation stage
- align provenance with graph evidence flow

## Priority 4
- move LoRA toward package/product lifecycle
- add model-switch compatibility workflow

## Priority 5
- prepare federated shard summary architecture

---

## 6. Next Deliverables

- issue tree for implementation
- per-layer technical inventory
- ADR set
- research-backed design documents
