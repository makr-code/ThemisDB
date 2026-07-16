# EPIC 1.2 Tensor Mid-Layer

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Summary

Tensor summary abstractions for compressed, summary-first retrieval.

## Scope

- Tensor compression families and summary types
- Shard-local and cross-shard summary routing
- Compatibility with ANN candidate expansion and graph validation

## Planned Repository Surfaces

- `src/retrieval/include/tensor_midlayer.h`
- `src/retrieval/src/tensor_midlayer.cc`
- `tests/epic1_retrieval/tensor_midlayer_test.cc`
- `benchmarks/epic1_retrieval/tensor_routing_bench.cc`

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [ ] Define tensor summary object types and routing metadata
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.2 Tensor Mid-Layer`.

### Phase 2: Core implementation
- [ ] Document compression and quantization assumptions separately from storage policy
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.2 Tensor Mid-Layer`.

### Phase 3: Error handling and edge cases
- [ ] Enumerate degraded paths when summaries are stale, partial, or missing
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.2 Tensor Mid-Layer`.

### Phase 4: Tests
- [ ] Keep benchmark hooks for compression ratio and routing latency visible
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.2 Tensor Mid-Layer`.

### Phase 5: Performance and hardening
- [ ] Link federated summary behavior to EPIC 3 manifest work
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.2 Tensor Mid-Layer`.

### Phase 6: Documentation and acceptance
- [ ] Link federated summary behavior to EPIC 3 manifest work
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 1.2 Tensor Mid-Layer`.

### Phase 7: Integration
- [ ] Link federated summary behavior to EPIC 3 manifest work
- [ ] Wire the planned files into the nearest CMake and cross-epic integration checkpoints for `EPIC 1.2 Tensor Mid-Layer`.

## Acceptance Signals

- The planned repository surfaces remain stable enough for issue creation and ownership.
- The document names the dependencies, failure modes, and validation hooks needed before code lands.
- Tests and benchmarks have reserved file names before implementation starts.

## References

- `DISTRIBUTED_TENSOR_SHARDING.md`
- `docs/EPIC1_FEDERATED_SUMMARIES.md`
- `docs/EPIC3_MANIFEST_SCHEMA.md`
