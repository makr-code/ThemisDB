# GAP_ANALYSIS.md

# ThemisDB Gap Analysis
## From Current Hybrid RAG to Target Hybrid Knowledge Retrieval Architecture

**Status:** Draft  
**Date:** 2026-06-01

**Update 2026-06-17:** ANN-Frontdoor-Gap (Abschnitt 3.1) implementiert:
`include/index/ann_frontdoor.h`, `src/index/ann_frontdoor.cpp`,
`tests/index/test_ann_frontdoor.cpp`.

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

The current system is a capable modular hybrid stack. The ANN, Tensor, Graph, and Final-Layer gaps are now closed as explicit orchestration stages; remaining work is primarily cross-cutting hardening, observability, and lifecycle governance.

---

## 3. Layer-by-Layer Gaps

## 3.1 ANN Frontdoor Gaps

### Present
- vector-oriented retrieval logic
- embedding-aware retrieval concepts

### Missing / Incomplete
- explicit ANN-frontdoor abstraction → **implementiert**: `AnnFrontdoor` in `include/index/ann_frontdoor.h`
- unified HNSW/DiskANN strategy → **implementiert** in `AnnFrontdoor::planRetrieval()` / `planStrategy()`
- ANN for adapters/packages/shard summaries → **implementiert** über `AnnScopeKind`, AdapterRepository-Scopes und shard-aware routing
- clear hot/cold retrieval planning → **implementiert** in `AnnRetrievalPlan` / `planRetrieval()` und an den Search-/Adapter-Aufrufstellen verdrahtet

### Consequence
Semantic retrieval is now formalized as the first universal retrieval gate in the ANN path; remaining work is in higher-layer tensor/graph/LLM orchestration.

### Current Focus
- cross-cutting observability and reasoning traces
- stronger end-to-end provenance and package governance
- lifecycle hardening around package/model operations

---

## 3.2 Tensor Mid-Layer Gaps

### Present
- tensor-oriented subsystems and components
- LoRA/AdaLoRA alignment with tensor concepts
- summary/compression potential

### Missing / Incomplete
- explicit tensor mid-layer abstraction → **implementiert**: `TensorMidLayer` in `include/tensor/tensor_mid_layer.h`
- unified tensor fingerprints → **implementiert** über `TensorFingerprintGraph`
- tensor routing and summary APIs → **implementiert** über `TensorLayerPlan`, `TensorLayerSummary`, `summarize()` und `summarizeFederatedShards()`
- cross-layer use of tensor structures in RAG → **implementiert** über `TensorRAGPipeline::setTensorMidLayer()` und tensor summary propagation im `RAGDecision`
- federated shard summary model → **implementiert** als erste Mid-Layer-Stufe über `FederatedTensorSummary`

### Consequence
Tensor capabilities are now elevated into a coherent system-level compression and routing layer; remaining work shifts to graph-truth validation and final-layer LLM/LoRA orchestration.

---

## 3.3 Graph Truth Layer Gaps

### Present
- graph relations
- constraints and provenance-friendly modeling
- evidence potential

### Missing / Incomplete
- consistent graph evidence assembly flow → **implementiert** über `GraphTruthValidator` und `GraphTruthEvidence`
- explicit graph-based validation stage in the target retrieval pipeline → **implementiert** über `TensorRAGPipeline::setGraphTruthValidator()`
- strong integration with tensor summary outputs → **implementiert** über TensorMidLayer → GraphTruthValidator Übergabe

### Consequence
Graph is now formalized as the explicit post-approximation validation layer for the ANN → Tensor → Graph path; remaining work shifts primarily to final-layer LLM/LoRA orchestration and lifecycle maturity.

---

## 3.4 LLM / LoRA Final Layer Gaps

### Present
- LLM/LoRA/training subsystems
- adaptation-oriented architecture
- registry and provenance-adjacent ideas

### Missing / Incomplete
- package-oriented adapter lifecycle → **implementiert** über `FinalLayerPackage`, `registerPackage()`, `updatePackage()` und `setPackageStatus()`
- explicit final-layer orchestration over layered context → **implementiert** über `FinalLayerOrchestrator` und `TensorRAGPipeline::setFinalLayerOrchestrator()`
- robust model-switch workflow → **implementiert** über paketbasierte Auflösung, Router-Fallback und Draft-Adapter-Erkennung
- compatibility matrix and rebuild-first logic → **implementiert** über `buildCompatibilityMatrix()` und `resolve()`-Kompatibilitätsprüfung

### Consequence
The final generation layer is now formalized as the explicit ANN → Tensor → Graph → LLM/LoRA completion stage, with package-aware adapter resolution and compatibility-aware model switching wired into the retrieval pipeline.

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
- expand layered observability across ANN → Tensor → Graph → Final Layer
- identify quick wins in tracing, diagnostics, and failure visibility

## Priority 2
- strengthen provenance and package governance end-to-end
- close remaining lifecycle-management rough edges

## Priority 3
- optimize federated shard-summary retrieval behavior
- validate cross-shard cost and merge quality under load

## Priority 4
- harden package/model switching with broader integration coverage
- extend compatibility policies where rebuild-first constraints evolve

## Priority 5
- prepare federated shard summary architecture

## Priority 6
- broaden end-to-end orchestration coverage in builds, focused tests, and operational docs

---

## 6. Next Deliverables

- issue tree for implementation
- per-layer technical inventory
- ADR set
- research-backed design documents
