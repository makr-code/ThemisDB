> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# failover roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
- [x] Automatic failover orchestration implemented (Target: Q2 2026)
- [x] Disaster recovery plan execution pipeline implemented (Target: Q2 2026)
- [x] Queue-pressure and retry telemetry thresholds added (Target: Q3 2026)
- [x] Chaos + failover end-to-end scenario matrix added (Target: Q3 2026)

## In Progress
- [ ] Cross-region traffic manager integration (Target: Q3 2026)

## Implementation Phases
### Phase 1: Design / API Contract
- [x] Define failover and DR state/step enums and result contracts (Target: Q2 2026)
### Phase 2: Core Implementation
- [x] Implement auto failover queue + worker loops (Target: Q2 2026)
- [x] Implement DR step execution and validation flow (Target: Q2 2026)
### Phase 3: Error Handling & Edge Cases
- [x] Handle missing managers and dry-run behavior (Target: Q2 2026)
### Phase 4: Tests
- [x] Add focused unit test suite for AutoFailoverManager — 39 tests covering lifecycle, state machine, manual failover, queue pressure, config management, statistics, event callbacks, failure tracking, last-result, and edge cases (Target: Q2 2026)
- [x] Add chaos + failover end-to-end scenario matrix (Target: Q3 2026)
### Phase 5: Performance/Hardening
- [x] Introduce queue-pressure and retry telemetry thresholds (Target: Q3 2026)
### Phase 6: Documentation & Acceptance
- [x] Baseline module documentation created (Target: Q2 2026)

## Production Readiness Checklist
- [x] No stub/todo markers in core source files
- [x] Orchestrator lifecycle and state transitions implemented
- [x] Queue-pressure telemetry (current_queue_depth, max_queue_depth_observed, tasks_dropped_queue_full, queue_pressure_events)
- [x] Retry telemetry (total_retry_attempts, successful_retries, failed_retries)
- [x] QUEUE_PRESSURE event emitted when queue fill ratio >= queue_pressure_threshold
- [ ] Extended multi-region failover soak tests

## Known Issues & Limitations
- Some integration outcomes depend on external manager availability.

## Breaking Changes
- None currently planned.