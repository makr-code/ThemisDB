---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-11-18"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Security - AI Module

AI provenance note: Exact model backend not verifiable from final document text alone.

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Threat Model

| Threat | Current Mitigation |
|---|---|
| oversized or malformed prompt input | local validation rejects empty and >8192-byte description |
| endpoint misuse or connectivity failures | explicit endpoint presence check, timeout bounds, transport error handling |
| untrusted endpoint response | non-2xx rejected; malformed JSON shape, field types, and dependency entries fail closed |
| sensitive log leakage | prompt and callback error text are not logged; endpoint logs and trace baggage contain only scheme, host, and port |
| partial artifact acceptance | missing mandatory implementation payload causes fail-closed error |

## Implemented Controls

- Validation runs before endpoint I/O.
- Curl enforces the positive configured timeout; injected callbacks must enforce their own deadline, and the instance HTTP-post callback has no timeout parameter. Transport retries are bounded to three attempts on the selected route.
- HTTP status codes are validated.
- JSON parsing is wrapped with exception-to-error conversion.
- Generated output is only accepted when required implementation content exists.
- Enabled C1/C2/sandbox callback configuration is checked before endpoint I/O; callback exceptions fail closed.
- The C1 threshold and returned score must be finite and in `[0,1]`.
- CAI rule-based fallback, ethics-evaluation failure, and non-finite/out-of-range scores cannot pass `passesAcceptanceCriteria()`.

## Security Gaps and Follow-ups

- Field-level validation now enforces bounded/unique `required_capabilities` and `dependencies` tokens.
- Endpoint safety now supports configurable allow-listing plus request/response size limits with fail-closed rejection.
- Sandbox and static-analysis pipeline for generated artifacts is not yet part of this module path.
- `required_capabilities` values are locally syntax-checked and forwarded only; no provider acknowledgement or generated-manifest capability validation exists. Do not treat them as satisfied.
- The AI module does not own the `LLMAQLHandler` inference/chat gates; those paths are separate `llm` module behavior.

## Planning Traceability

- Wave C strategic planning issue: `#5040`
- Dependency planning issues: Wave A `#5038`, Wave B `#5039`
- Remaining dependency tracking items for Wave A/B stability and C2 multi-node infra/security review are now explicitly closed in `ROADMAP.md` and `FUTURE_ENHANCEMENTS.md`.

## Sourcecode Verification (Module: ai/security)

- Verified file:
  - src/ai/ai_plugin_generator.cpp
- Verified controls:
  - timeout-bounded endpoint options
  - HTTP code checks
  - guarded JSON parsing
  - fail-closed required-field enforcement

## Issue Scope Traceability

- Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
- dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
- follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
