## Summary

Define and finalize the Adaptive Thinking design spec, config schema, and runtime control contract.

## Deliverables

- Config schema for `adaptive_thinking.*`
- Policy profile format and validation rules
- API/admin visibility contract for profile decisions
- Failure/fallback behavior specification

## Tasks

- [ ] Define config fields and defaults
- [ ] Define policy profile schema and parser behavior
- [ ] Specify shadow/active/off mode semantics
- [ ] Document error handling and fallback path
- [ ] Publish design doc references

## Acceptance Criteria

- Schema and semantics documented and approved
- Invalid config/profile handling deterministic and tested
- No ambiguity for off/shadow/active behavior

## Labels

- type:feature
- area:llm
- area:server
- priority:P1
- effort:medium
