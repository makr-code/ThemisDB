---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "security"
Canonical_References: "include/exporters/exporter_interface.h; include/exporters/export_encryption.h; src/exporters/*.cpp"
Related: "Issue #7346; DOC-WEEKLY-2026-44"
---

# Security - Exporters Module

Exact model backend not verifiable from final document text alone.

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in exporters focuses on safe data egress boundaries, authorization and policy gating, privacy-preserving output controls, and deterministic failure behavior in export pipelines.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| unauthorized data export attempts | policy-gated export authorization checks |
| leakage of sensitive fields | explicit field filtering and PII redaction controls |
| unsafe output propagation | encryption and bounded export path controls |
| malformed export config/input abuse | structured config/format validation failures |
| operational blind spots during export incidents | metrics and diagnostics surfaces |

## Implemented Security Controls

- export implementations call the policy guard before output/cursor activity
  when an `ExportOptions` policy engine is configured; a null pointer does not
  provide authorization.
- filtering and redaction paths constrain sensitive data exposure.
- encryption controls protect sensitive export outputs.
- explicit errors and metrics support production incident handling.

## Security Follow-ups

- continue hardening policy parity across all exporter variants.
- tighten diagnostics around filter/redaction rejection paths.
- extend stress coverage for high-volume mixed-sensitivity exports.

## Sourcecode Verification (Module: exporters/security)

- Verified files:
  - src/exporters/jsonl_llm_exporter.cpp
  - src/exporters/join_exporter.cpp
  - src/exporters/export_encryption.cpp
  - src/exporters/pii_detector.cpp
  - src/exporters/huggingface_hub_client.cpp
  - src/exporters/aql_predicate_filter.cpp
- Verified controls:
  - policy-gated authorization before export egress
  - filtering/redaction/encryption safety paths
  - explicit observable failure behavior for unsafe export scenarios