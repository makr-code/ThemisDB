---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# AQL Assistance Incident Runbook

## Scope and safety

Use this runbook for incidents in `src/aql/` translation, parser-validation,
conversation-context, and embedding/few-shot assistance paths. The assistance
feature is distinct from direct AQL submission. Do not turn off validation,
execute partial streamed output, or treat a generated query as authorized in
order to restore assistance availability.

Follow deployment-specific access and change-control procedures when
inspecting configuration or restarting services. Do not use an unverified
admin command or weaken validation to restore availability. The
cross-scope [`docs/operability/RUNBOOK_AQL_ASSISTANCE.md`](../../docs/operability/RUNBOOK_AQL_ASSISTANCE.md)
contains operational recommendations that require reconciliation with the
current module contract; this page does not adopt those commands or its
`WARN_ONLY` recommendation as production guidance.

## First response

1. Confirm whether direct AQL requests are affected; separate assistance
   failures from core query execution incidents.
2. Record the time window, request/trace identifier, affected operation, and
   sanitized error category. Do not copy prompts, credentials, or query data
   containing sensitive values into tickets or logs.
3. Inspect server logs for `[TRANSLATION:*]`, `[VALIDATION:*]`,
   `[BRIDGE:*]`, and `[TRANSLATION:Confidence]` events.
4. Check provider readiness, parser-service wiring, and the handler's
   `getCircuitBreakerStates()` output where the deployment exposes it.
5. Compare failure/retry counts and latency with the established AQL
   dashboard or baseline; do not infer an outage from a single rejected query.

## Triage by symptom

| Symptom | Check | Safe response |
|---|---|---|
| Translation errors or retry exhaustion | Provider health, timeout and retry configuration, parser diagnostics, and error category | Restore the provider or correct the request/schema context; keep validation enabled |
| Circuit breaker open | Provider availability and repeated failure pattern; inspect the relevant infer/RAG/embed/finetune state | Resolve the underlying dependency issue and allow configured recovery; do not repeatedly restart to bypass the breaker |
| Candidate rejected by parser | Parser diagnostics and whether the schema context matches the request | Return a structured failure or request a simpler query; do not pass an invalid candidate to execution |
| Embedding bridge reports `[BRIDGE:ExecutionFailed]` | Embedding provider readiness and handler lifetime | Confirm lexical fallback is active; restore embeddings if semantic ranking is required |
| Context eviction or bound reached | `max_turns`, `max_history_tokens`, token estimates, and request size | Treat expected eviction as bounded operation; reduce context or start a fresh conversation |
| Low confidence | Returned score, explanation, suggestions, and parser result | Require application-level review or reject according to policy; a score is not a permission check |

## Recovery and rollback

- Prefer restoring the failed dependency or reverting the last relevant
  configuration/code change over weakening validation or access controls.
- Roll back only through the deployment's approved configuration/release
  process, and restore the last known-good retry, timeout, and resource bounds.
- If an operational exception is authorized, record approver, scope, expiry,
  and compensating controls; this document does not approve any bypass.
- Verify recovery with a bounded non-sensitive test request, successful
  parser validation, normal error/retry rates, and closed/recovering breaker
  state. Confirm direct AQL behavior separately.

## Escalation

Escalate provider/model health to the LLM service owner, parser or query
diagnostics to the query/AQL maintainers, and suspected unauthorized access or
prompt-injection activity through the project security incident process in
[`../../SECURITY.md`](../../SECURITY.md). Preserve only approved evidence and
redact user prompts and secrets.

## Related procedures

- [`OPERATIONS.md`](OPERATIONS.md) — health signals and routine operations.
- [`ERROR_RECOVERY_MATRIX.md`](ERROR_RECOVERY_MATRIX.md) — recovery categories.
- [`TRUST.md`](TRUST.md) — trust boundaries and incident evidence handling.
