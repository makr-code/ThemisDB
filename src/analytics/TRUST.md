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

# Analytics Trust Boundaries

## Trust model

Analytics consumes caller-provided records, event streams, query results,
models, configuration, and optional external-service responses. It computes
results; it does not authenticate the original data producer or decide whether
a caller is authorized to receive a result. Authentication and authorization
must be enforced at the server/API and export boundaries that invoke these
interfaces.

## Boundaries and controls

| Boundary or risk | Control and operator responsibility |
|---|---|
| Untrusted or malformed event, tensor, or model input | Validate schema, shape, thresholds, sizes, and numeric ranges before use; propagate structured input errors. LLM analyzer responses are task-schema/range/bounds checked before materialization. |
| External model service | TensorFlow Serving defaults to HTTPS with certificate verification; plaintext requires explicit `allow_insecure_transport`. Configure endpoint, timeout, and any API key through the deployment's secret-management system. |
| Imported in-process model | `ModelServingEngine` offers SHA-256 verification and a `require_model_integrity` fail-closed setting. Compare against a hash obtained through a trusted channel. Hash checking alone does not establish publisher identity or signed provenance. |
| Streaming/resource exhaustion | Configure explicit window, queue, concurrency, and latency limits; monitor backpressure and rejection outcomes. Unbounded zero defaults are not a production safety envelope. |
| Distributed shard fan-out | Apply the configured tenant/shard topology and inspect partial-result/circuit-breaker metadata. A partial result is not proof of complete coverage. |
| Exported data or Arrow Flight | Enforce export authorization and Flight authentication middleware at the applicable integration boundary; validate destination and format. The exporter itself is not an authorization identity provider. |
| Similarity or heuristic classification | Treat scores and labels as analytical evidence, not identity, authorization, or sole high-impact decisions. Built-in character-trigram vectors and lexical heuristics have known coverage limits. |

The implementation surfaces are documented in
[`SECURITY.md`](./SECURITY.md), [`PRODUCTION_REQUIREMENTS.md`](./PRODUCTION_REQUIREMENTS.md),
[`ml_serving.h`](../../include/analytics/ml_serving.h),
[`model_serving.h`](../../include/analytics/model_serving.h), and
[`analytics_export.h`](../../include/analytics/analytics_export.h).

## Data handling and incident response

Avoid logging raw event records, prompts, model payloads, credentials, or
unredacted output fields. Use operation/correlation IDs, failure classes, and
operator hints for diagnosis. Restrict access to exported files and model
artifacts using the deployment's existing access controls; analytics does not
replace those controls.

For suspected tampering, quarantine the artifact, preserve its expected and
observed digest plus source provenance, and restore only a verified copy. For
credential exposure, revoke/rotate the credential at its owning provider,
remove it from logs or artifacts, and follow the project's security incident
process. Do not weaken integrity, TLS, or authorization controls as an
incident workaround.

## Known residual risks

- SHA-256 verification detects content mismatch but is not a signature or
  trust-chain verification; signed artifact provenance remains a follow-up in
  [`FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md).
- Optional backends, deployment-level authentication, model quality, and data
  provenance depend on configuration and controls outside this module.
- A score, prediction, process embedding, or heuristic text result can be wrong
  on distribution shift or unsupported language/domain inputs. Consumers must
  set decision thresholds and human review appropriate to the use case.

See [RUNBOOK.md](./RUNBOOK.md) for failure-specific action and
[OPERATIONS.md](./OPERATIONS.md) for deployment checks.
