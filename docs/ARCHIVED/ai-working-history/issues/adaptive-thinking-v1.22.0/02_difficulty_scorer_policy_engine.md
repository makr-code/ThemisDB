## Summary

Implement deterministic request difficulty scoring and policy mapping to inference profiles.

## Deliverables

- Difficulty scorer (S0..S3 + reason codes)
- Policy engine mapping `{endpoint, complexity, SLA}` to profile
- Safe defaults and strict enum-based telemetry dimensions

## Tasks

- [ ] Implement scorer input feature extraction
- [ ] Implement class assignment and confidence output
- [ ] Implement policy table and profile lookup
- [ ] Add unit tests for mapping and corner cases

## Acceptance Criteria

- Deterministic output for equivalent inputs
- Full unit coverage for policy table variants
- Unknown inputs safely map to fallback profile

## Labels

- type:feature
- area:llm
- area:server
- priority:P1
- effort:large
