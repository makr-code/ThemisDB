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

# AQL Assistance Trust Model

## Scope

This document defines trust boundaries for the AQL assistance module, which
handles natural-language requests, schema context, provider output, optional
embeddings, and application-supplied tool callbacks. The module is not itself
an authorization authority. The security source for the module is
[`SECURITY.md`](SECURITY.md); report vulnerabilities using the project
[`SECURITY.md`](../../SECURITY.md) process.

## Trust boundaries and risks

| Boundary | Risk | Required control |
|---|---|---|
| Client text to prompt/handler | Prompt injection, oversized input, sensitive user data | Apply input bounds and injection checks; treat text as untrusted; avoid logging raw content |
| Provider response to AQL caller | Malformed or adversarial output may be mistaken for executable query | Validate the final candidate; never execute partial streamed output; apply safety and authorization controls downstream |
| Schema context to generator | Stale or overbroad metadata can mislead generation or reveal schema | Provide only needed context and enforce collection/field access independently |
| AQL module to parser/query service | Syntax acceptance may be confused with semantic or permission approval | Treat parsing as a structural check only; preserve query-layer validation and runtime authorization |
| Embedding provider to example ranking | Provider failure, malformed/empty vectors, or ranking manipulation | Use explicit capability handling and documented lexical fallback; keep examples reviewed |
| Agent tool callback to application action | A model-directed tool may invoke privileged behavior | Register only reviewed callbacks; validate arguments and authorize each operation at the tool boundary |

## Authentication, authorization, and data handling

`LLMAQLHandler` exposes a collection-access checker hook, and the module
contains validation and injection-detection support. These are defense-in-
depth surfaces, not evidence that every call path has complete authorization.
The host application must authenticate the caller and authorize the final
query and each tool action using current policy. Do not rely on model output,
confidence scores, a prompt's schema context, parser success, or a collection
name check as a substitute for those controls.

Provider credentials belong in the host's approved secret-management path;
never place them in source, prompts, examples, logs, or incident attachments.
Apply least-privilege provider access and deployment-specific retention
controls to requests and responses. Redact prompt and query content from
diagnostics unless an approved incident process explicitly requires protected
evidence.

## Failure and incident handling

Validation failures, detected injection patterns, unavailable dependencies,
and unsupported capabilities should remain explicit and observable. Do not
turn a provider or parser outage into permission to execute unchecked output.
For suspected prompt injection, data exposure, or unauthorized tool/query
execution:

1. Stop or isolate the affected assistance route through approved controls.
2. Preserve sanitized logs, request identifiers, and configuration revisions;
   protect any sensitive evidence under incident policy.
3. Review authorization decisions and tool execution independently of
   translation/validation results.
4. Escalate using the project security incident process and restore service
   only after controls are verified.

## Evidence and references

- [`include/aql/llm_aql_handler.h`](../../include/aql/llm_aql_handler.h) —
  validation hooks and access-checker surface.
- [`include/security/aql_injection_detector.h`](../../include/security/aql_injection_detector.h)
  — parser/AST-oriented injection detector API.
- [`AQL_ASSISTANCE_CONTRACT.md`](AQL_ASSISTANCE_CONTRACT.md) — explicit
  translation and validation outcomes.
- [`RUNBOOK.md`](RUNBOOK.md) — operational diagnosis and safe recovery.
- [`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md) — module
  production controls and constraints.
