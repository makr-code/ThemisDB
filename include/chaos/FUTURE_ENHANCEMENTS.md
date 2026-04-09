## include/chaos

### Scope
- Future API additions for distributed chaos orchestration.

### Design Constraints
- Maintain backward-compatible enum/value semantics.

### Required Interfaces
- `FaultInjector`
- `ChaosScheduler`

### Implementation Notes
- Introduce optional extension structs instead of breaking field changes.

### Test Strategy
- Header-level compile tests and API compatibility checks.

### Performance Targets
- No additional overhead for default in-process use.

### Security / Reliability
- Preserve strict parameter validation guarantees.