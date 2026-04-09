# failover security

## Threat Model
- Incorrect failover decisions may cause split-brain or prolonged outage.
- Invalid DR plans may trigger incomplete recovery operations.

## Controls
- Quorum checks and optional split-brain prevention gates.
- Input validation in `DisasterRecoveryManager::validatePlan`.
- Explicit state transitions and fail/abort tracking.

## Remaining Risks
- Runtime behavior depends on correctness of external replication/fencing managers.