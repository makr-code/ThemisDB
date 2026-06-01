# EPIC 1.6 Federated Summaries

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Summary

Cross-shard tensor summary and summary-first retrieval planning.

## Scope

- Cross-shard summary exchange and merge rules
- Federated routing hints for planner and retrieval layers
- Shard-local freshness and integrity interaction

## Planned Repository Surfaces

- `src/retrieval/include/federated_summaries.h`
- `src/retrieval/src/federated_summaries.cc`
- `tests/epic1_retrieval/federated_summaries_test.cc`
- `benchmarks/epic1_retrieval/federated_bench.cc`

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [ ] Define summary exchange schema for shard-local and federated modes
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.6 Federated Summaries`.

### Phase 2: Core implementation
- [ ] Document how cross-shard summaries are ranked before expensive retrieval
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.6 Federated Summaries`.

### Phase 3: Error handling and edge cases
- [ ] Record failure modes for partial shard coverage or stale summary windows
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.6 Federated Summaries`.

### Phase 4: Tests
- [ ] Keep benchmark hooks for merge cost and summary fan-out
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.6 Federated Summaries`.

### Phase 5: Performance and hardening
- [ ] Align planner-facing metadata with EPIC 3 distributed retrieval documents
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.6 Federated Summaries`.

### Phase 6: Documentation and acceptance
- [ ] Align planner-facing metadata with EPIC 3 distributed retrieval documents
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.6 Federated Summaries`.

### Phase 7: Integration
- [ ] Align planner-facing metadata with EPIC 3 distributed retrieval documents
- [ ] Wire the planned files into the nearest CMake and cross-epic integration checkpoints for `EPIC 1.6 Federated Summaries`.

## Acceptance Signals

- The planned repository surfaces remain stable enough for issue creation and ownership.
- The document names the dependencies, failure modes, and validation hooks needed before code lands.
- Tests and benchmarks have reserved file names before implementation starts.

## References

- `docs/EPIC2_QUERY_PLANNER.md`
- `docs/EPIC3_DISTRIBUTED_RETRIEVAL.md`
- `docs/EPIC3_MANIFEST_SCHEMA.md`
