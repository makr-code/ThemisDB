## failover

### Scope
- Deep multi-region policy orchestration and operator runbook automation.

### Design Constraints
- Keep failover state machine deterministic and observable.
- Preserve existing API signatures for manager constructors and control methods.

### Required Interfaces
- `AutoFailoverManager::start/stop/triggerManualFailover`
- `DisasterRecoveryManager::executePlan/validatePlan`

### Implementation Notes
- Add pluggable policy evaluator for node promotion selection.
- Add richer metrics export for per-step timings and retries.

### Test Strategy
- Integration tests with simulated quorum loss and recovery.
- Failure-injection tests across each DR step hook.

### Performance Targets
- Manual failover enqueue path should remain sub-millisecond under normal load.
- DR step orchestration overhead target <= 5% of total recovery duration.

### Security / Reliability
- Enforce split-brain safeguards via epoch fencing where configured.
- Validate plan inputs before mutating cluster state.