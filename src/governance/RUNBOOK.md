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
---

Exact model backend not verifiable from final document text alone.

# Governance Incident Runbook

## Purpose and safety

Use this runbook to diagnose governance denials, policy integration failures, audit/lineage degradation, masking concerns, and incomplete subject-rights operations. Preserve the active policy/version, relevant decision context, diagnostics, and audit evidence before changing configuration. Do not bypass an enforcement decision to restore availability.

For security incidents, use the private reporting route in [SECURITY.md](../../SECURITY.md) and the project incident procedures in [SOP.md](../../SOP.md). This runbook is module-specific and does not replace incident command.

## Policy decision unexpectedly denied

1. Capture the route, request classification, policy mode, tenant/user context, decision fields, and policy file version/hash without copying sensitive payloads into general logs.
2. Check whether `X-Classification` was supplied, whether a route mapping exists, and whether the label has a configured profile. Native evaluation uses `streng-geheim` when classification and route mapping are absent; unknown labels also take the strict result.
3. Check policy YAML parse/reload logs and `reloadIfChanged()` result. A failed parse is not a successful policy deployment; validate the candidate file and retry through the normal change process.
4. Inspect the active evaluator and `governance_opa_fallback_total`. A fallback means OPA did not return a usable decision; it is not equivalent to OPA approval.
5. Verify the final `PolicyDecision` and downstream enforcement. Do not weaken classification or force `observe` solely to clear the incident.

## OPA REST unavailable, malformed, or timing out

For REST mode, symptoms include a `kOpaUnavailable` diagnostic for
`opa_adapter`, network/timeout/invalid-policy counters, and increased
native-fallback count.

1. Check endpoint reachability, policy path/bundle load, response shape,
   connection timeout, and configured `timeout_ms`.
2. If REST evaluation returned `nullopt`, `PolicyEngine` uses native
   evaluation. Compare that result with the intended policy before resuming
   sensitive traffic.
3. If `mode == WASM`, treat a missing or failed injected `WasmEvalFn` as a
   strict deny. It does not fall back to REST or native policy; a bundle path
   is not evidence that policy executed.
4. Restore the intended evaluator/configuration, run the focused OPA tests, and
   verify a known allow and deny case before returning traffic.

## Lineage or audit forwarding degraded

Check `DataLineageTracker::getStatistics()`, circuit state, `last_error_code`, `governance_lineage_events_total`, and diagnostics for component `lineage_tracker`.

- `OPEN`: audit forwarding is suppressed, while event recording can continue locally and return `kCircuitBreakerOpen`. Local in-memory evidence is not a substitute for the configured durable audit sink.
- `HALF_OPEN`: recovery window elapsed; the tracker is probing. Confirm the audit sink health and observe whether the next forwarding attempt recovers or reopens the circuit.
- Size-limit diagnostics indicate FIFO eviction. Identify the configured retention cap and assess whether required evidence has already aged out.

Restore audit sink availability/capacity, inspect failure logs, and verify successful forwarding plus circuit recovery. Preserve the observed error and local statistics; do not clear evidence before incident review.

## Masking or subject-rights failure

- If sensitive fields appear unmasked, verify the policy is enabled, has a rule for the actual JSON key, and is applied on every output path. Non-string values do not receive string masking; disabled/empty policy is a no-op. Stop the affected export until all consuming paths are checked.
- For a subject erasure report with `fully_erased == false`, inspect each failed `store_results` entry, retry only failed targets under the approved procedure, and obtain a new complete report. There is no cross-store rollback, so do not report completion based on partial success.
- If tokenization is configured, verify a non-empty secret is sourced securely. Empty-secret/HMAC failure falls back to plain SHA-256 and is unsuitable for keyed pseudonym separation.

## Rollback and escalation

For a bad policy change, inspect the active version and audit history, check rollback safety, preview impact, obtain the normal incident approval, perform the supported rollback, then verify both an allow and deny case. Stop if safety checks block rollback or the candidate version is unavailable; escalate to the governance/security owner rather than editing runtime state manually.

Escalate immediately for suspected policy bypass, unexplained evaluator
changes, missing audit evidence, or uncertain erasure status. Follow
[TRUST.md](TRUST.md), [SECURITY.md](SECURITY.md), and [SOP-05/SOP-08](../../SOP.md).

## Source references

- [`policy_engine.cpp`](policy_engine.cpp), [`opa_adapter.cpp`](opa_adapter.cpp)
- [`data_lineage.cpp`](data_lineage.cpp), [`data_masker.cpp`](data_masker.cpp)
- [`gdpr_subject_rights.cpp`](gdpr_subject_rights.cpp)
- [Operational audit guide](OPERATIONAL_AUDIT_GUIDE.md), [trust model](TRUST.md)
