---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Toolbox trust boundaries

**Purpose and audience:** Security and trust-boundary notes for developers and operators integrating the toolbox with content sources, ingestion workflows, model backends, and storage sinks.

## Trust boundaries

| Boundary | Toolbox responsibility | Required caller/owner control |
|---|---|---|
| Raw uploaded bytes | `ContentToolboxBridge::ingest()` forwards bytes to `ContentManager` before enrichment. | Keep content scanning, authorization, encryption, and storage policy in the content subsystem; do not bypass it for new raw-ingest flows. |
| Existing stored content | `enrichExisting()` reads assembled content and reruns enrichment. | The method does not re-run the raw-content security scan or deduplication. The caller must enforce authorization and decide whether re-enrichment is permitted. |
| Workflow profiles and steps | Builder loads profiles, registers format extractors, and injects engines/backends. | Restrict profile and plugin configuration to trusted operators; validate the selected steps and external dependencies at startup. |
| Text-generation backend | The toolbox forwards calls through ingestion interfaces; default construction uses a null backend. | The backend owner is responsible for model access policy, credential handling, output handling, and service isolation. Do not place secrets in profiles, logs, or returned errors. |
| Graph/vector writers | Optional sinks receive extracted entities or vector records. | Enforce authorization, schema compatibility, durability, and idempotency in the sink and calling service. A soft-failed write does not make `BridgeResult::ok` false. |
| Text helper input | Helpers process caller-provided strings and return transformed or heuristic results. | Validate input encoding and size at the application boundary where required. Normalization, language detection, and quality flags are not security validation. |

## Data and integrity notes

- `ContentFingerprinter` computes a deterministic SHA-256 digest of non-empty input bytes. It is a content identifier aid, not authentication, authorization, tamper-proof storage, or proof of provenance.
- Language and boilerplate outputs are heuristics. Do not use them alone for access decisions, policy enforcement, or safety classification.
- Text normalization folds selected German characters through the shared normalizer; it is not a general Unicode normalization or sanitization boundary.
- Error strings and IDs returned by bridge operations may be propagated to callers. Apply the host's log and response redaction rules before exposing them externally.
- Do not log raw document content, user context, model credentials, or full secret-bearing configuration while diagnosing failures.

## Failure handling

- Registry access before initialization throws, and repeated initialization replaces the registered instance with a warning. Treat unexpected registry lifecycle as an application configuration defect.
- Bridge-level failures and graph/vector sink failures have separate counters. Sink write failures can be soft-failed, so callers requiring durable writes must validate the appropriate sink outcome.
- An empty result does not establish that input was safe or that extraction succeeded; correlate it with workflow errors and metrics.
- Before retrying, determine whether content storage or one of the sinks already committed data. Re-enrichment skips content scanning and deduplication and can repeat sink writes.

## Incident response

Use the host application's security incident process when untrusted content reaches an unexpected workflow, access policy is bypassed, secrets appear in logs/results, or stored content is enriched without authorization. Preserve event IDs, affected content IDs, configuration version, and relevant counters, but not raw sensitive content or credentials. Coordinate with the content, ingestion, model-backend, and sink owners according to the failing boundary.

For operational triage, see [`RUNBOOK.md`](./RUNBOOK.md); for dataflow and integration contracts, see [`ARCHITECTURE.md`](./ARCHITECTURE.md), [`EMBEDDING.md`](./EMBEDDING.md), and [`SECURITY.md`](./SECURITY.md).

> Exact model backend not verifiable from final document text alone.
