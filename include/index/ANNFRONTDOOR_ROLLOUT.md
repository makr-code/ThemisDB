---
Author: "ThemisDB Team"
Created: "2026-08-09"
Last Updated: "2026-10-31"
Status: deprecated
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# AnnFrontdoor Rollout Notes (Historical)

> **Status:** The examples and migration checklist below describe a 2026-08
> rollout snapshot and are not current implementation instructions. Their
> signatures, configuration fields, defaults, and completion state have not
> been validated against current source. Use
> [`../../src/index/API.md`](../../src/index/API.md) and
> [`../../src/index/MODULE_GUIDE.md`](../../src/index/MODULE_GUIDE.md) for the
> current interface and integration preconditions.
>
> Current code still contains a direct `VectorIndexManager` path in
> `HybridSearch::search()` when no `AnnFrontdoor` is set. The approval and
> removal status for that existing path are not established by this source
> check; do not copy it as a new fallback pattern.

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

Use the route and fallback metadata declared by `AnnFrontdoorResult`:

```cpp
THEMIS_DEBUG("ANN route: strategy={} fallback={} reason={}",
    index::annStrategyName(result.strategy_used),
    result.fallback_mode,
    result.fallback_reason_code);
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

Current `AnnFrontdoor::Config` fields relevant to routing:

| Field | Default | Meaning |
|---|---|---|
| `hnsw_max_elements` | 1,000,000 | Element threshold used by HNSW routing |
| `scann_max_elements` | 50,000,000 | Element threshold used by ScaNN routing |
| `default_k` | 100 | Top-k used when caller passes `k ≤ 0` |
| `distributed_allow_partial_results` | true | Controls whether partial shard results are accepted |

---

## 6. Error Handling

`AnnFrontdoor::search()` rejects a null query pointer and zero dimension with
`std::invalid_argument`. Backend exceptions may also propagate from dispatch.
An empty candidate list alone does not distinguish an empty match from an
unavailable backend or a degraded route; inspect the returned error and routing
metadata. See [`../../src/index/API.md`](../../src/index/API.md).

---

## 7. Acceptance Checklist

- [x] `HybridSearch` uses AnnFrontdoor path when `ann_frontdoor_` is set
- [x] `TensorMidLayer` / `AdapterRepository` use AnnFrontdoor
- [x] AnnFrontdoor validation layer guards cardinality + range (Phase B gate, 2026-08-09)
- [x] Frozen error taxonomy at `include/index/index_error_codes.h`
- [x] Frozen contract semantics at `include/index/INDEX_CONTRACT.md`
- [ ] Tensor mid-layer integration tests verify routing decisions non-empty (Q3 2026, search module)
- [ ] End-to-end integration test with real HNSW index (Q3 2026, search module Phase 4)
