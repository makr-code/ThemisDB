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
- approximation boundaries are only partially formalized across layers
	- **implementiert**: explicit stage boundaries ANN -> Tensor -> Graph -> Final Layer
	- **implementiert**: cross-layer policy contracts via `reason_codes.h`, `FallbackMode` enum, and ADR E2-005
- end-to-end provenance and package governance remain incomplete
	- **implementiert**: package-aware final-layer resolution and compatibility checks
	- **implementiert**: unified provenance chain (`RetrievalProvenanceRecord` in `include/observability/retrieval_provenance.h`) — links trigger → tensor candidates → graph evidence → final-layer resolution into one exportable, audit-ready record; emitted as `retrieval_provenance_record` JSON log per step()
	- **offen**: persistent provenance store and queryable provenance export (post-generation lineage)

### 4.2 Distributed Retrieval
- no full tensor-summary-first federated retrieval execution path
	- **implementiert**: shard-aware ANN routing and federated tensor summary APIs
	- **implementiert**: end-to-end distributed execution policy in ANN Frontdoor (fan-out limits, deterministic merge semantics, shard retry/failover, partial-result and fail-closed fallback policy)
- cross-shard retrieval cost control is still incomplete
	- **offen**: adaptive shard pruning, cost-aware candidate budgeting, and quality/cost guardrails under load

### 4.3 Observability
- layered tracing and reasoning observability need expansion
	- **implementiert**: per-layer correlation IDs propagated through all four layers
	- **implementiert**: routing-reason telemetry via `layer_decision_log.h` emitter (event `layer_handoff_decision`) in ANN / Tensor / Graph / Final Layer — validated by focused regression tests
	- **implementiert**: unified provenance chain log (`retrieval_provenance_record`) for decision lineage visibility
	- **offen**: production-grade dashboards/SLOs for ANN/Tensor/Graph/Final-Layer handoff quality (infrastructure concern)

### 4.4 Lifecycle Management
- adapter/package/model lifecycle needs hardening beyond current baseline
	- **implementiert**: package registration/update/status + compatibility matrix
	- **implementiert**: promotion/rollback workflows + policy-gated deployment stages in `FinalLayerOrchestrator` (`promotePackage()`, `rollbackToPackage()`, deployment-stage serving gate)
	- **offen**: operational runbooks and production release governance automation

---

## 5. Recommended Priorities

## Priority 1
- expand layered observability across ANN → Tensor → Graph → Final Layer
- identify quick wins in tracing, diagnostics, and failure visibility

## Priority 2
- strengthen provenance and package governance end-to-end
- formalize cross-layer fallback and confidence-governance contracts

## Priority 3
- optimize federated shard-summary retrieval behavior
- validate cross-shard cost and merge quality under load

## Priority 4
- harden package/model switching with broader integration coverage
- implement promotion/rollback and policy-gated lifecycle transitions

## Priority 5
- implement full distributed execution policy on top of federated summaries

## Priority 6
- broaden end-to-end orchestration coverage in builds, focused tests, and operational docs

---

## 6. Next Deliverables

- cross-cutting issue tree (implemented): `docs/implementation/CROSS_CUTTING_GAPS_ISSUE_TREE_2026-06-17.md`
- cross-layer governance ADRs (partially implemented):
	- `docs/adr/adr-e2-005-cross-layer-fallback-confidence-policy.md`
- observability spec and metric taxonomy for ANN/Tensor/Graph/Final-Layer handoff (partially implemented):
	- `docs/implementation/CROSS_LAYER_OBSERVABILITY_SPEC_2026-06-17.md`
- distributed retrieval execution design (partially implemented):
	- `docs/implementation/DISTRIBUTED_RETRIEVAL_EXECUTION_DESIGN_2026-06-17.md`
- lifecycle runbook for package promotion, rollback, and compatibility-gated release (partially implemented):
	- `docs/implementation/LIFECYCLE_PROMOTION_ROLLBACK_RUNBOOK_2026-06-17.md`
