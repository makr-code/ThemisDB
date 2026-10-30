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

# AI Module Operations

AI provenance note: Exact model backend not verifiable from final document text alone.

## Operating boundary

The module is an endpoint client and artifact mapper, not a hosted model,
artifact compiler, or process sandbox. Operators own endpoint availability,
provider credentials, callback dependencies, filesystem policy, and downstream
review and deployment of generated code. Do not infer production readiness from
the API defaults or from successful generation alone.

## Configuration checks

Before enabling generation in a deployment:

- Set `llm_endpoint` to the intended service and configure
  `allowed_llm_endpoints` with the exact same string when allow-listing is
  required. An empty list skips this check.
- Set a deployment-appropriate `timeout_ms`, `max_request_body_bytes`, and
  `max_response_body_bytes`. Defaults are 10,000 ms, 256 KiB, and 8 MiB.
- Keep any endpoint credentials out of prompt text and endpoint URL user-info.
  The debug log prints the configured endpoint string.
- If C1, sandbox, or C2 is enabled, configure its required callback and verify
  its failure path before accepting traffic.
- If the sandbox gate is enabled, ensure `sandbox_dir` and `output_dir` are
  writable, access-controlled, and covered by the deployment's retention and
  disk-capacity policy.
- Serialize use of each `AIPluginGenerator` instance; its counters and instance
  callback state are not synchronized.

## Observability and limits

`getStats()` exposes cumulative per-instance counts for validation, transport,
HTTP, parse, safety-gate, sandbox, and successful requests. Use counter deltas
and request volume for operational diagnosis; the code defines no alert
thresholds or reset API. When tracing is compiled in, generation emits the
`ai.plugin.generate` span and event names in `ai_plugin_generator.cpp`.
Tracing is conditional on the build configuration.

The built-in transport applies `timeout_ms` to curl's request and connection
timeouts. Transport errors are attempted up to three times with 100 ms and
200 ms delays between attempts; HTTP status errors fail immediately. A
response-size limit is checked after the transport has collected the body, so
operators should not interpret it as a streaming memory limit.

With the sandbox gate enabled, each generation materializes five files
(`plugin.hpp`, `plugin.cpp`, `plugin_test.cpp`, `CMakeLists.txt`, and
`manifest.json`) under a unique bundle directory in both configured roots.
This path does not execute or compile those files and does not provide automatic
retention or cleanup.

## Capacity and change management

Capacity planning must account for provider latency, up to three transport
attempts, response parsing, and retained artifact copies. The callback-based
unit test and benchmark do not supply live-provider capacity baselines. Use
provider-backed measurements for a deployment-specific concurrency and timeout
budget.

Review endpoint, callback, size-limit, or directory changes through the normal
configuration process. Validate a change against a controlled endpoint and the
focused AI tests before rollout. Keep a known-good configuration available for
rollback; avoid automatic request replay after ambiguous timeouts because the
provider may already have processed the call.

## Recovery

Use the failure classification and steps in the [runbook](./RUNBOOK.md). For
suspected unsafe output, stop downstream consumption first, preserve relevant
artifact/evidence according to local retention policy, and escalate through
the [trust and incident guidance](./TRUST.md). Do not solve an operational
failure by disabling validation or configured safety gates.

## References

- [AI module guide](./MODULE_GUIDE.md)
- [API configuration](./API.md)
- [Performance expectations](./PERFORMANCE_EXPECTATIONS.md)
- [Production requirements](./PRODUCTION_REQUIREMENTS.md)
