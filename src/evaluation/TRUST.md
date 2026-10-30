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

# Evaluation Trust Model

AI provenance: Exact model backend not verifiable from final document text alone.

## Trust boundary

The evaluation APIs consume data and policy inputs from their callers and return
metrics or decisions. They do not authenticate callers, enforce database
authorization, fetch secrets, establish ground truth, or prove that a retrieved
answer is factually correct. The host service remains responsible for access
control, sensitive-data handling, and safe exposure of results.

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
- Category C work (policy/ACL, provenance, and transactions) is exact and CPU
  only under the documented planner and approximation contracts. Unsafe
  approximation requests are denied or escalated rather than silently
  permitted.
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
- [API contracts](./API.md)
- [Runbook](./RUNBOOK.md)
- [Production requirements](./PRODUCTION_REQUIREMENTS.md)
- `src/evaluation/include/query_planner.h`
- `src/evaluation/include/approximation_rules.h`
- `src/evaluation/include/retrieval_metrics.h`
- `src/evaluation/src/query_planner.cc`
- `src/evaluation/src/retrieval_metrics.cc`
