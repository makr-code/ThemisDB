## Summary

Create a full test matrix for adaptive thinking behavior: unit, integration, and performance.

## Deliverables

- Unit test suites for scorer/policy/validator
- Integration tests for off/shadow/active modes
- Benchmark harness comparing baseline vs adaptive

## Tasks

- [ ] Add unit tests for deterministic scoring/policy mapping
- [ ] Add integration tests for endpoint behavior and fallback
- [ ] Add benchmark scenarios for S0..S3 request classes
- [ ] Document pass/fail thresholds for release gate

## Acceptance Criteria

- All new adaptive logic covered by tests
- Performance report includes p50/p95 and deep-pass ratio
- Release gate thresholds defined and reproducible

## Labels

- type:test
- area:llm
- area:performance
- priority:P1
- effort:large
