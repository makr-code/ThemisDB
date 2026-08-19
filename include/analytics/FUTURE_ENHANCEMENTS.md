> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-08-19 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · ../../src/analytics/FUTURE_ENHANCEMENTS.md -->

# Analytics Module — Public Header Future Enhancements

**Module Path:** `include/analytics/`
**Canonical implementation enhancements:** [`../../src/analytics/FUTURE_ENHANCEMENTS.md`](../../src/analytics/FUTURE_ENHANCEMENTS.md)

---

## Scope

Planned enhancements to the **public header contract** in `include/analytics/`. Runtime pipeline, ML-model lifecycle, and benchmark work remain tracked in:

→ [`../../src/analytics/FUTURE_ENHANCEMENTS.md`](../../src/analytics/FUTURE_ENHANCEMENTS.md)

---

## Design Constraints

- `[x]` Columnar/JIT execution APIs must not expose arena layout or SIMD specifics to callers.
- `[x]` Streaming headers must maintain bounded-latency semantics; state management stays internal.
- `[x]` ML-serving headers must keep model-binary lifecycle behind the `ModelServingManager` interface.
- `[x]` Arrow export headers must preserve zero-copy transfer contracts for downstream consumers.

---

## Required Interfaces (Header Contract)

| Interface | Declared In | Consumer | Status |
|-----------|------------|----------|--------|
| `OLAPEngine::execute()` | `olap.h` | Query layer, API handlers | ✅ Stable |
| `CEPEngine` event-pattern API | `cep_engine.h` | Streaming event processors | ✅ Stable |
| `ForecastingEngine::predict()` | `forecasting.h` | Monitoring and alerting services | ✅ Stable |
| `ModelServingManager` lifecycle | `model_serving.h` | ML inference middleware | ✅ Stable |
| `ArrowFlightServer` serve/bind | `arrow_flight.h` | Bulk analytics consumers | ✅ Stable |

---

## Planned Enhancements

### Short-Term (Q3 2026)

- Align streaming-window and CEP header docs around a shared event-schema and schema-evolution contract.
- Document Arrow Flight batch-vs-streaming capability matrix for external analytics consumers.
- Add explicit stability annotations to experimental `automl.h` and `jit_aggregation.h` APIs.

### Medium-Term (Q4 2026)

- Introduce `analytics_policy.h` to provide per-query resource quotas and access-policy contract.
- Expose benchmark-reference latency and throughput notes alongside columnar and streaming hot paths.
- Deprecate any legacy row-oriented APIs superseded by columnar execution and annotate migration paths.
- extend model-serving contract from hash-integrity to signature/key-based verification metadata.
- add explicit environment policy hooks for secure-only external inference endpoints.

### Long-Term

- Unify ML-serving and AutoML result types behind a shared prediction-context envelope.
- Add extension hooks for embedders to inject custom forecasting backends and anomaly-detection algorithms.
- Provide a stable federated-analytics contract integrating `distributed_analytics.h` with cross-shard query planning.
