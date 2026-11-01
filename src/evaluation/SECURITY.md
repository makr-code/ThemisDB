# Evaluation Module Security

> **Update provenance (2026-10-31):** AI_Used: true; AI_Models: unknown;
> AI_Contributor: OpenAI ChatGPT; Human_Review: false; Human_Author: ThemisDB
> Contributors. Exact model backend not verifiable from final document text alone.

<!-- Source level: level1 · SOT domain: security -->
<!-- Last Updated: 2026-10-31 · Issue: #7344 · Milestone: DOC-WEEKLY-2026-44 -->

## Scope and assurance boundary

This page describes security-relevant boundaries visible in the evaluation
module interfaces and implementations. It is not a security audit, production
approval, or proof that a caller enforces the returned decisions.

The module is a C++ library, not an independently authenticated service. Its
metrics, labels, hardware signals, policy values, artifact metadata, benchmark
values and manifest-availability inputs are supplied by callers. The APIs do
not authenticate users, authorize database access, establish ground truth,
verify benchmark provenance or protect sensitive inputs in the consuming
application.

## Relevant code controls and limits

- `QueryPlanner` returns a typed path and fallback reason based on caller
  eligibility, freshness and configuration. It does not execute the selected
  path or verify the result.
- `ApproximationRuleEngine` returns boundary decisions. The caller must enforce
  them; policy evaluation is not a substitute for authorization.
- `retrieval_metrics` raises `MetricError` for defined invalid-input cases, but
  cannot establish that supplied labels or measurements are truthful or
  representative.
- `ArtifactLifecycleManager` computes states and returns metadata transitions;
  it does not persist artifacts or run rebuild jobs.
- The matrix and metric APIs are not evidence of a trusted measurement process.

## Risks and maintainer actions

| Risk | Required handling |
|---|---|
| Caller ignores a fallback, policy result or metric error | Preserve typed outcomes at the integration boundary and test enforcement there |
| Untrusted or biased labels/metadata | Version and validate the input dataset; do not treat a successful calculation as truth validation |
| Stale or missing artifact/manifest state | Keep the caller's exact-path/recovery behavior explicit and verify the full consumer path |
| Benchmark value presented without provenance | Require retained output and environment details before claiming a measured baseline |
| Sensitive queries or answers exposed in logs | Follow the consuming service's data-handling rules; the module defines no logging policy |

No current security acceptance or end-to-end integration evidence is established
by this documentation review. See [TRUST.md](./TRUST.md),
[PRODUCTION_REQUIREMENTS.md](./PRODUCTION_REQUIREMENTS.md), and
[MODULE_EVIDENCE.md](./MODULE_EVIDENCE.md).

## Canonical references

- `src/evaluation/include/query_planner.h`
- `src/evaluation/src/query_planner.cc`
- `src/evaluation/include/approximation_rules.h`
- `src/evaluation/src/approximation_rules.cc`
- `src/evaluation/include/retrieval_metrics.h`
- `src/evaluation/src/retrieval_metrics.cc`
