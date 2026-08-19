# Security - Analytics Module

<!-- Status: current | validated: 2026-08-19 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in analytics focuses on execution isolation, bounded runtime behavior under load, safe integration with optional external services, and controlled export surfaces.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| unbounded stream or window growth | bounded windowing and runtime controls in streaming and CEP paths |
| unsafe distributed fan-out behavior | distributed analytics coordination with controlled merge semantics |
| unsafe external inference interactions | explicit serving integrations, transport policy validation, and structured error behavior |
| export misuse or leakage risk | dedicated export paths with analytics export interfaces |
| malformed analytical input | fail-closed validation and structured runtime errors |
| poisoned model artifacts | SHA-256 integrity check on `loadModel` with fail-closed mismatch handling |
| malformed or adversarial LLM JSON | task-specific schema/type/range validation with bounded payload limits |

## Implemented Security Controls

- analytics runtime uses explicit module-level execution paths rather than dynamic code execution.
- optional integration points are capability-gated and return structured errors when unavailable.
- streaming and CEP components provide bounded operational surfaces.
- model registry import path supports explicit SHA-256 validation and can enforce mandatory integrity metadata (`require_model_integrity=true`).
- TF Serving integration blocks insecure HTTP transport unless `allow_insecure_transport=true` is explicitly set.
- LLM process analyzer validates fraud/5R/prediction JSON payloads for type safety, numeric bounds, and bounded array/string sizes.

## Security Follow-ups

- continue hardening endpoint/authn expectations for remote serving and distributed paths.
- continue tightening runtime limits for worst-case data-volume scenarios.
- maintain observability for security-relevant failure classes.
- add signature/key-based artifact trust chain on top of current SHA-256 integrity checks.

## Sourcecode Verification (Module: analytics/security)

- Verified files:
  - src/analytics/cep_engine.cpp
  - src/analytics/streaming_window.cpp
  - src/analytics/streaming_join.cpp
  - src/analytics/distributed_analytics.cpp
  - src/analytics/ml_serving.cpp
  - src/analytics/analytics_export.cpp
- Verified controls:
  - bounded streaming/CEP runtime surfaces
  - explicit distributed and serving integration boundaries
  - structured execution surfaces for export and analytics orchestration