# Index Module — Frozen Core Contracts

**Version:** 1.0.0 (Frozen)  
**Status:** FROZEN — Q3 2026 gate delivery  
**Last Validated:** 2026-08-09  
**Frozen By:** Copilot agent (roadmap gate: Phase 1 Design / API Contract)

---

## 1. Scope

This document freezes the **core**, **acceleration**, and **lifecycle**
contracts for the Index module's active major line (v1.x).

All index components — `IAnnIndex`, `AnnFrontdoor`, `TieredIndexManager`,
`ShardedIndexProxy`, and derived backends — MUST conform to these contracts.

---

## 2. Core ANN Backend Contract (`IAnnIndex`)

### 2.1 Build Contract

| Method | Contract |
|---|---|
| `build(vectors, ids, count, dim)` | Builds the index from scratch. Returns `false` on failure; does not throw. Any prior state is discarded. |
| `add(id, vector, dim)` | Adds a single vector. Returns `false` if the backend cannot accept the vector (dim mismatch, full). |
| `size()` | Returns the current number of vectors. Thread-safe (const). |

**Invariants:**
- `build()` MUST be called before `search()` returns meaningful results.
- After `build()` returns `true`, `size() == count` MUST hold.
- Calling `search()` on an unbuilt index returns an empty vector without
  throwing.

### 2.2 Search Contract

| Method | Contract |
|---|---|
| `search(query, dim, k)` | Returns at most `k` candidates, sorted by ascending distance. Never throws. |

**Search invariants (frozen, enforced by AnnFrontdoor validation layer):**
- Result count ≤ `k` (cardinality guarantee).
- All distances ≥ 0.0 (range guarantee).
- No NaN or ±∞ distances in the returned vector.
- If the index has fewer than `k` vectors, `size()` entries are returned.

> **Note:** The AnnFrontdoor validation layer (Phase B gate, 2026-08-09)
> enforces these invariants defensively at the orchestration boundary.
> Backend implementations SHOULD satisfy them natively; the frontdoor filter
> is a last-resort guard for backends that violate them.
> See: `include/index/index_error_codes.h` `BACKEND_INVALID_RESULT`.

### 2.3 Persistence Contract

| Method | Contract |
|---|---|
| `save(path)` | Writes the index to disk. Returns `false` if not supported. |
| `load(path)` | Loads the index from disk. Returns `false` if not supported or path is invalid. |

---

## 3. AnnFrontdoor Contract

### 3.1 Strategy Selection Contract

`AnnFrontdoor::search()` selects a backend strategy deterministically from
`AnnRetrievalPlan` with the following precedence:

1. Caller-provided `AnnQueryContext::forced_strategy` (if set)
2. Distributed fan-out when `AnnQueryContext::use_distributed == true`
3. Dataset-size thresholds: HNSW ≤ `hnsw_max`, ScaNN ≤ `scann_max`,
   DiskANN > `scann_max`, otherwise FLAT_BRUTE_FORCE
4. Cold-tier demotion: HNSW → ScaNN/DiskANN when `hot_tier == false`
5. FLAT_BRUTE_FORCE fallback when no registered backend is available

### 3.2 Result Contract

`AnnFrontdoorResult` returned by `search()` guarantees:

- `candidates.size() ≤ k` (truncated defensively at the validation layer)
- All `candidates[i].distance ≥ 0.0` (NaN / negative entries removed)
- `candidates` sorted by ascending distance (closest first)
- `strategy_used` reflects the actually executed strategy, not the planned one
- `is_distributed == true` iff results were merged from ≥ 2 shard backends

### 3.3 Exception Contract

`AnnFrontdoor::search()` throws only `std::invalid_argument` for:
- `query_vector == nullptr`
- `dim == 0`

All other failure modes produce degraded results (empty candidates,
fallback strategy) rather than exceptions.

---

## 4. Lifecycle Contract

### 4.1 TieredIndexManager

| Operation | Contract |
|---|---|
| `registerHotTier(backend)` | Registers a backend as hot tier. Replaces existing. Not thread-safe during registration; must be called before concurrent searches. |
| `registerColdTier(backend)` | Same as above for cold tier. |
| `isHotTierAvailable()` | Thread-safe; returns current hot-tier availability. |

### 4.2 Rebuild Lifecycle

1. `initiateRebuild()` — marks index as rebuilding; searches continue on stale index
2. Ingest new vectors into a shadow index
3. `commitRebuild()` — atomically swaps shadow → live index
4. `abortRebuild()` — discards shadow index; live index unchanged

**Invariants:**
- At most one rebuild may be active at a time (`REBUILD_ALREADY_IN_PROGRESS`)
- A cancelled or failed rebuild MUST NOT modify the live index
- `size()` on the live index is unaffected during a rebuild

---

## 5. Error Taxonomy Reference

See `include/index/index_error_codes.h` for the frozen error code enum and
structured `IndexError` exception class.

Summary:
- **1100–1149**: Backend failures (`BACKEND_SEARCH_FAILED`, `BACKEND_DIM_MISMATCH`, etc.)
- **1150–1174**: Rebuild failures (`REBUILD_INGEST_FAILED`, `REBUILD_TIMEOUT`, etc.)
- **1175–1199**: Distribution failures (`DISTRIBUTION_ALL_SHARDS_FAILED`, etc.)

---

## 6. Backward Compatibility

- All contracts in this document are frozen for the v1.x major line.
- Additive changes (new backends, new error codes, new metadata fields) are
  permitted without a major version bump.
- Removal, renaming, or semantic change of any frozen item requires a **v2.x**
  version bump and a deprecation notice in CHANGELOG.md.
