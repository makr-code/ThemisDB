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

# Governance Implementation Guide

## Purpose and scope

Use this guide when modifying governance runtime behavior. Public declarations live in `include/governance/`; implementation is under `src/governance/`. Keep policy decisions, compliance evaluation, data protections, and operations as separate concerns as described in [ARCHITECTURE.md](ARCHITECTURE.md).

## Policy-engine configuration and evaluation

`PolicyEngine::loadFromYAML(path)` reads:

- `vs_classification`: profiles with `encryption_required`, `ann_allowed`, `export_allowed`, `cache_allowed`, `redaction_level`, `retention_days`, and `log_encryption`.
- `enforcement.resource_mapping`: route/resource keys mapped to classification labels.
- `enforcement.default_mode`: default `enforce`/`observe` behavior.
- `data_masking.enabled` and `data_masking.rules`: field, strategy, optional truncation length, and optional collection secret.

The engine parses into temporary profile, mapping, and masking state, then swaps those values while holding its mutex. A YAML or standard exception returns `false` before publishing the replacement state. `reloadIfChanged()` checks the remembered path/mtime and reports a load failure as `false`; callers should not treat a failed reload as a successful policy update.

Native evaluation uses an explicit classification, then a configured route mapping, and finally `streng-geheim` when neither is available. Unknown classifications also resolve to the strictest policy result. An attached OPA decision is used when available; REST `std::nullopt` falls
back to native evaluation. In explicit WASM mode, the adapter returns a strict
deny when its injected evaluator is absent or fails, preventing native
fallback. CCPA opt-out overrides export permission after a successful
evaluator path.

## Data-protection implementation details

- `DataMasker` matches JSON string fields by key recursively, including strings nested in arrays. Non-string scalar values and unmatched fields pass through. With masking disabled or no rules, it returns the input unchanged; for duplicate field rules, the first rule wins.
- `TOKENIZE` uses HMAC-SHA-256 when a non-empty collection secret is supplied. Empty-secret or HMAC failure falls back to unkeyed SHA-256, which does not provide keyed pseudonym separation. Do not rely on that fallback for production tokenization.
- `DataLineageTracker` assigns missing event IDs/timestamps, appends events by dataset, and enforces global/per-dataset retention bounds by evicting oldest events. Audit logger failures can open a circuit breaker; local recording may continue while the returned result indicates audit is unavailable.
- `GdprSubjectRightsManager` serializes erasure requests per subject and calls registered targets independently. Its report records each result and sets `fully_erased` only when all targets succeed; there is no cross-target rollback.

## OPA boundary

REST evaluation has a configured endpoint, policy path, and timeout.
Network/timeout/HTTP/response-parse failures return no decision and record
diagnostics; `PolicyEngine` may then apply native evaluation. In explicit WASM
mode, an injected evaluator is the only local evaluation hook. Without it, or
if it fails, the adapter returns an enforced strict deny and does not
substitute REST or native evaluation. Bundle presence does not indicate policy
execution.

## Change and validation checklist

1. Change the narrowest public contract in the corresponding `include/governance/*.h`; keep implementation-only details private to `src/governance/`.
2. Add or update a focused regression under `tests/governance/` for the decision, failure, and boundary case. Cover caller-visible output rather than only internal state.
3. Update the relevant module docs in the same change; update [TRUST.md](TRUST.md) or [SECURITY.md](SECURITY.md) when a trust boundary changes.
4. Run the focused governance test and mapped performance benchmark when changing a hot path. See [MODULE_GUIDE.md](MODULE_GUIDE.md) for test names and [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md) for release thresholds.
5. Confirm invalid configuration, integration failure, and sensitive-data behavior remain explicit; do not turn a failure into an implicit permit.

## Source references

- [`policy_engine.cpp`](policy_engine.cpp), [`opa_adapter.cpp`](opa_adapter.cpp)
- [`data_masker.cpp`](data_masker.cpp), [`data_lineage.cpp`](data_lineage.cpp)
- [`model_governance.cpp`](model_governance.cpp), [`gdpr_subject_rights.cpp`](gdpr_subject_rights.cpp)
- [Public interfaces](../../include/governance/README.md)
