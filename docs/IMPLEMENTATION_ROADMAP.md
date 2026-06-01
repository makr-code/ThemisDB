# Implementation Roadmap for Epics 1, 2, and 3

<!-- Status: current | validated: 2026-06-01 -->

## Purpose

This roadmap collects the implementation order, documentation checkpoints, and repository landing zones for the three architecture epics introduced in `ISSUE_SET.md`.

## Epic Overview

| Epic | Goal | Primary landing zones |
|---|---|---|
| EPIC 1 | Build the layered retrieval stack from ANN through graph-backed validation and model selection | `docs/EPIC1_*.md`, `src/retrieval/`, `tests/epic1_retrieval/`, `benchmarks/epic1_retrieval/` |
| EPIC 2 | Make retrieval choices measurable, hardware-aware, and policy-governed | `docs/EPIC2_*.md`, `src/evaluation/`, `tests/epic2_evaluation/`, `benchmarks/epic2_evaluation/` |
| EPIC 3 | Define portable, distributed tensor artifacts with placement, integrity, and recovery semantics | `docs/EPIC3_*.md`, `src/distributed_tensor/`, `tests/epic3_distributed_tensor/`, `benchmarks/epic3_distributed_tensor/` |

## Delivery Waves

### Wave A: Contracts and decisions
- Complete all architecture and ADR documents before adding production code.
- Freeze public header names, artifact names, and planner vocabulary.
- Use `docs/EPIC1_2_3_DEPENDENCIES.md` to sequence cross-epic work.

### Wave B: Skeleton implementation surfaces
- Create module-local `README.md`, `include/README.md`, `src/README.md`, and placeholder `CMakeLists.txt` files.
- Keep build targets disabled until interfaces have stable acceptance criteria.
- Treat all file lists in the new CMake scaffolds as the source of truth for future code landing zones.

### Wave C: Tests and benchmarks
- Add tests first for contract validation, then benchmarks for latency/throughput/cost trade-offs.
- Keep planner, retrieval, and distributed artifact scenarios aligned with the reference documents in `TARGET_ARCHITECTURE.md`, `EVALUATION_FRAMEWORK.md`, and `DISTRIBUTED_TENSOR_SHARDING.md`.

## 21 Sub-Issue Map

### EPIC 1: Hybrid knowledge retrieval architecture
- [ ] 1.1 ANN frontdoor — planning tracked in `docs/EPIC1_ANN_FRONTDOOR.md`
- [ ] 1.2 Tensor mid-layer — planning tracked in `docs/EPIC1_TENSOR_MIDLAYER.md`
- [ ] 1.3 Graph truth validation — planning tracked in `docs/EPIC1_GRAPH_VALIDATION.md`
- [ ] 1.4 LoRA artifacts — planning tracked in `docs/EPIC1_LORA_ARTIFACTS.md`
- [ ] 1.5 Model switch workflow — planning tracked in `docs/EPIC1_MODEL_SWITCH.md`
- [ ] 1.6 Federated summaries — planning tracked in `docs/EPIC1_FEDERATED_SUMMARIES.md`
- [ ] 1.7 Observability and governance — planning tracked in `src/retrieval/README.md`

### EPIC 2: Evaluation and benchmarking
- [ ] 2.1 Hardware profiles — planning tracked in `docs/EPIC2_HARDWARE_PROFILES.md`
- [ ] 2.2 Benchmark framework — planning tracked in `docs/EPIC2_BENCHMARK_FRAMEWORK.md`
- [ ] 2.3 Evaluation metrics — planning tracked in `docs/EPIC2_EVALUATION_METRICS.md`
- [ ] 2.4 Approximation governance — planning tracked in `docs/EPIC2_APPROXIMATION_GOVERNANCE.md`
- [ ] 2.5 Hybrid query planner — planning tracked in `docs/EPIC2_QUERY_PLANNER.md`
- [ ] 2.6 Artifact lifecycle — planning tracked in `docs/EPIC2_ARTIFACT_LIFECYCLE.md`
- [ ] 2.7 Storage strategy — planning tracked in `src/evaluation/README.md`

### EPIC 3: Distributed tensor artifacts
- [ ] 3.1 Artifact classes — planning tracked in `docs/EPIC3_ARTIFACT_CLASSES.md`
- [ ] 3.2 Manifest schema — planning tracked in `docs/EPIC3_MANIFEST_SCHEMA.md`
- [ ] 3.3 Shard placement — planning tracked in `docs/EPIC3_SHARD_PLACEMENT.md`
- [ ] 3.4 Integrity model — planning tracked in `docs/EPIC3_INTEGRITY_MODEL.md`
- [ ] 3.5 Recovery strategy — planning tracked in `docs/EPIC3_RECOVERY_STRATEGY.md`
- [ ] 3.6 Distributed retrieval — planning tracked in `docs/EPIC3_DISTRIBUTED_RETRIEVAL.md`
- [ ] 3.7 Tensor infrastructure — planning tracked in `src/distributed_tensor/README.md`


## Seven-Phase Completion Rule

Every new implementation file planned by these epics should advance through the same checklist:
- [ ] Phase 1: Design / API contract
- [ ] Phase 2: Core implementation skeleton
- [ ] Phase 3: Error handling and edge cases
- [ ] Phase 4: Tests
- [ ] Phase 5: Performance and hardening
- [ ] Phase 6: Documentation and acceptance
- [ ] Phase 7: Integration

## Exit Criteria

- Each epic has architecture documentation, ADR placeholders, and repository-local module scaffolding.
- Each planned header/source/test/benchmark file has a documented home before implementation begins.
- Cross-epic dependencies are explicit enough to support issue creation and milestone planning.
