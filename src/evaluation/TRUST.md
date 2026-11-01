---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot; OpenAI ChatGPT/themisdb-doc-orchestrator"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Evaluation Trust Model

AI provenance: Exact model backend not verifiable from final document text alone.

## Trust boundary

The evaluation APIs consume data and policy inputs from their callers and return
metrics or decisions. They do not authenticate callers, enforce database
authorization, fetch secrets, establish ground truth, or prove that a retrieved
answer is factually correct. The host service remains responsible for access
control, sensitive-data handling, and safe exposure of results.

These are component-level contracts, not proof that a downstream consumer uses
or enforces them. The public-path `include/evaluation/evaluation.h` currently
includes no component headers; the declarations are under
`src/evaluation/include/`.

Treat ranked IDs, relevance labels, benchmark values, artifact metadata,
hardware signals, shard-manifest availability, and configuration as
caller-supplied evidence. The metric layer can reject malformed data; it cannot
prove that supplied labels or metadata are honest or representative.

## Correctness and policy controls

- `computeRetrievalQuality()` rejects empty ground truth, invalid `k`, and
  duplicate identifiers. Other metric functions validate numeric and
  domain-specific ranges and report failures through `MetricError`.
- The query planner returns typed paths and fallback reasons. A planner decision
  is not an execution result and must not be used as proof that downstream
  retrieval succeeded.
- Tensor summaries are advisory. A truth-bearing answer must retain exact graph
  validation or the specified exact-on-demand path.
- The approximation contract describes exactness requirements for Category C
  work and the planner's GPU eligibility rules disallow Category C GPU
  execution. This does not establish that an external consumer applies the
  checks; callers must preserve and enforce the decision.
- The benchmark matrix stores supplied result values; it does not authenticate
  measurement provenance or establish that a run was reproducible.

## Risks and required handling

| Risk | Control and operator response |
|---|---|
| Incorrect or biased relevance labels | Version the query set and judgments; compare only matching evaluation inputs and disclose label limitations |
| Invalid numeric values or duplicate IDs | Preserve `MetricErrorKind`; correct the source data rather than substituting a pass-shaped value |
| Stale or mismatched tensor artifact | Retain freshness and source-sequence metadata; use exact fallback and investigate lifecycle state |
| Missing distributed manifest or weak summary confidence | Keep typed fallback outcomes; do not accept a summary-only result as truth |
| Caller ignores fallback/error or misreports metrics | Preserve typed outcomes in host-service logs and gate decisions; audit the integration contract |
| Benchmark data presented as a measured baseline without a run | Require raw output and environment details in `MODULE_EVIDENCE.md` before promotion |

## Incident handling

For suspected policy bypass, incorrect truth-bearing output, or an unrecovered
summary-first false negative, stop promotion of the affected route, preserve
decision inputs and fallback/error details, and use the exact path where
available. Notify the module owner and release/security reviewer. Do not log
secrets or unrestricted user query/answer payloads as incident evidence. Record
the test and benchmark evidence before restoring the route.

## References

- [Security posture](./SECURITY.md)
- [Current roadmap/evidence gap](./ROADMAP.md)
- [API contracts](./API.md)
- [Runbook](./RUNBOOK.md)
- [Production requirements](./PRODUCTION_REQUIREMENTS.md)
- `src/evaluation/include/query_planner.h`
- `src/evaluation/include/approximation_rules.h`
- `src/evaluation/include/retrieval_metrics.h`
- `src/evaluation/src/query_planner.cc`
- `src/evaluation/src/retrieval_metrics.cc`
