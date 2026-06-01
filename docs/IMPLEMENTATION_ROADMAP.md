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

### Wave B: Documentation scaffold ✅ / Source files deferred
- Module-local `README.md`, `include/README.md`, `src/README.md`, and `CMakeLists.txt` are in place.
- Architecture, ADR, audit, security, and roadmap docs exist for all three epics.
- Source files (`*.cc`) and header files (`*.h`) are **out of scope for this PR**; they will be
  introduced in a dedicated implementation PR after contract review.
- Build targets remain commented out in CMake until source files are added and acceptance tests are approved.

### Wave C: Tests and benchmarks
- Add tests first for contract validation, then benchmarks for latency/throughput/cost trade-offs.
- Keep planner, retrieval, and distributed artifact scenarios aligned with the reference documents in `TARGET_ARCHITECTURE.md`, `EVALUATION_FRAMEWORK.md`, and `DISTRIBUTED_TENSOR_SHARDING.md`.

## 21 Sub-Issue Map

### EPIC 1: Hybrid knowledge retrieval architecture
- [ ] 1.1 ANN frontdoor — planned: `src/retrieval/include/ann_frontdoor.h` + `src/retrieval/src/ann_frontdoor.cc`
- [ ] 1.2 Tensor mid-layer — planned: `src/retrieval/include/tensor_midlayer.h` + `src/retrieval/src/tensor_midlayer.cc`
- [ ] 1.3 Graph truth validation — planned: `src/retrieval/include/graph_validator.h` + `src/retrieval/src/graph_validator.cc`
- [ ] 1.4 LoRA artifacts — planned: `src/retrieval/include/lora_package.h` + `src/retrieval/src/lora_package.cc`
- [ ] 1.5 Model switch workflow — planned: `src/retrieval/include/model_switch.h` + `src/retrieval/src/model_switch.cc`
- [ ] 1.6 Federated summaries — planned: `src/retrieval/include/federated_summaries.h` + `src/retrieval/src/federated_summaries.cc`
- [ ] 1.7 Observability and governance — planned: `src/retrieval/include/retrieval_observability.h` + `src/retrieval/src/retrieval_observability.cc`

### EPIC 2: Evaluation and benchmarking
- [ ] 2.1 Hardware profiles — planned: `src/evaluation/include/hardware_profile.h` + `src/evaluation/src/hardware_profile.cc`
- [ ] 2.2 Benchmark framework — planned: `src/evaluation/include/benchmark_matrix.h` + `src/evaluation/src/benchmark_matrix.cc`
- [ ] 2.3 Evaluation metrics — planned: `src/evaluation/include/evaluation_metrics.h` + `src/evaluation/src/evaluation_metrics.cc`
- [ ] 2.4 Approximation governance — planned: `src/evaluation/include/approximation_rules.h` + `src/evaluation/src/approximation_rules.cc`
- [ ] 2.5 Hybrid query planner — planned: `src/evaluation/include/query_planner.h` + `src/evaluation/src/query_planner.cc`
- [ ] 2.6 Artifact lifecycle — planned: `src/evaluation/include/artifact_lifecycle.h` + `src/evaluation/src/artifact_lifecycle.cc`
- [ ] 2.7 Storage strategy — planned: `src/evaluation/include/storage_strategy.h` + `src/evaluation/src/storage_strategy.cc`

### EPIC 3: Distributed tensor artifacts
- [ ] 3.1 Artifact classes — planned: `src/distributed_tensor/include/tensor_artifact_classes.h` + `.cc`
- [ ] 3.2 Manifest schema — planned: `src/distributed_tensor/include/artifact_manifest.h` + `.cc`
- [ ] 3.3 Shard placement — planned: `src/distributed_tensor/include/shard_placement.h` + `.cc`
- [ ] 3.4 Integrity model — planned: `src/distributed_tensor/include/integrity_verification.h` + `.cc`
- [ ] 3.5 Recovery strategy — planned: `src/distributed_tensor/include/recovery_manager.h` + `.cc`
- [ ] 3.6 Distributed retrieval — planned: `src/distributed_tensor/include/distributed_planner.h` + `.cc`
- [ ] 3.7 Tensor infrastructure — planned: `src/distributed_tensor/include/tensor_infrastructure.h` + `.cc`


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
