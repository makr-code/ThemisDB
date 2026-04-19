> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Analytics Module — Architecture Guide

**Version:** 1.0
**Last Updated:** 2026-04-06
**Module Path:** `src/analytics/`

---

## 1. Overview

The Analytics module transforms ThemisDB from a transactional database into a full analytical
platform. It provides OLAP multi-dimensional query processing, process mining, NLP text
analysis, time-series forecasting, machine learning integration, anomaly detection, and
complex event processing — all operating over the same data that the transactional engine
manages.

---

## 2. Design Principles

- **Separation from Query Parsing** – the analytics layer receives already-parsed query
  plans from the query module; it does not parse AQL.
- **Columnar Execution** – results are processed in columnar batches for CPU cache
  efficiency and optional Arrow/Parquet export.
- **Platform Parity** – full implementation on Linux/Unix; stub with error logging on
  Windows where native Arrow support is unavailable.
- **SIMD Acceleration** – AVX2 SIMD paths for aggregation hot paths.
- **Pluggable ML** – ML/AI functionality delegates to the `llm` module; the analytics
  module orchestrates but does not host model weights.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `olap.cpp` | Core OLAP engine: GROUP BY, CUBE, ROLLUP, GROUPING SETS |
| `streaming_window.cpp` | Streaming windows: tumbling, sliding, session, hopping with watermarks |
| `columnar_execution.cpp` | Columnar batch processor with AVX2 SIMD |
| `jit_aggregation.cpp` | JIT-compiled hot-path aggregation dispatch (LLVM-ready) |
| `process_mining.cpp` | Process discovery (Alpha / Heuristic / Inductive algorithms) |
| `process_pattern_matcher.cpp` | Conformance checking against reference process models |
| `llm_process_analyzer.cpp` | LLM-powered process analysis and anomaly explanation |
| `nlp_text_analyzer.cpp` | Sentiment analysis, entity extraction, modality detection |
| `diff_engine.cpp` | Dataset diffing and change detection |
| `cep_engine.cpp` | Complex Event Processing (production-ready) |
| `streaming_join.cpp` | `HashJoin` and `IntervalJoin` stream-stream join engines |
| `forecasting.cpp` | Time-series forecasting (LINEAR_REGRESSION, EXP_SMOOTHING, HOLT_WINTERS, ARIMA, ENSEMBLE); SARIMA/PROPHET defined in enum but not yet implemented |
| `anomaly_detection.cpp` | Statistical and ML-based anomaly detection |
| `arrow_export.cpp` | Apache Arrow IPC / Parquet export |
| `arrow_flight.cpp` | Arrow Flight RPC server/client for remote analytics (in-process + gRPC) |
| `analytics_export.cpp` | CSV / JSON export |
| `distributed_analytics.cpp` | Fan-out queries across shards (scatter-gather coordinator) |
| `automl.cpp` | Automated ML model selection |
| `ml_serving.cpp` | External ML inference (ONNX Runtime, TensorFlow Serving) |
| `model_serving.cpp` | In-process named+versioned model registry and online inference pipeline |
| `incremental_view.cpp` | Incremental materialized view maintenance |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│             AQL Query Engine (src/query/)                       │
│  executes analytical query plans, calls analytics engines       │
└──────────────────────────┬──────────────────────────────────────┘
                           │ analytical sub-plan
┌──────────────────────────▼──────────────────────────────────────┐
│                     Analytics Dispatcher                        │
│            routes to OLAP / Process / NLP / CEP                 │
└──────┬───────────┬──────────────┬────────────────┬──────────────┘
       │           │              │                │
┌──────▼──┐  ┌─────▼──────┐ ┌───▼──────┐ ┌───────▼────────┐
│  OLAP   │  │  Process   │ │   NLP    │ │      CEP       │
│ Engine  │  │  Mining    │ │Analyzer  │ │    Engine      │
│         │  │            │ │          │ │  (full impl)   │
└──────┬──┘  └─────┬──────┘ └───┬──────┘ └───────┬────────┘
       │           │              │                │
┌──────▼───────────▼──────────────▼────────────────▼────────────┐
│              Columnar Execution Engine (AVX2)                  │
└──────────────────────────┬─────────────────────────────────────┘
                           │
              ┌────────────▼────────────┐
              │  Arrow / Parquet Export  │
              └─────────────────────────┘
```

---

## 4. Data Flow

```
Query Plan (from query engine)
    │
    ▼
Analytics Dispatcher
    │
    ├─ OLAP path:
    │    executeSimpleGroupBy() / executeCubeQuery() / executeRollupQuery()
    │    → columnar hash aggregation (AVX2)
    │    → result cache
    │    → JSON / Arrow / Parquet
    │
    ├─ Process Mining path:
    │    process_mining.cpp → Alpha/Heuristic/Inductive algorithms
    │    → conformance checker (process_pattern_matcher.cpp)
    │    → LLM explanation (llm_process_analyzer.cpp)  [optional]
    │
    ├─ NLP path:
    │    nlp_text_analyzer.cpp → sentiment / entity / modality
    │    (delegates heavy NLP to llm module via plugin interface)
    │
    └─ Diff path:
         diff_engine.cpp → change set with additions/deletions/modifications
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Consumed by** | `src/query/` | analytics sub-plan execution |
| **Uses** | `src/llm/` | LLM-powered process analysis and NLP |
| **Uses** | `src/storage/` | data access for analytics queries |
| **Uses** | `src/sharding/` | `distributed_analytics.cpp` fan-out |
| **Exports to** | clients | Arrow IPC, Parquet, CSV, JSON |

---

## 6. Threading & Concurrency Model

- `OLAPEngine` is thread-safe; concurrent queries use independent `Impl` state.
- `columnar_execution.cpp` uses AVX2 on a single thread per batch; parallelism is
  achieved at the shard fan-out level (`distributed_analytics.cpp`).
- CEP streaming windows use a dedicated background thread per window group.
- Process mining algorithms are single-threaded per invocation.

---

## 7. Performance Architecture

| Technique | Where Applied |
|---|---|
| Hash-based aggregation | `olap.cpp` O(n) average grouping |
| Columnar data layout | `columnar_execution.cpp` for cache efficiency |
| AVX2 SIMD | Aggregation hot paths |
| Result caching | `OLAPEngine` caches repeated query results |
| Lazy evaluation | CUBE/ROLLUP skips unused dimension combinations |
| Incremental views | `incremental_view.cpp` avoids full re-computation |

---

## 8. Security Considerations

- Analytics queries run under the same RBAC/RLS enforcement as regular queries.
- NLP and process analysis via LLM: prompts are sanitized before submission to prevent
  data exfiltration through prompt injection.
- Parquet/Arrow exports are scoped to the requesting tenant.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `analytics.olap.result_cache_size` | 128 entries | Max OLAP result cache entries |
| `analytics.simd.avx2_enabled` | auto-detect | Enable AVX2 SIMD for aggregation |
| `analytics.export.parquet_enabled` | true (Linux) | Enable Parquet export |
| `analytics.cep.window_thread_pool_size` | 4 | CEP streaming window threads |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Unsupported operation on Windows | Return structured error; log via spdlog |
| Arrow/Parquet library unavailable | Disable export; log warning at startup |
| LLM timeout in process analysis | Return partial result without LLM explanation |
| OOM during large CUBE computation | Spill intermediate results (planned) |

---

## 11. Known Limitations & Future Work

- CEP engine (`cep_engine.cpp`) is fully implemented with NFA-based pattern matching, EPL parsing, window management, aggregation, alert dispatch, CDC integration, and stateful checkpointing.
- Windows platform uses stub implementations for `olap.cpp` and `process_mining.cpp`.
- Distributed analytics fan-out is production-ready for scatter-gather; result merging for CUBE across shards uses partial-result tolerance (< 20% shard failure).
- AutoML (`automl.cpp`) is production-ready; ONNX export (`exportONNX()`) is planned for Q4 2026.
- SARIMA and PROPHET are defined in the `ForecastMethod` enum (`include/analytics/forecasting.h:154-155`) but are not yet implemented in `forecasting.cpp`; planned for Q4 2026.

---

## 12. References

- `src/analytics/README.md` — module overview
- `src/analytics/ROADMAP.md` — roadmap and planned features
- `src/analytics/FUTURE_ENHANCEMENTS.md` — future enhancements
- `docs/architecture/architecture_multi_model.md` — multi-model data model
- `ARCHITECTURE.md` (root) — full system architecture
- `docs/de/analytics/README.md` — secondary documentation hub (German)
- `docs/de/analytics/olap_guide.md` — OLAP usage guide
- `docs/de/analytics/cep_guide.md` — CEP engine guide
- `docs/de/analytics/forecasting_guide.md` — forecasting guide
