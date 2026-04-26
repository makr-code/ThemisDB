> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

# Toolbox Module — Future Enhancements (Implementation)

<!-- Status: current | validated: 2026-04-16 | Primary: src/toolbox/ | Secondary: docs/de/toolbox/ -->
<!-- Links: ../../include/toolbox/FUTURE_ENHANCEMENTS.md · ROADMAP.md -->

## Scope

Implementation-side tasks for `src/toolbox/` aligned with the header-level plan in
`include/toolbox/FUTURE_ENHANCEMENTS.md`:

- `PrometheusIngestionToolboxMetrics` — concrete metrics backend behind `IngestionToolboxMetrics` interface
<!-- TODO: add measurable target, interface spec, and test strategy -->
- Graph-sink wiring in `toolbox_builder.cpp` — forward `impl_->graph_writer` to produced bridges
<!-- TODO: add measurable target, interface spec, and test strategy -->
- Vector-sink population in `content_toolbox_bridge.cpp` — fill `BridgeResult::vectors` from `ContentManager`
<!-- TODO: add measurable target, interface spec, and test strategy -->
- `extractEntitiesStream()` — chunked streaming enrichment implementation
<!-- TODO: add measurable target, interface spec, and test strategy -->
- `ToolboxComposite` — MIME-routing composite toolbox
<!-- TODO: add measurable target, interface spec, and test strategy -->

## Implementation Notes

### `PrometheusIngestionToolboxMetrics`

Location: `src/toolbox/prometheus_ingestion_toolbox_metrics.cpp`
Guard: `THEMIS_ENABLE_TOOLBOX_METRICS`

```cpp
// STUB/SIMULATION NOTE:
// Purpose: placeholder until a Prometheus C++ client library is available
// Activation: compiled only with THEMIS_ENABLE_TOOLBOX_METRICS=ON
// Production Delta: real implementation uses prometheus-cpp counter/histogram registration
// Removal Plan: replace atomic stubs with prometheus-cpp calls when library is vendored
class PrometheusIngestionToolboxMetrics final : public IngestionToolboxMetrics {
    std::atomic<uint64_t> extract_calls_{0};
    std::atomic<uint64_t> extract_empty_{0};
    // histogram: stored as running sum + count for simplicity
    std::atomic<double>   latency_sum_ms_{0.0};
    std::atomic<uint64_t> latency_count_{0};
public:
    void recordExtractCall()               override { extract_calls_.fetch_add(1, std::memory_order_relaxed); }
    void recordExtractLatencyMs(double ms) override {
        latency_sum_ms_.fetch_add(ms, std::memory_order_relaxed);
        latency_count_.fetch_add(1, std::memory_order_relaxed);
    }
    void recordExtractEmpty()              override { extract_empty_.fetch_add(1, std::memory_order_relaxed); }
};
```

`IngestionToolbox::extractEntities()` wraps the `engine->execute()` call:
```cpp
auto t0 = std::chrono::steady_clock::now();
metrics_->recordExtractCall();
auto result = engine->execute(ctx);
auto t1 = std::chrono::steady_clock::now();
metrics_->recordExtractLatencyMs(
    std::chrono::duration<double, std::milli>(t1 - t0).count());
if (!result) {
    metrics_->recordExtractEmpty();
    return {};
}
```

### Graph-Sink Wiring in `ToolboxBuilder::build()`

Current gap: `impl_->graph_writer` is stored but never used in `build()`.

Target: expose it via `BuiltToolbox` returned by `buildWithBridges()`:
```cpp
ToolboxBuilder::BuiltToolbox ToolboxBuilder::buildWithBridges() {
    auto toolbox = this->build();  // existing build() path unchanged
    BuiltToolbox out;
    out.toolbox = toolbox;
    if (impl_->graph_writer) {
        out.aql_bridge = std::make_shared<aql::AQLIngestionBridge>(
            toolbox, impl_->graph_writer);
    }
    // vector_writer not yet wired (depends on ContentToolboxBridge vector path)
    return out;
}
```

Existing `build()` remains unchanged — no breaking change.

### Vector-Sink Population in `ContentToolboxBridge::ingest()`

Current gap: `out.vectors` is never populated; the `vector_writer_` sink path is dead code.

Target:
```cpp
// After ContentManager ingest, retrieve VectorRecords from ContentManager:
auto vec_records = impl_->content_manager_->getVectorRecords(
    cm_result.primary_content_id);
out.vectors = std::move(vec_records);
// Now the existing vector_writer_ sink path executes correctly.
```

Requires `ContentManager::getVectorRecords(content_id)` to be added to
`include/content/content_manager.h` and implemented in `src/content/content_manager.cpp`.

### `extractEntitiesStream()` — Streaming Path

Key implementation constraint: the workflow engine currently returns a full
`WorkflowResult` in one shot. Streaming requires either:
1. Text chunking before `execute()` (pre-processing streaming), or
2. A chunked workflow step that emits partial results via a callback.

Recommended approach for v2.0.0: **pre-processing streaming** using a `TextChunker`
that splits input by token window and calls `extractEntities()` per chunk on a
background thread pool, yielding results through a `ResultStream<BaseEntity>`.

### `ToolboxComposite`

Implementation in `src/toolbox/toolbox_composite.cpp`.
Internal routing table: `std::vector<std::pair<std::string, shared_ptr<IngestionToolbox>>>`.
Match algorithm: linear scan with `starts_with(mime, prefix)`.
Thread-safety: routing table is const after construction; no mutex needed on hot path.

## Known Limitations Before These Enhancements Land

- `ToolboxBuilder::withGraphWriter(writer)` is effectively a no-op until the `buildWithBridges()` path is implemented; callers must manually wire bridges.
- `ContentToolboxBridge` silently skips vector-store writes for all content; this affects RAG pipelines that depend on the bridge for embedding population.
- No production-observable metrics are available for `IngestionToolbox` invocations; operators must infer enrichment behavior from upstream workflow logs.
