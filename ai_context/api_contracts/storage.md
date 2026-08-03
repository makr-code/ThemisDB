# Storage Module Contract

**Datum:** 2026-08-03  
**Status:** Active  
**Module:** storage (RocksDB backend, K-V layer, LSM-tree)  
**Primary:** include/storage/storage_engine.h, src/storage/ROADMAP.md

## Public API Surface

| API | Namespace | Input Contract | Output Contract | Errors | Thread-Safety | Ownership/Lifetime | Notes |
|---|---|---|---|---|---|---|---|
| `put()` | `themis::storage::IStorageEngine` | Key (≤8KB), Value (≤128MB), WriteOptions | Status (OK or ErrorCode) | WriteError (disk full, IO error), KeyError (invalid key) | ✅ Thread-safe (internal queue serialization) | Key/Value copied; ownership transferred to engine | P0; GATE-STOR-01 ≤50µs |
| `get()` | `themis::storage::IStorageEngine` | Key (UTF-8, ≤8KB), ReadOptions (snapshot_id, timeout) | std::optional<std::string> (value if found, empty if not) | NotFoundError (treated as empty, not error), ReadError (IO/corruption) | ✅ Multi-reader (snapshot isolation) | Value owned by return; may be cached | P0; GATE-STOR-02 ≤20µs |
| `delete()` | `themis::storage::IStorageEngine` | Key (≤8KB), DeleteOptions (immediate/deferred) | Status (OK or error) | DeleteError (IO), KeyNotFoundError (OK; idempotent) | ✅ Thread-safe (internal serialization) | N/A (mutation only) | P0; GATE-STOR-03 ≤30µs |
| `scan()` | `themis::storage::IStorageEngine` | Range (start_key, end_key), batch_size (≤1K), snapshot | Iterator<(Key, Value)> (lazy; yields batch_size items per next()) | ScanError (range invalid), SnapshotExpired | ✅ Multi-scan safe (each gets consistent view) | Iterator borrowed from engine; valid ≤1 hour | P1 (bulk ops); streaming |
| `deleteRange()` | `themis::storage::IStorageEngine` | Key range [start, end), async flag | void (keys in range scheduled for deletion) | RangeError (invalid bounds), DeleteError | ✅ Thread-safe (queued operation) | N/A (mutation only) | P1; async by default |
| `createSnapshot()` | `themis::storage::IStorageEngine` | Snapshot ID (optional; auto-generated if empty) | Snapshot handle (unique, valid 1 hour) | SnapshotLimitError (max 100 concurrent) | ✅ Thread-safe (atomic creation) | Handle owned by caller; must close explicitly | P1 (rare; MVCC) |
| `releaseSnapshot()` | `themis::storage::IStorageEngine` | Snapshot handle | void | SnapshotNotFoundError (already released) | ✅ Thread-safe | N/A (resource cleanup) | P1 (paired with createSnapshot) |
| `compact()` | `themis::storage::IStorageEngine` | Range (optional), strategy (balanced/aggressive) | void (LSM compaction runs async; progress available via stats) | CompactionError (storage failure) | 🔒 Single active compact per engine | N/A (maintenance) | P2 (admin); ≤30s typical |
| `stats()` | `themis::storage::IStorageEngine` | N/A | StorageStats (size_mb, key_count, write_amp, compaction_progress) | None | ✅ Lock-free snapshot | Stats value-owned; immutable | P2; utility |

## Transaction & MVCC Contracts

| Operation | Contract | Notes |
|---|---|---|
| Begin snapshot | Creates consistent read view at MVCC timestamp | All gets within snapshot see same version |
| Concurrent writes | Each write assigned new sequence number | Readers skip uncommitted writes via MVCC filter |
| Read committed | Default isolation level; snapshots survive across multiple ops | Snapshot ID unchanged per transaction |
| Compaction | Removes old versions not in any snapshot | Blocked if snapshot active; background otherwise |

## Write Batching & Durability

| Level | Durability | Latency | Use |
|---|---|---|---|
| SyncWrite | fsync per write | ≤100µs | Transactions, critical ops |
| BatchWrite | fsync per batch (default 100) | ≤50µs | Normal CRUD |
| AsyncWrite | fsync ~100ms interval | ≤5µs | Non-critical logging |

## Concurrency & Isolation

| Scenario | Behavior | Test |
|---|---|---|
| 64 concurrent puts | Serialized via write queue; median latency ≤50µs | test_storage_concurrent_writes.cpp |
| Concurrent gets + compact | Gets see consistent snapshot; compact waits if snapshot active | test_storage_snapshot_safety.cpp |
| Range delete during scan | Scan blocks until delete completes; no partial reads | test_storage_deleterange_scan_order.cpp |

## Invariants & Durability

| Invariant | Enforcement |
|---|---|
| No data loss after acknowledged write | WAL ensures all put() ACK'd = durably written |
| Snapshot isolation across reads | MVCC sequence numbers enforce consistent version set |
| Deleted keys never returned | Deletion marked in LSM; skipped by iterators |
| Compaction doesn't lose data | Old files retained until snapshot releases them |

## Error Categories

| Error | When | Recovery |
|---|---|---|
| WriteError | Disk full or IO failure | Check disk space; may need to resize or migrate |
| ReadError (corruption) | Checksum failed; LSM block corrupted | Restore from backup; mark snapshot as corrupted |
| SnapshotLimitError | >100 concurrent snapshots | Close old snapshots; pool pattern recommended |
| RangeError | Invalid key bounds (start > end) | Validate range in caller; swap if needed |
| CompactionError | Background compaction failed | Log error; user can retry via explicit compact() call |

## Performance Commitments (Release Gates)

| Gate | Latency | Concurrency | Write Level | Test |
|---|---|---|---|---|
| GATE-STOR-01 | put() ≤50 µs | 64 writers | BatchWrite | bench_storage_release_gates.cpp |
| GATE-STOR-02 | get() ≤20 µs | 64 readers | Snapshot | bench_storage_read_gates.cpp |
| GATE-STOR-03 | delete() ≤30 µs | 64 writers | BatchWrite | bench_storage_delete_gates.cpp |
| GATE-STOR-04 | scan() ≤5 µs/item | Concurrent | Snapshot | bench_storage_scan_gates.cpp |

## API Stability & Versioning

| Item | Status | Notes |
|---|---|---|
| IStorageEngine interface | Public v1.x | Stable; new methods = version break |
| WriteOptions/ReadOptions | Public v1.x | Frozen; new flags must use default |
| LSM tree format | Internal | May change; not part of contract |
| RocksDB version | Internal | Updated by maintenance; external API unchanged |
| Snapshot ID format | Internal | String-based; opaque to caller |

---

**Zuletzt geprueft (Storage contracts):** 2026-08-03
