# EPIC 2.4 Approximation Governance

<!-- Status: current | planning scaffold | validated: 2026-06-01 -->

## Summary

Approximation boundaries and governance rules for layered retrieval.

## Scope

- Approximate versus exact zones per retrieval stage
- Fail-open versus fail-closed decisions with policy ownership
- Planner enforcement and observability requirements

## Planned Repository Surfaces

- `src/evaluation/include/approximation_rules.h`
- `src/evaluation/src/approximation_rules.cc`
- `tests/epic2_evaluation/approximation_rules_test.cc`

## Seven-Phase Roadmap

### Phase 1: Design / API contract
- [ ] Define governance vocabulary before runtime enforcement is added
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.4 Approximation Governance`.

### Phase 2: Core implementation
- [ ] Document who can override approximation behavior and under what evidence
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.4 Approximation Governance`.

### Phase 3: Error handling and edge cases
- [ ] Enumerate policy edge cases for regulated or provenance-critical queries
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.4 Approximation Governance`.

### Phase 4: Tests
- [ ] Reserve tests for policy decisions and fallback paths
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.4 Approximation Governance`.

### Phase 5: Performance and hardening
- [ ] Keep governance aligned with graph validation and integrity requirements
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.4 Approximation Governance`.

### Phase 6: Documentation and acceptance
- [ ] Keep governance aligned with graph validation and integrity requirements
- [ ] Prepare the next phase only after the current contract is reviewed for `EPIC 2.4 Approximation Governance`.

### Phase 7: Integration
- [ ] Keep governance aligned with graph validation and integrity requirements
- [ ] Wire the planned files into the nearest CMake and cross-epic integration checkpoints for `EPIC 2.4 Approximation Governance`.

## Acceptance Signals

- The planned repository surfaces remain stable enough for issue creation and ownership.
- The document names the dependencies, failure modes, and validation hooks needed before code lands.
- Tests and benchmarks have reserved file names before implementation starts.

## References

- `docs/EPIC1_GRAPH_VALIDATION.md`
- `docs/EPIC2_QUERY_PLANNER.md`
- `docs/EPIC3_INTEGRITY_MODEL.md`
