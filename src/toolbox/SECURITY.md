# Security - Toolbox Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in the toolbox module focuses on deterministic extraction and bridge behavior, explicit registry/bootstrap guarding, bounded text-processing helper behavior, and observable soft-failure handling for ingestion-facing paths.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| hidden extraction pipeline failure | explicit empty/soft-fail result behavior and metrics surfaces |
| unsafe global registry access | explicit initialization guard and test reset behavior |
| malformed text-path degradation | deterministic helper behavior for chunking, normalization, and language detection |
| opaque bridge sink failures | logged and diagnosable content bridge soft-fail behavior |

## Implemented Security Controls

- builder and registry operations expose explicit lifecycle errors.
- bridge and extraction paths remain diagnosable under soft failures.
- helper utilities keep deterministic bounded behavior on empty or malformed input.
- streaming and composite dispatch remain explicit and observable.

## Security Follow-ups

- broaden fault-injection coverage for bridge sink and registry misuse edge cases.
- deepen stress coverage for synchronous streaming and composite-routing workloads.
- tighten diagnostics taxonomy across extraction and helper incident classes.

## Sourcecode Verification (Module: toolbox/security)

- Verified files:
  - src/toolbox/ingestion_toolbox.cpp
  - src/toolbox/toolbox_builder.cpp
  - src/toolbox/content_toolbox_bridge.cpp
  - src/toolbox/toolbox_registry.cpp
  - src/toolbox/toolbox_streaming.cpp
  - src/toolbox/text_quality_scorer.cpp
- Verified controls:
  - explicit bootstrap and registry guards
  - observable bridge/extraction soft-fail behavior
  - deterministic helper-path behavior