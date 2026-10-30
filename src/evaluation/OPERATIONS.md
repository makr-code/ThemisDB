---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Evaluation Module Operations

AI provenance: Exact model backend not verifiable from final document text alone.

## Operational model

The module is consumed as C++ libraries and contracts; it is not an independently
deployed process. The consuming service owns request logging, service health,
alerts, and configuration. Evaluation module outputs are returned as
`PlannerDecision`, metric value types or `MetricError`, validation-result
structures, lifecycle states, and benchmark records. Export these values through
the host application's existing observability system without discarding fallback
or error details.

## Routine validation

For a code change or release candidate:

1. Configure the repository with the intended preset and required dependencies.
   Record the preset and configuration; a failed configure means no focused
   executable evidence was produced.
2. Check the available targets with the build system. Evaluation tests are
   registered conditionally in `tests/evaluation/CMakeLists.txt` and
   `tests/epic2_evaluation/CMakeLists.txt`; missing prerequisites can omit
   targets.
3. Build and run the focused tests for the changed component. For the retrieval
   metrics example, use the commands in [RUNBOOK.md](./RUNBOOK.md).
4. Run relevant benchmarks separately from correctness tests. Preserve complete
   output, compiler/build mode, hardware, dataset/query-set version, and
   configuration.
5. Update `MODULE_EVIDENCE.md` with the exact executed command, result, and
   blockers. Keep source audit, test execution, and measured benchmark claims
   distinct.

## Promotion and rollback gates

Use `ROADMAP.md`, `PRODUCTION_REQUIREMENTS.md`, and `PHASE_6_ACCEPTANCE_CHECKLIST.md`
as the authoritative module gate/evidence record. Do not enable a default
workflow or promote a performance claim solely because a target is present or
because a guardrail is defined. If required executable evidence is unavailable,
record the environment gap and leave the promotion gate unresolved.

For an unexpected runtime decision, preserve the complete planner inputs and
result. The caller can request `force_exact` or `force_cpu` through
`ExecutionEligibility`; use exact execution when correctness is uncertain.
Rollback means disabling the affected caller integration or policy rollout,
not deleting benchmark data or rewriting historical evidence. Re-enable only
after the cause is fixed and focused evidence is captured.

## Evidence retention and operator signals

- Retain the `FallbackReason`, selected path, policy version, and relevant
  freshness/eligibility inputs for planner incidents.
- Retain `MetricErrorKind` and the input dataset/query-set version for metric
  failures; avoid logging sensitive query content or answer payloads unless the
  host application's data policy permits it.
- Retain lifecycle state, invalidation reason, and rebuild outcome for artifact
  incidents.
- Retain benchmark output and the run environment so comparisons can be
  reproduced.

No module-owned metric names, alert thresholds, service endpoints, or
configuration keys are specified by these APIs. Define those at the consuming
service boundary and document them there.

## References

- [Runbook](./RUNBOOK.md)
- [Production requirements](./PRODUCTION_REQUIREMENTS.md)
- [Acceptance checklist](./PHASE_6_ACCEPTANCE_CHECKLIST.md)
- [Evidence record](./MODULE_EVIDENCE.md)
- [Performance guardrails](./PERFORMANCE_EXPECTATIONS.md)
- `src/evaluation/CMakeLists.txt`
- `tests/evaluation/CMakeLists.txt`
- `tests/epic2_evaluation/CMakeLists.txt`
