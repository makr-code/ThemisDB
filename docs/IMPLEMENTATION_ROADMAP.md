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

### Wave B: Skeleton implementation surfaces ✅
- Module-local `README.md`, `include/README.md`, `src/README.md`, and `CMakeLists.txt` are in place.
- All 21 planned header files exist in `include/` with documented API contracts.
- All 21 planned source files exist in `src/` as minimal stubs with factory functions.
- Build targets remain commented out in CMake until acceptance tests are approved.

### Wave C: Tests and benchmarks
- Add tests first for contract validation, then benchmarks for latency/throughput/cost trade-offs.
- Keep planner, retrieval, and distributed artifact scenarios aligned with the reference documents in `TARGET_ARCHITECTURE.md`, `EVALUATION_FRAMEWORK.md`, and `DISTRIBUTED_TENSOR_SHARDING.md`.

## 21 Sub-Issue Map

### EPIC 1: Hybrid knowledge retrieval architecture
- [x] 1.1 ANN frontdoor — skeleton in `src/retrieval/include/ann_frontdoor.h` + `src/ann_frontdoor.cc`
- [x] 1.2 Tensor mid-layer — skeleton in `src/retrieval/include/tensor_midlayer.h` + `src/tensor_midlayer.cc`
- [x] 1.3 Graph truth validation — skeleton in `src/retrieval/include/graph_validator.h` + `src/graph_validator.cc`
- [x] 1.4 LoRA artifacts — skeleton in `src/retrieval/include/lora_package.h` + `src/lora_package.cc`
- [x] 1.5 Model switch workflow — skeleton in `src/retrieval/include/model_switch.h` + `src/model_switch.cc`
- [x] 1.6 Federated summaries — skeleton in `src/retrieval/include/federated_summaries.h` + `src/federated_summaries.cc`
- [x] 1.7 Observability and governance — skeleton in `src/retrieval/include/retrieval_observability.h` + `src/retrieval_observability.cc`

### EPIC 2: Evaluation and benchmarking
- [x] 2.1 Hardware profiles — skeleton in `src/evaluation/include/hardware_profile.h` + `src/hardware_profile.cc`
- [x] 2.2 Benchmark framework — skeleton in `src/evaluation/include/benchmark_matrix.h` + `src/benchmark_matrix.cc`
- [x] 2.3 Evaluation metrics — skeleton in `src/evaluation/include/evaluation_metrics.h` + `src/evaluation_metrics.cc`
- [x] 2.4 Approximation governance — skeleton in `src/evaluation/include/approximation_rules.h` + `src/approximation_rules.cc`
- [x] 2.5 Hybrid query planner — skeleton in `src/evaluation/include/query_planner.h` + `src/query_planner.cc`
- [x] 2.6 Artifact lifecycle — skeleton in `src/evaluation/include/artifact_lifecycle.h` + `src/artifact_lifecycle.cc`
- [x] 2.7 Storage strategy — skeleton in `src/evaluation/include/storage_strategy.h` + `src/storage_strategy.cc`

### EPIC 3: Distributed tensor artifacts
- [x] 3.1 Artifact classes — skeleton in `src/distributed_tensor/include/tensor_artifact_classes.h` + `.cc`
- [x] 3.2 Manifest schema — skeleton in `src/distributed_tensor/include/artifact_manifest.h` + `.cc`
- [x] 3.3 Shard placement — skeleton in `src/distributed_tensor/include/shard_placement.h` + `.cc`
- [x] 3.4 Integrity model — skeleton in `src/distributed_tensor/include/integrity_verification.h` + `.cc`
- [x] 3.5 Recovery strategy — skeleton in `src/distributed_tensor/include/recovery_manager.h` + `.cc`
- [x] 3.6 Distributed retrieval — skeleton in `src/distributed_tensor/include/distributed_planner.h` + `.cc`
- [x] 3.7 Tensor infrastructure — skeleton in `src/distributed_tensor/include/tensor_infrastructure.h` + `.cc`


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
