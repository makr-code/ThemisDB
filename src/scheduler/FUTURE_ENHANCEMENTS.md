---
Author: ThemisDB Team
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
Source level: 1
SOT domain: module-behavior
Canonical references: include/scheduler/*.h; src/scheduler/*.cpp; tests/scheduler/*.cpp; benchmarks/scheduler/*.cpp
Related issue: #7376; DOC-WEEKLY-2026-44
---

Exact model backend not verifiable from final document text alone.

# Scheduler future enhancements

The items below are proposals and review needs, not committed delivery dates or
claims about current implementation.

## Reliability and contract alignment

- Implement and exercise the coordinator-availability check currently left as
  a placeholder in `TaskScheduler::executeTaskNow`.
- Reconcile the event-condition parser's documented failure policy with its
  current structural rejection and unknown-operator match behavior.
- Replace skipped/simulated coordination and stress scenarios with tests of the
  actual scheduler components.
- Confirm request-context behavior and API authorization expectations with the
  server/security owners.

## Performance and operability

- Establish an owner, metric/statistic, CI workflow, thresholds, and
  representative-hardware baseline before describing a benchmark as a release
  gate.
- Add direct production-component benchmark coverage for integration paths only
  where a measurable use case exists.
- Keep operational requirements and security policy in the corresponding
  reviewed canonical documents rather than inferring them from configuration
  fields.

See [ROADMAP.md](ROADMAP.md), [AUDIT.md](AUDIT.md), and
[PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md) for verified gaps and
current benchmark coverage.
