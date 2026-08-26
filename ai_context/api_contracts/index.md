# Index Module Contract

Datum: 2026-08-03  
**Status:** Active  
**Module:** index (HNSW, B-tree, R-tree, index lifecycle)  
**Primary:** include/index/hnsw_index.h, include/index/index_manager.h, src/index/ROADMAP.md

## Public API Surface

| API | Namespace | Input Contract | Output Contract | Errors | Thread-Safety | Ownership/Lifetime | Notes |
|---|---|---|---|---|---|---|---|
| `insert()` | `themis::index::IIndex` | Key (≤256B), Value (serializable), options | bool (true = inserted, false = updated) | SerializationError, OOMError | ✅ Thread-safe via internal RW-lock | Key/Value copied internally | P0; GATE-INDEX-03 ≤5µs |
| `search()` | `themis::index::IIndex` | Key (UTF-8, ≤1KB for fulltext), SearchOptions (limit=100, offset=0) | Vec<SearchResult> (score, key, distance; sorted by score) | KeyNotFoundError OK (return empty), IndexCorruptionError | ✅ Multi-reader (RW-lock shared) | Results owned by return; stable order | P0; GATE-INDEX-01 ≤10µs |
| `knnSearch()` | `themis::index::IIndex` | Query vector (float[], dim must match), k (≤10K) | Vec<(score, key)> (k nearest neighbors, sorted desc by score) | DimensionMismatchError, TopKTooLarge | ✅ Multi-reader | Neighbors owned by return; scores mutable | P0; GATE-INDEX-02 ≤8µs for k=10 |
| `delete()` | `themis::index::IIndex` | Key | bool (true if found & deleted, false if not found) | LockError if being compacted | ✅ Thread-safe (exclusive lock) | N/A (mutation only) | P0; GATE-INDEX-04 ≤3µs |
| `createIndex()` | `themis::index` | IndexSpec (type=HNSW|BTREE|RTREE, key-schema) | std::unique_ptr<IIndex> | InvalidSpecError, StorageError | 🔒 Single-threaded factory | Caller owns index; typically stored in manager | P1 (rare; startup/admin) |
| `rebuild()` | `themis::index::IIndex` | Optional: progress callback | void (index rebalanced, order optimized) | RebuildTimeout (default 60s), CheckpointError | ⚠️ Blocks all ops until complete | Callback borrowed; called per batch | P2 (maintenance); can run async |
| `getStats()` | `themis::index::IIndex` | N/A | IndexStats (node count, height, memory usage MB) | None | ✅ Lock-free snapshot | Stats value-owned; immutable | P2; utility |
| `estimateCost()` | `themis::index` | Query predicate (SELECT WHERE clause) | Cost (IO, CPU, cardinality estimate) | None | ✅ Lock-free (heuristic-based) | Cost value-owned | P1 (planning); not gated |

## Index Type Contracts

| Type | Insert | Search | KNN | Delete | Memory | Notes |
|---|---|---|---|---|---|---|
| HNSW | O(log n) amortized | ≤10µs P99 | ≤8µs P99 | O(log n) | 1.5× data | Vector-specific; hierarchical |
| B-tree | O(log n) | ≤10µs P99 | N/A | O(log n) | 1.1× data | Sorted ranges; range queries fast |
| R-tree | O(log n) | ≤10µs P99 (spatial) | N/A | O(log n) | 1.3× data | Spatial; 2D/3D/geo bounds |

## Concurrency & Locking

| Scenario | Behavior | Test |
|---|---|---|
| 32 threads searching, 1 inserting | Readers blocked during insert window (~1ms); P95 search latency <20µs | test_index_mixed_rwlock.cpp |
| Concurrent deletes + search | Deletes serialized; searches see consistent snapshot | test_index_delete_consistency.cpp |
| Rebuild during queries | Queries on old index until rebuild complete; no new inserts allowed | test_index_rebuild_concurrent.cpp |

## Invariants & Consistency

| Invariant | Enforcement |
|---|---|
| Search results ordered by score (descending) | Verified after every test query |
| KNN results exactly k neighbors (if available) | Checked in unit tests |
| Index height balanced (HNSW: max 16 levels) | Structure validation on rebuild |
| No duplicate keys in index | Enforced by insert (update if exists) |
| Deletion is idempotent | delete(k) twice = same result |

## Error Categories

| Error | When | Recovery |
|---|---|---|
| IndexCorruptionError | Checksum mismatch or internal invariant broken | Trigger rebuild (expensive) or drop & recreate |
| DimensionMismatchError | Vector dim ≠ index schema | Ensure consistent embedding model; may need reindex |
| KeyNotFoundError | Search for non-existent key (search returns empty, not error) | OK; treated as no-match |
| OOMError | Memory quota exhausted | Reduce cache size or split index across shards |

## Performance Commitments (Release Gates)

| Gate | Latency | K (KNN) | Concurrency | Test |
|---|---|---|---|---|
| GATE-INDEX-01 | search() ≤10 µs | N/A | 32 readers | bench_index_release_gates.cpp |
| GATE-INDEX-02 | knnSearch() ≤8 µs | 10 | 32 readers | bench_index_knn_gates.cpp |
| GATE-INDEX-03 | insert() ≤5 µs | N/A | 1 writer + 32 readers | bench_index_insert_gates.cpp |
| GATE-INDEX-04 | delete() ≤3 µs | N/A | 1 writer + 32 readers | bench_index_delete_gates.cpp |

## API Stability

| Item | Status | Notes |
|---|---|---|
| IIndex interface | Public v1.x | Frozen; new methods require v2.x |
| IndexSpec proto format | Internal | May change; not exposed in public API |
| Search result order | Guaranteed stable | Part of public contract |
| Cost estimator | Internal | Heuristic-based; accuracy improves over time |

---

**Zuletzt geprueft (Index contracts):** 2026-08-03
