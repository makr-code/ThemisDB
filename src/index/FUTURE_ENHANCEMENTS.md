# Index Module - Future Enhancements

<!-- Status: current | validated: 2026-09-24 -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->

## Scope

- hardening and refinement of index runtime behavior across core and accelerated paths
- expansion of deterministic reliability under mixed backend and lifecycle operations
- stronger benchmark-backed guardrails for index hot paths

## Design Constraints

- index contracts remain backward compatible within major release line.
- core lookup/mutation behavior remains deterministic under bounded constraints.
- backend degradation paths remain explicit and observable.
- lifecycle operations remain auditable and operationally controllable.

## Required Interfaces

| Interface | Requirement |
|---|---|
| core index interfaces | deterministic mutation/search semantics |
| acceleration interfaces | explicit backend-aware execution with bounded fallback |
| lifecycle interfaces | stable rebuild/tiering/distribution behavior |
| optimization interfaces | bounded adaptive/replay-driven tuning behavior |

## Implementation Notes (v1.4.0)

### GPU Vector Index Backend - CUDA (In Progress)
- Implemented CUDA kernels for vector similarity operations:
  - L2 (squared Euclidean) distance computation
  - Cosine distance with normalization
  - Inner product (max-IP search)
- Batch vector search with Thrust library for efficient sorting/reduction
- Device memory management with GPU memory manager
- Stream-based async kernel execution
- FAISS/RAFT integration for advanced search algorithms

### GPU Vector Index Backend - HIP (In Progress)
- HIP equivalent kernels for AMD GPU support
- Rocm Thrust for AMD device operations
- hipBLAS for matrix operations (dot product, GEMM)
- HIP stream management and device memory allocation
- Feature-parity with CUDA backend

### Broader Hardening (v1.4.0+)
- tighten parity across CPU/GPU/vector backends and fallback pathways.
- standardize diagnostics for rebuild/tiering/distribution incidents.
- expand resilience tests for prolonged retrieval/update workloads.
- broaden benchmark depth for advanced quantization and distributed scenarios.

## Test Strategy

- unit and integration suites for vector/secondary/spatial/graph index flows.
- regressions for unsupported backend and lifecycle-degradation scenarios.
- deterministic stress runs for high-concurrency index operations.
- release-profile benchmark runs for mapped index targets.

## Performance Targets

- core lookup/mutation and control-plane operations remain inside regression budgets.
- index hot paths remain stable at p95/p99 envelopes.
- mapped benchmark manifests reach no-missing-case status for release gating.

## Security / Reliability

- maintain strict validation before index mutation/rebuild operations.
- preserve explicit fallback signaling for backend capability failures.
- enforce bounded lifecycle behavior under degraded runtime conditions.
- keep diagnostics actionable for production index incidents.
## RAG-Readiness Audit Backlog (2026-09-23)

- [ ] Representative-Hardware Parity Gates fuer ANN/GPU aktivieren (Target: Q4 2026)
  - Rationale: RAG-Produktivfaehigkeit haengt an reproduzierbarer ANN/GPU-Paritaet unter Realhardware.
  - Umsetzungsschritte: (1) CUDA/HIP/Vulkan Paritaetsmatrix definieren, (2) p95/p99 + recall-Paritaet je Backend messen, (3) fail-closed bei nicht erreichter Paritaet.
  - Abhaengigkeiten: `src/index`, `src/gpu`, `benchmarks/index`, `tests/index`.
  - Messbares DoD: Recall-Differenz CPU vs GPU <=1 Prozentpunkt bei Top-10; p95-Latenzbudget pro Backend dokumentiert und gate-gebunden.

- [ ] Embedding-/Index-Schema-Versionvertrag erzwingen (Target: Q1 2027)
  - Rationale: Ohne strikt versionierte Lifecycle-Regeln drohen stille Retrieval-Regressions bei Modellwechseln.
  - Umsetzungsschritte: (1) `embedding_model_id`, `embedding_dim`, `chunking_profile`, `index_schema_version` verpflichtend persistieren, (2) Reindex-Trigger-Regeln implementieren, (3) dual-read migration path fuer Releases.
  - Abhaengigkeiten: `src/llm/wiki_index_store.cpp`, `src/ingestion`, `src/storage`.
  - Messbares DoD: 100% neu geschriebener Vektor-Entitaeten tragen vollständige Version-Metadaten; Reindex-Entscheid deterministisch nachvollziehbar.
