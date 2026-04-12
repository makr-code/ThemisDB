# include failover roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
- [x] Header API complete for implemented failover/DR managers (Target: Q2 2026)

## In Progress
- [ ] Add external-policy extension points without ABI breaks (Target: Q3 2026)

## Implementation Phases
### Phase 1: Design / API Contract
- [x] Define orchestration and DR contracts (Target: Q2 2026)
### Phase 2: Core Implementation
- [x] Source implementations present in `src/failover` (Target: Q2 2026)
### Phase 3: Error Handling & Edge Cases
- [x] Explicit state and step result types exposed (Target: Q2 2026)
### Phase 4: Tests
- [ ] Add include-boundary API compatibility tests (Target: Q3 2026)
### Phase 5: Performance/Hardening
- [ ] Evaluate lock-free telemetry additions without API churn (Target: Q3 2026)
### Phase 6: Documentation & Acceptance
- [x] Baseline include module docs created (Target: Q2 2026)

## Production Readiness Checklist
- [x] Public headers map to concrete implementation
- [ ] ABI compatibility checks across compilers

## Known Issues & Limitations
- Behavior depends on runtime manager integrations supplied by host.

## Breaking Changes
- None currently planned.