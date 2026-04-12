# include llama_cpp roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
- [x] Header contract for llama.cpp plugin is available (Target: Q2 2026)

## In Progress
- [ ] Formalize ABI constraints for plugin host boundaries (Target: Q3 2026)

## Implementation Phases
### Phase 1: Design / API Contract
- [x] `LlamaCppPlugin` API declared (Target: Q2 2026)
### Phase 2: Core Implementation
- [x] Source implementation exists in `src/llama_cpp` (Target: Q2 2026)
### Phase 3: Error Handling & Edge Cases
- [x] Optional model-loaded path and stats API exposed (Target: Q2 2026)
### Phase 4: Tests
- [ ] Add include/API compatibility tests for plugin loading boundaries (Target: Q3 2026)
### Phase 5: Performance/Hardening
- [x] Track compatibility for high-throughput concurrent inference calls (`bench_llama_cpp_inference.cpp`): generate/batch/embed/stream latency, concurrent threads, stub+real model (Target: Q3 2026)
### Phase 6: Documentation & Acceptance
- [x] Baseline include module docs created (Target: Q2 2026)

## Production Readiness Checklist
- [x] Header contract exists for runtime integration
- [ ] ABI compatibility verification matrix

## Known Issues & Limitations
- Runtime behavior and model support depend on linked llama backend.

## Breaking Changes
- None currently planned.