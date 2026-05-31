# Security - Exporters Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

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

- policy and authorization paths gate exports before output activity.
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