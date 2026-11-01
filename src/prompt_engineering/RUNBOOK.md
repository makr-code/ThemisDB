---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Prompt Engineering Runbook

## Scope

This runbook supports triage of prompt lifecycle, evaluation, rewrite, and feedback issues. The module is a library subsystem: it does not define a standalone health endpoint or a deployment-specific dashboard. Use the owning service's logs, metrics adapter, and storage procedures.

## First response

1. Identify the affected prompt/template ID, operation, time window, deployment version, and whether the in-memory or RocksDB-backed path is in use.
2. Capture the operation's actual result or exception fields and nearby module logs. The error-code enum does not imply a shared `PromptEngineeringErrorContext` API. Avoid copying raw prompts, responses, credentials, or user context into an incident ticket; use a redacted sample where possible.
3. Check `PromptEngineeringIntegration::getStatus()` and background-worker status when the integration façade is used. Export `PromptEngineeringMetrics::exportMetrics()` through the service's configured metrics adapter; the library itself only returns metric text.
4. Preserve current prompt version IDs and feedback snapshots before attempting rollback, pruning, or configuration replacement.

## Symptom-to-diagnosis

| Symptom | Inspect | Safe next action |
|---|---|---|
| Template creation returns an empty ID | `PromptManager::validateTemplate()` errors; name, version, content, metadata, image alt text | Correct the invalid fields and validate again. An empty description is a warning, not a create failure. |
| Template lookup/rendering returns no value | Requested ID and active flag; persistence read/parse warnings | Confirm the ID and template state. `getPromptWithContext()` treats inactive templates as unavailable. |
| Rendered prompt has unresolved placeholders | Input content and context keys | Supply the exact `{key}` entries. The renderer does not reject missing keys or escape replacement text. |
| Injection warning or high risk score | `DetectionResult` score, matched labels, detector config, caller integration path | Review the redacted input and the specific heuristic. Do not treat a clean score as proof of safety or disable production controls to suppress alerts. |
| Rewrite fails or stops | `RewriteResult` error code/message, phase, `RewriteContext::max_steps`, registered rule IDs, engine stats | Correct invalid rules or priority/phase assumptions. If the step bound is reached, inspect for a cycle or repeated transformation before raising the bound. |
| Rewrite document is blocked | `was_blocked`, terminal policy rule ID and trace | Treat the block as a policy decision. Escalate to the rule owner before changing a policy rule. |
| Evaluation score changes unexpectedly | Fixture set, evaluator config, provider identity/model, per-case metrics | Re-run baseline and candidate on the same fixtures and configuration; distinguish provider fallback from embedding scores. |
| Feedback persistence or cross-shard publishing fails | Storage logs, `FeedbackCollector` result, whether both sync and embedding model are configured | Local recording can succeed when cross-shard publish fails. Verify local persistence separately; do not retry publishing blindly without checking duplicate semantics. |

## Recovery and rollback

- For a bad prompt revision, verify the target version with `PromptVersionControl::getVersion()` and history, then use its rollback API to create the recovery revision. Re-run the regression fixtures and verify the active template/version in the owning integration before restoring traffic.
- For a bad rewrite rule file, retain the previous reviewed file. Validate and test the replacement in an isolated test path before loading it. The engine skips duplicate rule IDs rather than replacing their definitions; explicitly unregister affected rules before re-registering, and verify `list_rules()` and a representative rewrite result.
- For a malformed provider or unstable evaluator, remove the provider only in a controlled test/recovery context, confirm lexical fallback behavior on the same fixtures, and restore the known provider/config only after parity checks. Do not silently change the scoring basis for a production comparison.
- Do not edit RocksDB records directly as an ad hoc repair. Follow the storage owner's backup/restore procedure and preserve version/feedback evidence.

## Escalation evidence

Include module and application versions, operation, redacted prompt/template ID, error code, rewrite phase/rule ID if applicable, metric snapshot, provider/config identifier, and relevant test/benchmark results. Escalate authorization, data exposure, or policy-bypass concerns through the project security incident process in [`../../SECURITY.md`](../../SECURITY.md).

Related operational context: [`OPERATIONS.md`](./OPERATIONS.md), [`TRUST.md`](./TRUST.md), and [`ERROR_TAXONOMY_REFERENCE.md`](./ERROR_TAXONOMY_REFERENCE.md).

Exact model backend not verifiable from final document text alone.
