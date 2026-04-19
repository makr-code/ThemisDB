# include/failover

## Scope
- API extensibility for advanced failover policies and DR automation.

### Design Constraints
- Keep constructor and lifecycle signatures stable.

### Required Interfaces
- `AutoFailoverManager`
- `DisasterRecoveryManager`

### Implementation Notes
- Prefer additive config fields and optional hooks.

### Test Strategy
- Compile-time and ABI compatibility checks.

### Performance Targets
- Keep API-level abstractions zero-cost where feasible.

### Security / Reliability
- Enforce clear states for failed/aborted workflows.