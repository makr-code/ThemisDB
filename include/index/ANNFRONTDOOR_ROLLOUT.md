# AnnFrontdoor Rollout Guide

**Version:** 1.0.0  
**Status:** ACTIVE — Q3 2026 migration delivery  
**Last Validated:** 2026-08-09  

---

## 1. Overview

`AnnFrontdoor` is the canonical routing layer between query callers and ANN
backends (HNSW, ScaNN, DiskANN, Flat, Distributed).  All new retrieval flows
MUST use `AnnFrontdoor::search()` rather than calling `IAnnIndex::search()`
directly.

---

## 2. Already Migrated Callers

| Caller | Status | Notes |
|---|---|---|
| `HybridSearch::search()` | ✅ Migrated | `setAnnFrontdoor()` wires the frontdoor; legacy `vector_index_` fallback retained for backward compatibility during rollout |
| `TensorMidLayer` | ✅ Migrated | Integrates via `setAnnFrontdoor()` |
| `AdapterRepository` | ✅ Migrated | Integrates via `setAnnFrontdoor()` |

---

## 3. Migration Steps for New Callers

### 3.1 Dependency Injection

Add an `AnnFrontdoor` shared_ptr to the caller's constructor or via a setter:

```cpp
// In your header:
std::shared_ptr<index::AnnFrontdoor> ann_frontdoor_;

// Setter:
void setAnnFrontdoor(std::shared_ptr<index::AnnFrontdoor> fd) {
    ann_frontdoor_ = std::move(fd);
}
```

### 3.2 Call Pattern

Replace direct `IAnnIndex::search()` calls with:

```cpp
index::AnnQueryContext ctx;
ctx.scope_kind   = index::AnnScopeKind::Document;  // or appropriate scope
ctx.dataset_size = static_cast<int64_t>(index_size);
ctx.hot_tier     = true;                            // or derived from tier info
ctx.correlation_id = request_id;

auto result = ann_frontdoor_->search(
    query_vector.data(), query_vector.size(), /*k=*/10, ctx);

for (const auto& candidate : result.candidates) {
    // candidate.id, candidate.distance
}
```

### 3.3 Logging the Routing Decision

Use `result.routing_reason` and `result.strategy_used` for observability:

```cpp
THEMIS_DEBUG("ANN routing: strategy={} reason={}",
    index::annStrategyName(result.strategy_used),
    result.routing_reason);
```

### 3.4 Handling Partial Distributed Results

```cpp
if (result.is_distributed && result.shards_succeeded < result.shards_attempted) {
    THEMIS_WARN("Partial ANN results: {}/{} shards succeeded",
        result.shards_succeeded, result.shards_attempted);
    // Decide: accept partial results, retry, or return error
}
```

---

## 4. Backward Compatibility During Rollout

During the rollout period, callers MAY retain a legacy `IAnnIndex` fallback
when `ann_frontdoor_` is null.  The HybridSearch pattern is the reference:

```cpp
if (ann_frontdoor_) {
    // Use AnnFrontdoor path (canonical)
    auto result = ann_frontdoor_->search(...);
    // ...
} else if (legacy_index_) {
    // Legacy fallback (deprecated, will be removed in v2.x)
    auto result = legacy_index_->search(...);
    // ...
}
```

The legacy fallback MUST be removed before the v2.x line is released.

---

## 5. Configuration

`AnnFrontdoor::Config` key fields:

| Field | Default | Meaning |
|---|---|---|
| `hnsw_max_dataset_size` | 1,000,000 | Dataset size above which HNSW is not selected |
| `scann_max_dataset_size` | 5,000,000 | Dataset size above which DiskANN is selected |
| `default_k` | 10 | Top-k used when caller passes `k ≤ 0` |
| `fail_closed` | false | When true, partial shard failures return an error instead of degraded results |

---

## 6. Error Handling

Use `include/index/index_error_codes.h` for structured error reporting.
`AnnFrontdoor::search()` does not throw for recoverable failures — check
`result.candidates.empty()` for "no results" scenarios.

---

## 7. Acceptance Checklist

- [x] `HybridSearch` uses AnnFrontdoor path when `ann_frontdoor_` is set
- [x] `TensorMidLayer` / `AdapterRepository` use AnnFrontdoor
- [x] AnnFrontdoor validation layer guards cardinality + range (Phase B gate, 2026-08-09)
- [x] Frozen error taxonomy at `include/index/index_error_codes.h`
- [x] Frozen contract semantics at `include/index/INDEX_CONTRACT.md`
- [ ] Tensor mid-layer integration tests verify routing decisions non-empty (Q3 2026, search module)
- [ ] End-to-end integration test with real HNSW index (Q3 2026, search module Phase 4)
