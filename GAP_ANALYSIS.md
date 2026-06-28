# GAP_ANALYSIS.md

# ThemisDB Gap Analysis
## From Current Hybrid RAG to Target Hybrid Knowledge Retrieval Architecture

**Status:** Updated to reflect GS3 full-scan and recent fixes
**Date:** 2026-06-27

**Update 2026-06-21 → 2026-06-27:** Completed GS3 full-codebase scan and follow-up actions. Key artefacts and changes are documented below. Wrapper Abstraction Excess scanner implemented and integrated; test-suite green.

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
- end-to-end provenance and package governance are largely implemented; remaining gaps are operationalization and rollout governance
	- **implementiert**: package-aware final-layer resolution and compatibility checks
	- **implementiert**: unified provenance chain (`RetrievalProvenanceRecord` in `include/observability/retrieval_provenance.h`) — links trigger → tensor candidates → graph evidence → final-layer resolution into one exportable, audit-ready record; emitted as `retrieval_provenance_record` JSON log per step()
	- **implementiert**: persistent provenance store (`IProvenanceStore` / `RocksDBProvenanceStore` in `include/observability/provenance_store.h`, `src/observability/provenance_store.cpp`) with query-id/step retrieval, chain queries, and time-range queries
	- **implementiert**: configurable retention policies in `RocksDBProvenanceStore::Config` (`retention_max_records`, `retention_max_age_ms`) with write-time pruning
	- **implementiert**: Tensor-RAG step persistence wiring via optional `TensorRAGPipelineConfig::provenance_store` (validated by focused tests)
	- **implementiert**: operational provenance export surfaces (API/CLI endpoints) — GET `/api/v1/observability/provenance` with query_id/time-range/limit filters; `themisctl provenance-export` CLI command with JSON/CSV output and file export (GAP-4.1, v1.9.0)

### 4.2 Distributed Retrieval
- no full tensor-summary-first federated retrieval execution path
	- **implementiert**: shard-aware ANN routing and federated tensor summary APIs
	- **implementiert**: end-to-end distributed execution policy in ANN Frontdoor (fan-out limits, deterministic merge semantics, shard retry/failover, partial-result and fail-closed fallback policy)
	- **implementiert**: cost-aware shard pruning with quality floor and budget-aware shard selection in `AnnFrontdoor::planRetrieval()` and `AnnFrontdoorResult` pruning metadata
- cross-shard retrieval cost control
	- **implementiert**: adaptive shard pruning based on utility scoring (relevance, freshness, locality) and cost budgets
	- **offen**: per-query guardrails and production load testing under SLO constraints; this is the remaining open item in 4.2

### 4.3 Observability
- layered tracing and reasoning observability are partially implemented and need production hardening
	- **implementiert**: per-layer correlation IDs propagated through all four layers
	- **implementiert**: routing-reason telemetry via `layer_decision_log.h` emitter (event `layer_handoff_decision`) in ANN / Tensor / Graph / Final Layer — validated by focused regression tests
	- **implementiert**: unified provenance chain log (`retrieval_provenance_record`) for decision lineage visibility
	- **offen**: production-grade dashboards and SLOs for ANN/Tensor/Graph/Final-Layer handoff quality (infrastructure concern); this is the remaining open item in 4.3

### 4.4 Lifecycle Management
- adapter/package/model lifecycle needs hardening beyond current baseline
	- **implementiert**: package registration/update/status + compatibility matrix
	- **implementiert**: promotion/rollback workflows + policy-gated deployment stages in `FinalLayerOrchestrator` (`promotePackage()`, `rollbackToPackage()`, deployment-stage serving gate)
	- **offen**: production release governance automation and operational runbook validation; this is the remaining open item in 4.4

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


## 7. GS3 Full Scan — Status & Findings (NEW)

Summary:
- Full GS3 run completed across the repository (scan execution: 2026-06-21).
- Total gaps detected: **130,897** across **2,713** files.
- Scanners executed: **33** modules yielding **46** gap type categories.
- Urgent issues (CRITICAL + HIGH): **16,005** — prioritized for remediation.

Key scan artefacts:
- Raw JSON scan output: `ai_working/gs3_quick_scan.json` (~67.5 MB)
- Comprehensive analysis: `ai_working/GS3_SCAN_REPORT_2026_06_21.md`
- German executive summary: `ai_working/GS3_FULL_SCAN_RESULTS_2026_06_21.md`

Wrapper Abstraction Excess Scanner (Boring-Code detection):
- File: `tools/scanners/gs3_step04_design_wrapper_abstraction_excess.py`
- Purpose: detect thin wrappers, passthrough methods, and abstraction cascades (severity by wrapper depth)
- Tests: `tools/scanners/test_gs3_wrapper_abstraction.py` — **5 tests, all passed**
- Docs: `tools/WRAPPER_ABSTRACTION_EXCESS_GUIDE.md` (implementation guide + remediation patterns)
- Integration: Registered in GS3 orchestrator/phase list — executes automatically in Phase 7-10 runs

Top-level automated findings (examples):
- `todo_as_productionlogic`: 1,312 CRITICAL
- `circular_lock_ordering`: 1,105 HIGH
- `db_connection_leak`: 626 HIGH

Primary consequences and interpretation:
- The scan reveals concentrated high-severity issues alongside many medium findings; a focused remediation plan is required to reduce risk to acceptable levels before broad production rollout.
- The wrapper-scanner allows direct targeting of over-abstraction hotspots introduced by automated code generation or poor layering.

Immediate recommended actions (next 30 days):
1. Run the Wrapper Abstraction Excess scanner across the full repo to generate a wrapper-specific baseline (command shown below).
2. Produce a filtered analysis of wrapper gaps and top-20 files (by severity + wrapper_depth).
3. Create GitHub issues for CRITICAL/HIGH gaps with automated labels and suggested remediations (triage sprint backlog).
4. Deploy `ci_gs3_validate.py` into `.github/workflows/` to enable PR-level scans; gate high-severity findings as review blockers.
5. Build a remediation sprint plan: owners, estimates, and acceptance criteria (focus first on CRITICAL+HIGH bundles).

Commands to run locally (examples):
```powershell
# Full GS3 scan (already executed previously):
python tools/gs3.py scan . --scan-mode full --output ai_working/gs3_quick_scan.json

# Run only wrapper scanner and output JSON (for rapid baseline):
python -c "from scanners.gs3_step04_design_wrapper_abstraction_excess import WrapperAbstractionExcessScanner; s=WrapperAbstractionExcessScanner(); s.scan('.'); import json; print(json.dumps([g.to_dict() for g in s.gaps]))" > ai_working/gs3_wrapper_gaps.json
```

Notes on automation and governance:
- CI integration is prepared but not yet deployed to GitHub Actions; this is intentionally staged to allow team review of false-positive thresholds and remediation playbooks.
- The GS3 analysis tool (`ai_working/analyze_gs3_final.py`) exists and produces prioritized reports; use it to generate per-subsystem dashboards and CSV exports for issue creation.

Ownership & next contact points:
- GS3 owner for remediation coordination: `@team-retrieval` (assign owners per subsystem)
- Suggested triage owner for wrapper remediation: `@team-architecture`

Closing remark:
- The architectural layer gaps identified earlier (ANN/Tensor/Graph/Final Layer) remain valid — the new GS3 findings add an operational quality dimension (technical debt, over-abstraction, and hotspots) that should be incorporated into the remediation roadmap and sprint planning.
