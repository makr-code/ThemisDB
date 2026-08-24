## Summary

Implement fast/deep pass orchestration with robust fallback to current static inference behavior.

## Deliverables

- Fast pass execution path
- Escalation to deep pass by policy/validator
- Queue-pressure and timeout guards
- Fallback path preserved for all failures

## Tasks

- [ ] Integrate scorer/policy into inference orchestration
- [ ] Implement deep-pass trigger matrix
- [ ] Add queue-depth suppression for deep path
- [ ] Add deterministic fallback guardrails
- [ ] Add integration tests for mode transitions

## Acceptance Criteria

- No request failure due to adaptive layer failure
- Escalation behavior matches policy matrix
- Fallback path functionally equivalent to pre-adaptive behavior

## Labels

- type:feature
- area:llm
- area:server
- priority:P1
- effort:large
