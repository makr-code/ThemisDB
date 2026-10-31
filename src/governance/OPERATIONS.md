---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
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

# Governance Operations

## Purpose and operating boundary

This page covers routine deployment, monitoring, policy rollout, and evidence retention for governance operators. It complements [RUNBOOK.md](RUNBOOK.md), which handles incidents. Governance evaluates and reports controls; it does not own caller authentication, the external OPA service, durable audit storage, or the downstream data/model operation.

## Before enabling a deployment

- Load the intended policy file and verify `PolicyEngine::loadFromYAML()` succeeds. Review profile, route mapping, mode, and masking configuration from the deployed artifact.
- Confirm each production request path supplies trusted classification/identity context and consumes export/cache/ANN/masking decisions.
- Configure audit and lineage sinks where evidence forwarding is required. Understand that lineage is bounded in-memory state, not durable storage.
- Verify OPA endpoint, policy path, timeout, and response contract. Confirm the
  active evaluator mode. WASM mode returns a strict deny without an injected
  evaluator and never substitutes REST/native evaluation.
- Verify masking secrets are non-placeholder and securely supplied. Empty tokenization secrets result in unkeyed SHA-256 fallback.
- Confirm downstream handlers can safely fail closed when a decision is missing or denied.

## Policy rollout and rollback

1. Review and validate the policy in the normal policy lifecycle; preserve its author, version, rationale, and audit trail.
2. Stage the YAML or managed rule update and test representative allowed, denied, unknown-classification, and opt-out cases.
3. Deploy through the controlled configuration path. `loadFromYAML()` builds replacement state before swapping it under lock; if it returns false, keep the prior active configuration and investigate the parse/load error.
4. After reload, check route-to-classification mapping and a representative query permission result. Inspect the available decision, diagnostics, and audit fields; do not assume every event identifies the evaluator or its mode.
5. If behavior regresses, use the versioned manager's safety check and preview before rollback; record the reason and verify the restored decisions.

Do not edit policy files directly during an incident in a way that bypasses review/version history. Emergency procedures and their authority are defined by the project incident process.

## Monitoring signals

| Signal | Operational meaning | Response |
|---|---|---|
| `governance_opa_fallback_total` | An evaluator returned no decision and `PolicyEngine` used native evaluation (for example, REST `nullopt`). | Check endpoint, policy path, timeout, and response format; verify the native result is acceptable. Explicit WASM failures return a strict deny instead of incrementing this native-fallback path. |
| `governance_lineage_events_total` | Locally recorded lineage event volume by event type. | Compare with expected ingest/export activity and configured retention bounds. |
| `governance_fields_masked_total` | Masked string-field count by strategy. | Compare with the expected output path and policy; absence can mean no matching fields or an unapplied/disabled policy. |
| OPA diagnostics and error counters | Timeout, connection, HTTP, or response-parse failures. | Use the diagnostic component/context to distinguish service failure from invalid policy output. |
| `DataLineageTracker::getStatistics()` | Event/dataset counts, breaker state, and latest error code. | Investigate an open circuit, retention eviction, or stale evidence before relying on audit completeness. |
| Policy audit/version history | Decision and change trace where configured. | Preserve records for change review and incident reconstruction. |

Metric absence is not proof of healthy behavior: some paths emit a counter only when a configured policy actually matches or a failure occurs. Pair metrics with representative end-to-end checks and audit evidence.

## Capacity and retention

Lineage has configurable per-dataset and global event limits. At a limit, the implementation retains recent events by FIFO eviction and emits a size-limit diagnostic. Choose limits based on evidence-retention requirements and memory budget; if an event has been evicted, the in-memory tracker cannot reconstruct it. Durable retention and backup are responsibilities of the configured audit/evidence store.

Use [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md) for benchmark case mappings and the source-commented target. It does not publish a current measured baseline or confirmed release budget. Record workload, build profile, hardware, effective YAML, and comparison baseline before interpreting p95/p99.

## Escalation and recovery

For unresolved evaluator failure, policy drift, audit loss, incomplete deletion, or a masking gap, stop the affected sensitive operation, preserve the policy version and evidence, then follow [RUNBOOK.md](RUNBOOK.md). Escalate authorization-boundary issues through [TRUST.md](TRUST.md) and the private route in [SECURITY.md](../../SECURITY.md). Do not declare regulatory compliance based only on module reports.

## Source references

- [Production requirements](PRODUCTION_REQUIREMENTS.md)
- [Operational audit guide](OPERATIONAL_AUDIT_GUIDE.md)
- [Policy versioning guide](POLICY_VERSIONING_GUIDE.md)
- [Performance expectations](PERFORMANCE_EXPECTATIONS.md)
