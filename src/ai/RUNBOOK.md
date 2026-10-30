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

# AI Generation Runbook

AI provenance note: Exact model backend not verifiable from final document text alone.

## Scope and signals

This runbook covers incidents on `AIPluginGenerator::generatePlugin()`. It does
not cover provider model quality or deployment of generated plugins. Use the
per-instance `Stats` snapshot and application logs to classify failures:

| Counter | Likely class | First checks |
|---|---|---|
| `validation_errors` | Invalid prompt, oversized serialized request, or endpoint not on configured allow-list | Description/list bounds, request limit, exact endpoint string |
| `transport_errors` | Exhausted transport attempts or enabled C2 callback missing/failing | Endpoint reachability, timeout, callback health |
| `http_errors` | HTTP status failure or response over configured byte limit | Provider status and returned body size |
| `parse_errors` | Invalid JSON, oversized generated field, or missing implementation | Provider schema/version and payload structure |
| `safety_rejections` | Enabled C1 gate unavailable, errored, non-finite, or below threshold | Callback health, score, threshold, evaluation input |
| `sandbox_rejections` | Artifact write/read/copy or verifier failure | Directory permissions/space, artifact I/O, verifier output |
| `successes` | Generation and all enabled gates completed | Compare with request volume and failure deltas |

Counters are cumulative for the generator instance and have no reset method.
They are not synchronized for concurrent calls; collect snapshots consistently
with the application's access model. The implementation does not define alert
thresholds.

## Triage procedure

1. Record incident time, request correlation ID if available, generator
   configuration with secrets removed, and counter deltas. Preserve relevant
   logs and trace data before restarting the owning service.
2. Determine whether failure occurred before transport. Validation and
   allow-list failures require correcting the request/configuration; retrying
   the same input will not resolve them.
3. For transport failures, check DNS/routing, endpoint availability, and
   configured `timeout_ms`. The generator makes at most three attempts and
   waits 100 ms then 200 ms between them. HTTP status failures do not retry.
4. For HTTP or parse failures, inspect provider status and response format
   without logging full prompts, credentials, or generated source. Check the
   configured response limit and the source-defined field limits in
   [IMPLEMENTATION.md](./IMPLEMENTATION.md).
5. For C1/C2 gate failures, restore the required callback or its dependency.
   Keep the gate enabled while investigating; a missing or failed callback is
   intentionally a rejection.
6. For sandbox failures, verify that both configured directories exist or can
   be created, are writable, and have sufficient space. Verify the optional
   callback separately. Do not treat successful read-back as code isolation.

## Recovery and rollback

- Correct endpoint, size, timeout, directory, or callback configuration using
  the deployment's normal reviewed configuration path. Change one variable at
  a time and verify with the focused test suite or a controlled endpoint.
- If provider behavior changed, route generation back to the last known-good
  endpoint/configuration. Do not automatically replay a request after an
  ambiguous transport timeout: the provider may have received it before the
  connection failed.
- If generated output is suspected unsafe, stop its downstream loading or
  deployment, preserve the artifact and available evidence under approved
  retention controls, and escalate through the security process. Do not delete
  the sandbox/output directories wholesale; they may contain unrelated runs.
- Roll back the configuration change if error counters or gate rejection rates
  worsen. Do not disable validation or safety gates as a recovery shortcut.

## Escalation

Escalate repeated endpoint, timeout, or storage failures to the owning service
operator/provider contact. Escalate suspected unsafe generated code, credential
exposure, or unexpected gate acceptance to the project security contact using
the repository's security reporting process. Include sanitized configuration,
counter deltas, timestamps, and relevant artifact identifiers—not raw secrets
or unrestricted prompt/response bodies.

## References

- [Operations guidance](./OPERATIONS.md)
- [Trust boundaries](./TRUST.md)
- [Implementation limits](./IMPLEMENTATION.md)
- [Focused tests](../../tests/ai/test_ai_plugin_generator.cpp)
