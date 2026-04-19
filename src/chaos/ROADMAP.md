> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# chaos roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
- [x] Fault injection registry and scheduler implemented (Target: Q2 2026)
- [x] Expiry handling and callback hooks implemented (Target: Q2 2026)

## In Progress
- [ ] Cluster-wide distributed chaos coordination (Target: Q3 2026)

## Implementation Phases
### Phase 1: Design / API Contract
- [x] Define `FaultSpec` and `FaultType` API (Target: Q2 2026)
### Phase 2: Core Implementation
- [x] Implement `FaultInjector` and `ChaosScheduler` (Target: Q2 2026)
### Phase 3: Error Handling & Edge Cases
- [x] Validate node id and probability bounds in `injectFault` (Target: Q2 2026)
### Phase 4: Tests
- [x] Add module-level dedicated stress tests for scheduler jitter (Target: Q3 2026)
### Phase 5: Performance/Hardening
- [x] Add configurable scheduler tick and wake strategy (Target: Q3 2026)
### Phase 6: Documentation & Acceptance
- [x] Baseline module documentation created (Target: Q2 2026)

## Production Readiness Checklist
- [x] Public headers and implementation aligned
- [x] No TODO/stub markers in core implementation
- [x] Extended stress benchmark coverage

## Known Issues & Limitations
- In-process simulation only; no direct process/network sabotage.

## Breaking Changes
- None currently planned.