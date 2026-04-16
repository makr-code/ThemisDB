# Toolbox Module — Future Enhancements (Public Headers)

<!-- Status: current | validated: 2026-04-16 | Primary: include/toolbox/ | Secondary: docs/de/toolbox/ -->
<!-- Links: ../../src/toolbox/FUTURE_ENHANCEMENTS.md · ROADMAP.md -->

## Scope

API-level extensions planned for `include/toolbox/` headers:

- `IngestionToolboxMetrics` — Prometheus-compatible counters and histograms for Toolbox usage
- `ToolboxBuilder` graph-writer end-to-end wiring — connector from `withGraphWriter()` to `build()` result
- `ContentToolboxBridge` vector-sink path — populate `BridgeResult::vectors` from enrichment output
- `FUTURE_ENHANCEMENTS.md` for `src/toolbox/` companion — implementation notes for the above items
- Streaming enrichment interface — `extractEntitiesStream()` for large-text chunked enrichment
- Multi-toolbox composition — `ToolboxComposite` for routing different content types to specialized toolboxes

## Design Constraints

- [ ] `IngestionToolboxMetrics` is an optional dependency; its absence must not change runtime behavior in `IngestionToolbox` or `ToolboxBuilder` (compile-time flag `THEMIS_ENABLE_TOOLBOX_METRICS`)
- [x] Dependency direction: `toolbox/` → `ingestion/`; the reverse direction is permanently forbidden
- [x] `ToolboxBuilder` is not thread-safe; all `build()` calls happen on the same thread; the resulting `IngestionToolbox` is thread-safe
- [ ] `extractEntitiesStream()` must not buffer the full result in memory; it must yield entities incrementally using a `ResultStream<BaseEntity>` base interface
- [ ] Graph-writer wiring in `ToolboxBuilder::build()` must propagate the configured writer to `AQLIngestionBridge` and `RAGIngestionBridge` without breaking null-writer behavior
- [ ] `ToolboxComposite` routes by MIME type prefix; its routing table must be configurable at construction time and immutable thereafter

## Required Interfaces

| Interface | Consumer | Status |
|---|---|---|
| `IngestionToolboxMetrics` | `IngestionToolbox`, `ToolboxBuilder` | ❌ Not implemented (ROADMAP Phase 4) |
| `ToolboxBuilder::withGraphWriter(writer)` → propagated to bridges | `AQLIngestionBridge`, `RAGIngestionBridge` | ⚠️ API exists; propagation not wired |
| `ContentToolboxBridge::BridgeResult::vectors` | Vector store sinks | ⚠️ Field exists; never populated |
| `extractEntitiesStream(text, mime, filename)` | Large-document ingestion | ❌ Not yet designed |
| `ToolboxComposite` | Multi-format server bootstrap | ❌ Not yet designed |

## Planned Features

### 1. `IngestionToolboxMetrics` — Observability
**Priority:** High
**Target Version:** v1.9.0 (ROADMAP Phase 4)

Prometheus-compatible metrics exposed per `IngestionToolbox` instance.

**Proposed interface:**
```cpp
// include/toolbox/ingestion_toolbox_metrics.h
namespace themis::toolbox {

class IngestionToolboxMetrics {
public:
    /// Incremented on each call to extractEntities().
    virtual void recordExtractCall() = 0;

    /// Records latency of a single extractEntities() invocation.
    virtual void recordExtractLatencyMs(double ms) = 0;

    /// Incremented when extractEntities() returns an empty result due to
    /// workflow failure (no crash, but callers receive no enrichment).
    virtual void recordExtractEmpty() = 0;

    virtual ~IngestionToolboxMetrics() = default;
};

/// No-op implementation used as default when metrics are disabled.
class NullIngestionToolboxMetrics final : public IngestionToolboxMetrics {
public:
    void recordExtractCall() override {}
    void recordExtractLatencyMs(double) override {}
    void recordExtractEmpty() override {}
};

} // namespace themis::toolbox
```

**IngestionToolbox integration (DI):**
```cpp
// Injected via ToolboxBuilder::withMetrics(metrics)
// Default: NullIngestionToolboxMetrics (no-op)
toolbox->setMetrics(prometheus_metrics);
```

**Activation condition:** compile with `-DTHEMIS_ENABLE_TOOLBOX_METRICS=ON`

**Implementation Notes:**
- `extractEntities()` wraps the `engine->execute()` call with a `std::chrono::steady_clock` measurement.
- `recordExtractEmpty()` is called on the `!result` branch (graceful degradation path).
- `PrometheusIngestionToolboxMetrics` (in `src/toolbox/`) registers two Prometheus counters and one histogram.

---

### 2. `ToolboxBuilder` — Graph-Sink End-to-End Wiring
**Priority:** Medium
**Target Version:** v1.9.0

`ToolboxBuilder::withGraphWriter(writer)` stores the writer but currently does not pass it to the created toolbox or any bridges. The writer must be forwarded so that callers building with a `withGraphWriter()` call get effective graph-enrichment behavior without manually wiring bridges.

**Current behavior:**
```cpp
// src/toolbox/toolbox_builder.cpp — graphWriter() is accessor-only
// impl_->graph_writer is never used in build()
```

**Target behavior:**
```cpp
auto toolbox = ToolboxBuilder()
    .withGraphWriter(graph_sink)
    .withTextBackend(llm)
    .build();
// graph_sink is now transparently forwarded to the returned toolbox's
// AQLIngestionBridge and RAGIngestionBridge when those bridges are created
// by consumers via ToolboxBuilder::buildWithBridges() (new method).
```

**Proposed extension:**
```cpp
/// Build toolbox + pre-wired bridges in one call.
struct BuiltToolbox {
    std::shared_ptr<IngestionToolbox>            toolbox;
    std::shared_ptr<aql::AQLIngestionBridge>     aql_bridge;   // optional, set when toolbox has graph_writer
    std::shared_ptr<rag::RAGIngestionBridge>      rag_bridge;   // optional, set when toolbox has vector_writer
};
[[nodiscard]] BuiltToolbox buildWithBridges();
```

---

### 3. `ContentToolboxBridge` — Vector-Sink Population Path
**Priority:** Medium
**Target Version:** v1.9.0

`BridgeResult::vectors` exists in the struct but is never populated. The `ingest()` call produces chunks and embeddings inside `ContentManager`; those records must be surfaced into `BridgeResult::vectors` so the `vector_writer` sink path executes.

**Root cause:**
```cpp
// src/toolbox/content_toolbox_bridge.cpp:
// out.vectors is never filled — the if (!out.vectors.empty()) check is dead code
```

**Target behavior:**
```cpp
// After ContentManager::ingestRawBlob(), retrieve chunks:
auto chunks = impl_->content_manager_->getVectorRecords(cm_result.primary_content_id);
out.vectors = std::move(chunks);
// Now the vector_writer sink path executes correctly.
```

**Required new `ContentManager` API:**
```cpp
/// Returns the VectorRecords produced during ingestRawBlob() for content_id.
std::vector<ingestion::VectorRecord> getVectorRecords(const std::string& content_id) const;
```

---

### 4. Streaming Enrichment Interface
**Priority:** Low
**Target Version:** v2.0.0

For large documents (> 100 KB), `extractEntities()` buffers the full entity list in memory. A streaming interface allows consumers to process entities as they are produced without waiting for the full workflow to complete.

**Proposed interface:**
```cpp
// include/toolbox/ingestion_toolbox.h (new overload)

/// Streaming variant — yields entities incrementally.
/// Caller iterates via range-for on the returned stream.
/// Entities may arrive out of original text order.
///
/// @param text      UTF-8 source text.
/// @param mime      MIME type hint.
/// @param filename  Filename hint for workflow profile selection.
/// @return          Streaming range; empty on workflow failure.
ResultStream<ingestion::BaseEntity> extractEntitiesStream(
    const std::string& text,
    const std::string& mime     = "text/plain",
    const std::string& filename = "input.txt"
);
```

---

### 5. `ToolboxComposite` — Multi-Format Routing
**Priority:** Low
**Target Version:** v2.0.0

Production deployments often need different workflow profiles for different MIME types (legal PDFs vs. audio transcripts vs. office documents). `ToolboxComposite` routes `extractEntities()` calls to the appropriate sub-toolbox based on MIME prefix.

**Proposed interface:**
```cpp
// include/toolbox/toolbox_composite.h
namespace themis::toolbox {

class ToolboxComposite {
public:
    /// Register a toolbox for a MIME prefix.
    /// The first registered prefix that matches is used.
    /// Registrations are immutable after build().
    void registerToolbox(std::string mime_prefix,
                         std::shared_ptr<IngestionToolbox> toolbox);

    /// Fallback toolbox used when no prefix matches.
    void setDefault(std::shared_ptr<IngestionToolbox> toolbox);

    /// Route call to the matching sub-toolbox.
    std::vector<ingestion::BaseEntity> extractEntities(
        const std::string& text,
        const std::string& mime     = "text/plain",
        const std::string& filename = "input.txt"
    );
};

} // namespace themis::toolbox
```

---

## Test Strategy

- `IngestionToolboxMetrics`:
  - Unit-test `PrometheusIngestionToolboxMetrics`: verify counter/histogram increments after `extractEntities()` calls with and without profiles loaded.
  - Verify `NullIngestionToolboxMetrics` compiles and produces no observable side-effects.
- Graph-sink wiring:
  - Unit-test `ToolboxBuilder::buildWithBridges()`: build with a mock graph writer; verify `aql_bridge.graphWriter()` returns the configured writer.
  - Negative test: build without `withGraphWriter()`; verify `aql_bridge == nullptr`.
- Vector-sink path:
  - Unit-test `ContentToolboxBridge::ingest()` with a `MockContentManager` that returns non-empty `VectorRecord` list; verify `BridgeResult::vectors` is non-empty and `vector_writer::writeVectors()` is called.
- Streaming enrichment:
  - Unit-test `extractEntitiesStream()`: verify stream is empty on empty input; verify stream produces results without holding all entities in memory simultaneously (use a counting observer).
- `ToolboxComposite`:
  - Unit-test MIME routing: register `application/pdf` → toolbox-A and `text/plain` → toolbox-B; dispatch calls with matching and non-matching MIME types.
  - Test fallback: call with unregistered MIME; verify default toolbox is used.

## Performance Targets

- `extractEntities()` with metrics enabled: overhead ≤ 5 µs per call on the metrics recording path (no blocking, no lock contention).
- `extractEntitiesStream()` first-entity latency: ≤ 2× `extractEntities()` latency on the same input (streaming setup overhead).
- `ToolboxComposite::extractEntities()` routing overhead: ≤ 500 ns per call (prefix table lookup, no lock on hot path).
- `ToolboxBuilder::buildWithBridges()` vs. `build()`: ≤ 100 µs additional construction overhead.

## Security / Reliability

- `IngestionToolboxMetrics` implementations must not log or expose raw document text in metric labels.
- `ToolboxComposite` MIME routing must not expose workflow selection logic to callers (no `getToolboxFor(mime)` public accessor); routing is an implementation detail.
- `extractEntitiesStream()` cancellation must be thread-safe; cancellation from any thread stops the workflow without leaving partially-written sinks.
- Graph-writer null-safety: all new code paths involving `graph_writer` must guard with a null check before invoking `writeEntities()`; never throw from a null-writer code path.
