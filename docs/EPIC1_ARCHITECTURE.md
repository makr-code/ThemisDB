# EPIC 1 Architecture

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Summary

Layered retrieval design for ANN, tensor, graph, and LLM-backed reasoning.

## Scope

- Stage 1 candidate retrieval through ANN frontdoor contracts
- Stage 2 tensor summary routing and compression-aware selection
- Stage 3 graph truth validation and evidence assembly
- Stage 4 model-switch and adapter-aware answer generation

## Planned Repository Surfaces

- `src/retrieval/include/ann_frontdoor.h`
- `src/retrieval/include/tensor_midlayer.h`
- `src/retrieval/include/graph_validator.h`
- `src/retrieval/include/model_switch.h`

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [ ] Define common request and candidate vocabulary across all retrieval layers
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1 Architecture`.

### Phase 2: Core implementation
- [ ] Specify how evidence and provenance survive layer transitions
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1 Architecture`.

### Phase 3: Error handling and edge cases
- [ ] Reserve observability hooks for latency, fallback, and confidence tracing
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1 Architecture`.

### Phase 4: Tests
- [ ] Describe planner and federated summary touch points before implementation begins
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1 Architecture`.

### Phase 5: Performance and hardening
- [ ] Document acceptance criteria for the end-to-end layered path
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1 Architecture`.

### Phase 6: Documentation and acceptance
- [ ] Document acceptance criteria for the end-to-end layered path
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1 Architecture`.

### Phase 7: Integration
- [ ] Document acceptance criteria for the end-to-end layered path
- [ ] Wire the planned files into the nearest CMake and cross-epic integration checkpoints for `EPIC 1 Architecture`.

## Acceptance Signals

- The planned repository surfaces remain stable enough for issue creation and ownership.
- The document names the dependencies, failure modes, and validation hooks needed before code lands.
- Tests and benchmarks have reserved file names before implementation starts.

## References

- `ISSUE_SET.md`
- `TARGET_ARCHITECTURE.md`
- `docs/EPIC2_QUERY_PLANNER.md`
- `docs/EPIC3_DISTRIBUTED_RETRIEVAL.md`
