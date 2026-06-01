# Development Guide for Epics 1, 2, and 3

<!-- Status: current | validated: 2026-06-01 -->

## Goal

Use this guide when creating issues, documentation, headers, tests, or benchmarks for the hybrid retrieval, evaluation, and distributed tensor epics.

## Repository conventions for this roadmap

- Architecture and issue-level design lives in `docs/`.
- Planned runtime landing zones live in `src/retrieval/`, `src/evaluation/`, and `src/distributed_tensor/`.
- Planned validation landing zones live in `tests/epic1_retrieval/`, `tests/epic2_evaluation/`, and `tests/epic3_distributed_tensor/`.
- Planned performance landing zones live in `benchmarks/epic1_retrieval/`, `benchmarks/epic2_evaluation/`, and `benchmarks/epic3_distributed_tensor/`.

## Seven-phase template

### Phase 1: Design / API contract
- Freeze names, inputs, outputs, ownership, and invariants.
- Document open decision points and linked ADRs.

### Phase 2: Core implementation
- Add only the thinnest possible skeletons and extension points.
- Keep placeholder types and algorithms easy to remove or replace.

### Phase 3: Error handling and edge cases
- Enumerate invalid inputs, degraded modes, and fallback rules.
- Document when a layer is allowed to approximate and when it must fail closed.

### Phase 4: Tests
- Start with contract and regression tests.
- Add integration cases only once the public interface is stable.

### Phase 5: Performance and hardening
- Define the expected latency, memory, and rebuild budget for the component.
- Mark hardware-sensitive assumptions explicitly.

### Phase 6: Documentation and acceptance
- Update the relevant epic document, module README, and ADR references.
- Record acceptance signals that can be validated later.

### Phase 7: Integration
- Register the new files in the nearest CMake scaffold.
- Wire planner, observability, and cross-epic dependencies last.

## Definition of done for a new sub-issue
- A design document exists in `docs/`.
- The owning module README lists the planned header and source file names.
- Tests and benchmark directories already reserve their future file names.
- An ADR placeholder exists if the sub-issue changes architecture, artifact compatibility, or governance rules.
