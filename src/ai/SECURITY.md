# Security - AI Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Threat Model

| Threat | Current Mitigation |
|---|---|
| oversized or malformed prompt input | local validation rejects empty and >8192-char description |
| endpoint misuse or connectivity failures | explicit endpoint presence check, timeout bounds, transport error handling |
| untrusted endpoint response | non-2xx rejected, JSON parse guarded, mandatory payload checks enforced |
| sensitive log leakage | debug logging truncates prompt text |
| partial artifact acceptance | missing mandatory implementation payload causes fail-closed error |

## Implemented Controls

- Validation runs before endpoint I/O.
- Endpoint invocation is bounded by timeout values.
- HTTP status codes are validated.
- JSON parsing is wrapped with exception-to-error conversion.
- Generated output is only accepted when required implementation content exists.

## Security Gaps and Follow-ups

- Additional validation for `required_capabilities` and `dependencies` remains a hardening task.
- Endpoint allow-listing and response-size hard limits should be enforced in a subsequent hardening iteration.
- Sandbox and static-analysis pipeline for generated artifacts is not yet part of this module path.

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
