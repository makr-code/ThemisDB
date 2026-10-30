---
Author: ThemisDB Team
Created: 2026-10-30
Last Updated: 2026-10-30
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

Exact model backend not verifiable from final document text alone.

# Image Analysis Trust Model

## Purpose and trust boundary

This document is for maintainers, integrators, and incident responders handling
untrusted images or model assets. Image bytes cross into native image decoding,
Tesseract, or ONNX Runtime code in the host process. Treat image content,
OCR text, model files, labels, and plugin libraries as untrusted until
validated by the deployment. The plugin API does not establish caller
authentication, authorization, tenant isolation, or output sanitization;
enforce those controls at the service and data-access boundaries.

## Risks and current controls

| Risk | Source-backed control / limit |
|---|---|
| Malformed image triggers parser/runtime faults | Decode/inference failures can return structured errors, but the inspected code does not provide a complete hostile-input safety guarantee or process isolation. |
| Missing Tesseract/ONNX dependency | CMake gates native code and inference returns explicit failure; initialization/readiness may still succeed in no-runtime builds. |
| Oversized or slow input exhausts resources | No module-level byte/pixel cap, timeout, queue, or admission control is implemented. Caller-side limits and concurrency control are required. |
| OCR text used as executable/query content | OCR output is data, not trusted syntax. Escape/parameterize it for every downstream context and apply normal authorization. |
| Malicious or incompatible model / labels | YOLO loads a configured ONNX path; model integrity verification is not implemented in the plugin source. Model registry/integrity validation is planned work. |
| Capability confusion | Shared interface includes operations beyond the two concrete plugins. Both current plugins report detection only; embeddings return unsupported errors. |

Build-time dependency gating and structured result objects reduce ambiguity but
do not sandbox native code. The manager header declares plugin hash/signature
verification, but the inspected module source set does not establish that
verification is invoked or enforced; do not treat that declaration as a
security control without tracing and testing the linked implementation.

## Operational controls

* Accept image requests only from authenticated/authorized callers and apply
  tenant/object access checks before and after analysis.
* Enforce byte and decoded-pixel limits, concurrency limits, deadlines, and
  rate limits before dispatch. Isolate processing in a restricted worker if
  the threat model requires containment of native parser/runtime failures.
* Source model binaries and language data from approved channels; verify
  integrity outside the current plugin path, restrict filesystem permissions,
  and keep writable upload areas separate from model paths.
* Treat OCR transcripts and labels as tainted output. Sanitize for display and
  use parameterized query APIs; never execute returned text as commands or
  query syntax.
* Avoid placing image bytes, transcripts, model paths, or secrets in routine
  logs/metrics. Limit access and retention for diagnostics.

## Incident handling

For suspected exploit, unexpected native crash, model tampering, or data
exposure: stop or isolate the affected backend, preserve the plugin/build
identity and relevant sanitized error/telemetry, retain suspicious input only
under approved evidence-handling policy, and notify the project security
response path in [root SECURITY.md](../../SECURITY.md). Roll back to a
previously approved plugin/model/config combination only after confirming
integrity; do not disable dependency/error checks to restore service.

For day-to-day failure recovery use [RUNBOOK.md](RUNBOOK.md). Planned model
integrity and resource-control improvements remain tracked in
[FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md), not implemented controls.
