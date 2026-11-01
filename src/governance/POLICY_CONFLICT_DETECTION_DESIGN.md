---
Author: "ThemisDB Contributors"
Created: "2026-08-18"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Related: "Issue #7349"
---

Exact model backend not verifiable from final document text alone.

# Policy Conflict Detection: Current API and Design Caveats

This design note is not a specification for `PolicyEngine` decision precedence.
The conflict detector is a separate utility declared in
[`policy_conflict_detector.h`](../../include/governance/policy_conflict_detector.h)
and implemented in `policy_conflict_detector.cpp`.

## Current declared API

`PolicyConflictDetector` declares detection methods for all conflicts,
permit/deny conflicts, overlapping conflicts, and circular dependencies. It also
declares rule/all-rule precedence analysis, atomic add/update/remove methods,
single-pair conflict checking, and optional cached results. Types include
`PolicyConflict`, `PrecedenceEvaluation`, `AtomicUpdateResult`, `ConflictType`,
and `ConflictSeverity`.

The current implementation performs pairwise rule comparisons in at least the
permit/deny and overlap paths. The overlap matcher is a simple wildcard-prefix
check, not a general policy-language matcher. Its conflict cache is keyed by the
`PolicyManager` object's address; callers must not assume automatic invalidation
on every external policy mutation. Review the implementation before depending
on cache freshness or atomicity guarantees.

## Claims withdrawn from the earlier design

The previous version asserted a precise deny-overrides formula, creation-time
tie-break algorithm, cross-operation transaction guarantees, cache behavior,
thread-safety guarantees, conflict accuracy, latency measurements, and
benchmark/test gates. Those statements are not established by the public
declarations or current focused test registration and must not be used as
current behavior claims. The current detector API and implementation are not
evidence that `PolicyEngine` applies the same precedence rules.

No focused conflict-detector test or dedicated benchmark is registered by
`tests/governance/CMakeLists.txt` or `benchmarks/governance/CMakeLists.txt`.
Some broader test files exercise adjacent governance behavior, but no coverage
or performance result is claimed here.

## References

- [Conflict detector API](../../include/governance/policy_conflict_detector.h)
- `policy_conflict_detector.cpp`
- [Policy engine API](../../include/governance/policy_engine.h)
- [Governance test registration](../../tests/governance/CMakeLists.txt)
- [Benchmark registration](../../benchmarks/governance/CMakeLists.txt)
