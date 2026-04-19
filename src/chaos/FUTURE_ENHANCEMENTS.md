# chaos

## Scope
- Distributed chaos event broadcast and replay for multi-node testbeds.

### Design Constraints
- Keep deterministic scheduling semantics for CI reproducibility.
- Preserve backward-compatible `FaultSpec` fields.

### Required Interfaces
- `FaultInjector::injectFault`, `recoverFault`, `isFaultActive`
- `ChaosScheduler::schedule`, `scheduleIn`, `start`, `stop`

### Implementation Notes
- Add optional pluggable event sink for external orchestration.
- Keep default behavior in-process for unit tests.

### Test Strategy
- Unit tests for expiry and callback invocation order.
- Integration tests for delayed and simultaneous injections.

### Performance Targets
- Injection/recovery operations should remain O(1) average map operations.
- Scheduler wake latency target <= 25 ms under normal CI load.

### Security / Reliability
- Validate all user-provided fault parameters.
- Ensure callbacks cannot deadlock injector state.