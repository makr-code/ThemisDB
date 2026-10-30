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

# Analytics Implementation Guide

## Ownership and composition

Public interfaces live under [`include/analytics`](../../include/analytics/);
runtime implementations are in this directory; focused tests are under
[`tests/analytics`](../../tests/analytics/). The module's main planes are:

- aggregation and OLAP (`aggregation.*`, `olap.*`, `columnar_execution.*`);
- streaming, windows, joins, and CEP (`streaming_window.*`,
  `streaming_join.*`, `cep_engine.*`);
- predictive and ML integrations (`forecasting.*`, `anomaly_detection.*`,
  `model_serving.*`, `ml_serving.*`);
- process analysis (`process_mining.*`, `process_pattern_matcher.*`);
- export and distributed coordination (`analytics_export.*`,
  `arrow_export.*`, `arrow_flight.*`, `distributed_analytics.*`).

Storage, query planning, server transport, and remote model lifecycle remain
owned by their respective modules; analytics consumes their interfaces rather
than taking ownership of those subsystems.

## Contracts to preserve

Before changing a public behavior, read
[`analytics_api_contract.h`](../../include/analytics/analytics_api_contract.h)
and the affected component header. The shared contract specifies deterministic
GROUP BY key ordering for unchanged data/schema, SQL-style null treatment,
overflow errors, stable OLAP plan selection, forecasting invalid-input and
missing-model errors, and canonical analytics error codes.

Streaming has more than one explicit bound/configuration surface. Preserve the
selected window semantics, late-event policy, limits, and `BackPressureMode`;
do not silently discard or retry an event without accounting for the result
contract. `BoundedExecutionPolicy` uses zero to mean “unconstrained” for each
field, so production callers that need an envelope must set nonzero limits.

Optional backends must report unavailable or unsupported states explicitly.
Do not silently substitute another output format, serving backend, or model
when the selected contract promises fail-closed behavior.

## Model, export, and concurrency invariants

- Model registry entries are named and versioned. Integrity verification is
  available on model import; a configured required-integrity mode rejects
  missing/mismatched hashes.
- TensorFlow Serving uses HTTPS and certificate verification by default;
  plaintext requires the explicit insecure-transport option.
- Export and serving operations should preserve operation/correlation IDs,
  failure class, and operator hints when returning structured results.
- Registry lock scope, window state, and shard state follow the ownership rules
  in their headers. In particular, do not hold shared registry locks across
  inference or external I/O.
- The process-mining implementation has an explicit Windows stub build behind
  `THEMIS_PROCESS_MINING_WINDOWS_STUB`; that build returns unsupported errors
  rather than performing mining. Do not mistake it for the normal
  implementation.

## Failure handling

Use typed status/result values for expected failures. Validate dimensions,
timestamps, thresholds, policy limits, optional dependency availability, and
model integrity at the boundary. Keep errors actionable without logging input
records, credentials, or full model payloads. For specific diagnosis and
recovery steps, see [RUNBOOK.md](./RUNBOOK.md) and [OPERATIONS.md](./OPERATIONS.md).

## Focused validation map

| Change area | Existing focused evidence |
|---|---|
| Shared policies and cross-cutting errors | `test_analytics_bounded_execution_policy.cpp`, `test_analytics_contract_hardening_focused.cpp` |
| Streaming and windows | `test_streaming_window.cpp`, `test_analytics_streaming_limits_focused.cpp`, `test_analytics_highcardinality_stress.cpp` |
| Forecasting and anomaly models | `test_forecasting.cpp`, `test_anomaly_detection.cpp` |
| Serving and integrity | `test_model_serving.cpp`, `test_ml_serving.cpp` |
| Process mining and matching | `test_process_discovery_conformance.cpp`, `test_process_pattern_matcher.cpp`, `test_process_mining_llm.cpp` |
| Export and distributed execution | `test_arrow_export.cpp`, `test_arrow_flight.cpp`, `test_distributed_analytics.cpp`, `test_analytics_distributed_coordinator_safety.cpp` |

Run the narrowest applicable suite in the configured build, then the
repository's required validation gates. Benchmark acceptance is defined in
[`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md); the presence of
a benchmark case is not evidence that representative hardware has passed it.

See [API.md](./API.md) for interface mapping and [MODULE_GUIDE.md](./MODULE_GUIDE.md)
for the recommended reading and validation sequence.
