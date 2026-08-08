# Blocks 2-4 Implementation Plan
## Updates Module Hardening & GA Readiness

**Status**: In Progress  
**Block 1 Confirmation**: Complete (commits 7cad1f18c6, 0a84ed447c)  
**Start Date**: 2026-08-08

## Block 2: Phase 2-3 Hardening (2-3 weeks)

### Subtasks
- [~] Phase 2 deliverables (state machine, delta engine, migration, rollout)
- [ ] Phase 3 standardization (fail-safe, unified diagnostics, 15-20 new tests)
- [ ] All hardening tests passing

### Phase 2 Implementation
1. State machine edge cases (invalid, concurrent, partial corruption)
2. Delta engine determinism & rollback capability
3. Migration timeout & fallback handling
4. Rollout scheduler deterministic bounds

### Phase 3 Implementation
1. Unified error taxonomy (4 classes: StateTransition, PatchApply, Migration, Rollout)
2. Standardized fail-safe behavior across all surfaces
3. 15-20 focused edge-case tests (UPH-01..20)

## Block 3: Benchmark Depth (1-2 weeks)

### New Suites (4 total)
1. `bench_updates_coordinated_hardening.cpp` - Multi-node scenarios
2. `bench_updates_canary_resilience.cpp` - Canary failure injection
3. `bench_updates_schema_diversity.cpp` - Large dataset migrations
4. `bench_updates_long_run_stability.cpp` - 48h+ workload

### PERFORMANCE_EXPECTATIONS.md Updates
- Add gates UPDP-4..7 with mapped metrics

## Block 4: Documentation & Release Readiness (1 week)

### Deliverables
1. Close AUDIT.md findings (UPD-AUD-01, 02, 03)
2. Create MODULE_EVIDENCE.md with build/test evidence
3. Update CHANGELOG.md with v1.1.0 section
4. Create PRODUCTION_REQUIREMENTS.md checklist
5. Update ROADMAP.md marking phases complete

### Evidence Required
- Build: No errors, warnings minimized
- Tests: 50+ test cases all passing
- Benchmarks: UPDP-1..7 all gates PASS
- Code review: Signed off
- Security: Reviewed

## Timeline
- Phase 2 hardening: Week 1-2
- Phase 3 standardization & tests: Week 2-3
- Benchmarks: Week 3-4
- Documentation: Week 4
- Total: ~4 weeks execution

## Success Criteria
✓ Zero CRITICAL findings (Block 1 baseline)
✓ 20 new hardening tests all passing
✓ 4 benchmarks with gates UPDP-4..7 passing
✓ AUDIT.md fully closed
✓ All docs synchronized
✓ release_critical CI gate: 100%
✓ Doxygen coverage: >95%
✓ Ready for merge to develop
