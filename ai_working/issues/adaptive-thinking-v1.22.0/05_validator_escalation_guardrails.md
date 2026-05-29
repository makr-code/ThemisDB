## Summary

Implement lightweight answer validation and escalation guardrails for adaptive inference.

## Deliverables

- Validator checks for empty/truncated/malformed outputs
- Escalation reason codes
- Guardrails to prevent runaway deep-pass loops

## Tasks

- [ ] Implement validator checks
- [ ] Emit reason codes for escalation/fallback
- [ ] Add max-attempt guard and loop prevention
- [ ] Add tests for invalid and edge outputs

## Acceptance Criteria

- Validator catches defined invalid-output classes
- Escalation reasons are observable in logs/metrics
- No infinite or repeated escalation loops

## Labels

- type:feature
- area:llm
- area:api
- priority:P1
- effort:medium
