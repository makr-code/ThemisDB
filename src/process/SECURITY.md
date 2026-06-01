# Security - Process Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in the process module focuses on safe model intake boundaries, deterministic parser/validation behavior, explicit retrieval failure signaling, and bounded linking/compliance surfaces.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| malformed or oversized process definitions | bounded parser and validation paths |
| unsafe model/link manipulation | explicit process lifecycle and linking contracts |
| silent retrieval/context generation failures | explicit retrieval diagnostics and error propagation |
| hidden compliance/evaluation faults | explicit DMN/OCEL and conformance signaling |

## Implemented Security Controls

- model import/serialization paths are validation-gated.
- linking and retrieval operations expose explicit outcomes.
- malformed process definitions fail deterministically.
- compliance/evaluation paths remain observable and non-silent.

## Security Follow-ups

- continue hardening parser edge scenarios across supported model formats.
- tighten diagnostics around linking and retrieval mismatch conditions.
- expand stress coverage for high-churn process model operations.

## Sourcecode Verification (Module: process/security)

- Verified files:
  - src/process/bpmn_serializer.cpp
  - src/process/epk_serializer.cpp
  - src/process/epk_aris_xml_importer.cpp
  - src/process/process_model_manager.cpp
  - src/process/process_linker.cpp
  - src/process/dmn_evaluator.cpp
- Verified controls:
  - validation-gated import/lifecycle behavior
  - deterministic retrieval/linking failure handling
  - explicit observability for process compliance paths