# Wave A-8 Implementation Plan

## Objective
Close GPU/CUDA kernel gaps and voice adversarial hardening gaps for production readiness.

## Scope

### GPU Module (`src/gpu/`)
- [ ] Audit CUDA kernel stubs and identify remaining gaps
- [ ] Implement fail-closed fallback to CPU when GPU unavailable
- [ ] Enforce kernel timeouts and RAII lifecycle safety
- [ ] Add comprehensive error handling for GPU memory and CUDA API calls
- [ ] Add chaos/fault-injection tests for GPU failures
- [ ] Measure p95/p99 performance baselines

### Voice Module (`src/voice/`)
- [ ] Harden session lifecycle to fail-closed (22 listed items)
- [ ] Implement robust malformed/oversized stream rejection
- [ ] Add adversarial anti-spoof liveness hardening
- [ ] Ensure safe multi-session teardown without leaks
- [ ] Add production-strength error handling
- [ ] Add chaos tests for session isolation and streaming teardown

## Implementation Strategy

1. **Discover** current implementation status
   - Count remaining CUDA stubs
   - Audit voice session lifecycle implementation
   - Identify unchecked CUDA calls

2. **Plan** production implementations
   - Design RAII-safe GPU kernel wrappers
   - Design fail-closed session lifecycle
   - Document error handling taxonomy

3. **Implement** production-ready code
   - Implement missing error handlers
   - Add timeout enforcement
   - Add session isolation hardening

4. **Test** comprehensive coverage
   - Add chaos/fault-injection tests
   - Add regression tests for edge cases
   - Verify fail-closed behavior

5. **Measure** performance baselines
   - Capture p95/p99 latencies
   - Document GPU/CPU break-even points
   - Record session throughput

6. **Document** closure evidence
   - Update GPU ROADMAP.md with Wave A-8 evidence
   - Update Voice ROADMAP.md with Wave A-8 evidence
   - Record test results and baselines

## Timeline
- Phase 1 (Discovery): 30 mins
- Phase 2 (Planning): 30 mins
- Phase 3 (Implementation): 2 hours
- Phase 4 (Testing): 1 hour
- Phase 5 (Measurement): 30 mins
- Phase 6 (Documentation): 30 mins

Total: ~5 hours
