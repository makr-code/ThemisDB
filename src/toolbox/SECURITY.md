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

## Unified Incident Taxonomy

### Layer 1: Orchestration (ingestion_toolbox.cpp)
| Code | Incident | Cause | Mitigation |
|------|----------|-------|-----------|
| EX-EMPTY | extraction_empty | No text extracted from content | Recorded via `toolbox_extract_empty_results_total` counter |
| EX-FAILED | extraction_failed | Processor returned error | Recorded via `toolbox_extraction_failures_total` counter |
| EX-TIMEOUT | extraction_timeout | Operation exceeded budget | Monitored via `toolbox_extraction_latency_us` histogram |
| EX-OVERFLOW | extraction_overflow | Output too large | Logged + error counter incremented |

### Layer 2: Bridge (content_toolbox_bridge.cpp)
| Code | Incident | Cause | Mitigation |
|------|----------|-------|-----------|
| BR-NO-TEXT | bridge_no_text | ContentManager couldn't extract text | Soft-fail with empty result; `toolbox_bridge_failures_total` incremented |
| BR-WRITER | bridge_writer_failed | Writer sink (graph/vector) returned error | Soft-fail logged; `toolbox_bridge_latency_us` tracked; granular failure counters |
| BR-TOOLBOX | bridge_toolbox_failed | toolbox_ returned error | Logged + `toolbox_bridge_failures_total` incremented |
| BR-EMPTY | bridge_empty_result | Bridge succeeded but with no entities | Recorded in metrics; not counted as error |

### Layer 3: Registry (toolbox_registry.cpp)
| Code | Incident | Cause | Mitigation |
|------|----------|-------|-----------|
| REG-NOT-INIT | registry_not_initialized | globalToolbox() called before initialize() | Exception thrown; `toolbox_registry_misuse_total` incremented |
| REG-DOUBLE | registry_double_init | initialize() called when already initialized | Overwrites previous; logged warning |
| REG-RESET-ACTIVE | registry_reset_during_active | reset() during active usage | Soft-fail; subsequent calls throw; `toolbox_registry_misuse_total` incremented |

### Layer 4: Helper (text_*.cpp)
| Code | Incident | Cause | Mitigation |
|------|----------|-------|-----------|
| HLP-EMPTY | helper_empty_input | Empty text passed to helper | Returns default/empty result; `toolbox_helper_errors_total` incremented |
| HLP-ENCODING | helper_encoding_unsupported | Text encoding not supported | Logged + error counter incremented; fallback to UTF-8 |
| HLP-SIZE | helper_size_exceeded | Helper exceeded size limit | Truncate/skip; `toolbox_helper_errors_total` incremented |
| HLP-MALFORMED | helper_malformed_input | Invalid format for operation | Returns default; error counter incremented |

## Threat Model (Updated)

| Threat | Layer | Severity | Current Mitigation | Observable Signal |
|--------|-------|----------|-------------------|-------------------|
| hidden extraction pipeline failure | Orchestration | HIGH | explicit empty/soft-fail result behavior and metrics surfaces (EX-*) | toolbox_extraction_* counters + latency histogram |
| unsafe global registry access | Registry | HIGH | explicit initialization guard and test reset behavior (REG-*) | toolbox_registry_misuse_total + exception logging |
| malformed text-path degradation | Helper | MEDIUM | deterministic helper behavior for chunking, normalization, and language detection (HLP-*) | toolbox_helper_errors_total counters |
| opaque bridge sink failures | Bridge | MEDIUM | logged and diagnosable content bridge soft-fail behavior (BR-*) | toolbox_bridge_failures_total + granular writer failure counters |

## Sourcecode Verification (Module: toolbox/security)

- Verified files:
  - src/toolbox/ingestion_toolbox.cpp
  - src/toolbox/toolbox_builder.cpp
  - src/toolbox/content_toolbox_bridge.cpp
  - src/toolbox/toolbox_registry.cpp
  - src/toolbox/toolbox_streaming.cpp
  - src/toolbox/text_quality_scorer.cpp
  - src/toolbox/text_chunker.cpp
  - src/toolbox/text_normalizer.cpp
- Verified controls:
  - explicit bootstrap and registry guards
  - observable bridge/extraction soft-fail behavior
  - deterministic helper-path behavior
  - unified incident taxonomy across 4 execution planes
  - Prometheus metrics instrumentation for all incident classes