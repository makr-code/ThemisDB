# MVCC History & Conflict Layer

**Version:** 1.1  
**Status:** Production  
**Date:** 2026-02-25

## Overview

This document describes the design of the atomic History/Conflict layer integrated with the existing RocksDB TransactionDB MVCC infrastructure in ThemisDB.

The layer provides:
1. **Atomic history writes** – every write to a core "live" key (Entities, File Manifests) also writes an immutable history record within the same RocksDB transaction.
2. **Conflict artifacts** – on commit failure (write-write conflict, busy, timeout) individual `ConflictRecord` entries and one `ConflictSet` are persisted in a separate keyspace.  Structured conflict info is returned to callers.
3. **Time-travel reads** – `getAtTimestamp(key, ts)` and `listVersions(key)` APIs over the history keyspace.
4. **Conflict resolution plumbing** – `ConflictManager` stores and retrieves conflict artifacts; resolution strategy is a follow-up concern.

---

## 1. Keyspace Layout (prefix-based, no new Column Families)

All keys in the history/conflict namespaces use printable prefix strings that are distinct from existing live-key prefixes (`entity:`, `graph:`, `vector:`, `occ:ver:` etc.).

| Namespace | Prefix | Key Format | Notes |
|-----------|--------|------------|-------|
| Live keys | *(existing)* | `entity:<table>:<pk>` etc. | Unchanged |
| History | `hist:` | `hist:<base_key>\x00<8-byte-HLC-ts-BE>` | Per-version immutable entries |
| Conflict records | `conflict:` | `conflict:<conflict_id>` | One entry per key per failed commit |
| Conflict sets | `conflictset:` | `conflictset:<conflict_set_id>` | One entry per failed commit attempt |

### History key encoding

```
hist:<base_key>\x00<8-byte-big-endian-HLC-timestamp>
```

- Prefix `hist:` (5 bytes) distinguishes history from live keys
- `<base_key>` is the exact live-key string (e.g., `entity:users:user1`)
- `\x00` separates the logical key from the timestamp
- `<8-byte-big-endian-HLC-timestamp>` ensures lexicographic order = chronological order

This means `db.scanPrefix("hist:<base_key>\x00")` returns all history versions of that key in ascending timestamp order.

### Conflict key encoding

```
conflict:<conflict_id>
```

- `<conflict_id>` is a string built from the HLC timestamp at detection time, e.g. `1234567890_5` (physical_ms_logical)
- One record is written **per affected key** per failed commit.

### Conflict set key encoding

```
conflictset:<conflict_set_id>
```

- `<conflict_set_id>` is generated from the same HLC clock as the individual conflict IDs.
- One record is written **per failed commit attempt**, grouping all individual conflict record IDs.
- The `conflictset:` prefix is distinct from `conflict:` (the 9th character is `s` vs `:`), so scans of either namespace do not overlap.

---

## 2. Record Formats

### HistoryRecord (JSON, version 1)

```json
{
  "v": 1,
  "base_key": "entity:users:user1",
  "ts": 1234567890123,
  "op": "put",
  "value": "<hex-encoded bytes>",
  "txn_id": 42
}
```

Fields:
- `v` – format version (always 1 for now)
- `base_key` – the logical live key
- `ts` – the HLC timestamp value (uint64, encoded as the raw 64-bit integer)
- `op` – operation type: `"put"` or `"del"`
- `value` – hex-encoded bytes of the written value (empty string for `"del"`)
- `txn_id` – originating transaction ID (0 if unknown)

### ConflictRecord (JSON, version 1)

```json
{
  "v": 1,
  "conflict_id": "1234567890123_5",
  "base_key": "entity:users:user1",
  "detected_at": 1234567890456,
  "txn_id": 42,
  "base_hex": "<hex-encoded bytes or empty>",
  "ours_hex": "<hex-encoded bytes or empty>",
  "theirs_hex": "<hex-encoded bytes or empty>",
  "type": "busy"
}
```

Fields:
- `v` – format version
- `conflict_id` – globally unique ID (HLC timestamp string at detection time)
- `base_key` – the conflicting live key
- `detected_at` – HLC timestamp when the conflict was detected
- `txn_id` – transaction ID that encountered the conflict
- `base_hex` – hex-encoded bytes of the value at transaction start (snapshot read, captured once per key)
- `ours_hex` – hex-encoded bytes this transaction tried to write
- `theirs_hex` – hex-encoded bytes of the value committed by the conflicting transaction
- `type` – conflict classification: `"busy"` (write-write / lock contention), `"timeout"` (lock wait timeout), `"try_again"` (transient, retry), `"commit_error"` (other)

### ConflictSet (JSON, version 1)

```json
{
  "v": 1,
  "conflict_set_id": "1234567890789_9",
  "detected_at": 1234567890789,
  "txn_id": 42,
  "conflict_record_ids": ["1234567890123_5", "1234567890124_6"],
  "affected_keys": ["entity:users:user1", "entity:users:user2"]
}
```

Fields:
- `v` – format version
- `conflict_set_id` – globally unique ID for this failed commit attempt
- `detected_at` – HLC timestamp when the set was created
- `txn_id` – transaction ID that encountered the conflict
- `conflict_record_ids` – list of individual ConflictRecord IDs created for this commit attempt
- `affected_keys` – list of all keys involved in the conflict

---

## 3. Transactional Write Flow

### Entity Put (putEntity)

1. Check for SSI predicate conflict
2. If first write to this key in the transaction: read current live value via `mvcc_txn_->get(key)` → capture as `base_values_[key]` (**captured only once per key**)
3. Update `our_values_[key]` to the current serialized value
4. Write live key: `mvcc_txn_->put(key, value_bytes)`
5. Write history key: `history_mgr_->recordPut(*mvcc_txn_, key, value_bytes, txn_id)` – returns `std::optional<HLCTimestamp>`; if `nullopt`, the overall operation returns an error
6. Both writes are in the same `TransactionWrapper`, guaranteeing atomicity

### Entity Erase (eraseEntity)

1. Check for SSI predicate conflict
2. If first erase/write to this key in the transaction: read current live value → capture as `base_values_[key]` (**once per key**)
3. Update `our_values_[key]` to empty (deletion)
4. Delete live key: `mvcc_txn_->del(key)`
5. Write tombstone: `history_mgr_->recordDel(*mvcc_txn_, key, txn_id)` – returns `std::optional<HLCTimestamp>`; failure propagated as error

---

## 4. Base/Ours/Theirs Capture Semantics

| Value | How captured | When captured |
|-------|-------------|---------------|
| `base` | `mvcc_txn_->get(key)` before writing | **First** call to putEntity/eraseEntity for this key in the transaction; subsequent writes do NOT update base |
| `ours` | The serialized value being written | Updated on **every** call to putEntity/eraseEntity for this key (latest intended value) |
| `theirs` | `db_.get(key)` directly after commit failure | At commit failure handling |

The `mvcc_txn_->get(key)` call reads from the transaction's snapshot, giving the value as of the transaction's start point.  Because `base` is only captured on the **first** write, it correctly represents the pre-transaction state even if the same key is written multiple times within the same transaction (read-your-writes would otherwise corrupt the base).

---

## 5. Commit Failure → Conflict Artifact Flow

When `mvcc_txn_->commit()` fails:

1. Classify the failure type by inspecting `mvcc_txn_->getLastCommitFailureType()`:
   - `Busy` → type = `"busy"` (write-write conflict or lock contention)
   - `TimedOut` → type = `"timeout"`
   - `TryAgain` → type = `"try_again"`
   - Other → type = `"commit_error"`

2. For each key tracked in the transaction's write set:
   a. Read `theirs = db_.get(key)` (current committed value)
   b. Retrieve `base = base_values_[key]` and `ours = our_values_[key]`
   c. Build `ConflictRecord{..., type}` and persist via `ConflictManager::storeConflict()`

3. Build a `ConflictSet` listing all per-key ConflictRecord IDs, and persist via `ConflictManager::storeConflictSet()`.

4. Return `Status::Conflict(message, conflict_set_id, affected_keys)` to the caller.
   - `Status::conflict_id` = `conflict_set_id` (backwards-compatible: callers that only inspect `conflict_id` get the set ID)
   - `Status::conflict_set_id` = same value (explicitly named field for new callers)

The writes use a non-transactional write path and will always succeed (conflict keys include a unique HLC timestamp).

---

## 6. Time-Travel Read Flow

### getAtTimestamp(base_key, ts)

1. Construct history key prefix: `"hist:" + base_key + "\x00"`
2. Seek in RocksDB to `"hist:" + base_key + "\x00" + encode(ts+1)` (exclusive upper bound)
3. Step backward with `Prev()`
4. If the found key has the correct prefix and decoded ts ≤ requested ts, deserialize and return the `HistoryRecord`

### listVersions(base_key)

1. Construct history key prefix: `"hist:" + base_key + "\x00"`
2. Use `db_.scanPrefix(prefix, callback)` to iterate all history entries in ascending timestamp order
3. Deserialize and collect `HistoryRecord` entries

---

## 7. Retention/GC Strategy

The history keyspace participates in garbage collection through `MVCCStore::gcAllBefore(min_ts)`:

- History keys follow the same encoding as MVCC versioned keys (`<base>\x00<ts>`)
  but with a `hist:` prefix on `<base>`.
- The existing `gcVersionsBefore(key, min_ts)` logic applies to the history base-key `"hist:<live_key>"`.
- `GCOptions::min_versions_to_keep` (default 1) ensures at least one history version is retained.

Conflict records and conflict sets are not garbage collected automatically; operators can remove them by key prefix scan.

---

## 8. Backwards Compatibility & Migration

- **No new Column Families** are introduced in this version. All keys coexist in the default CF.
- The `hist:`, `conflict:`, and `conflictset:` prefixes are not used by any existing code.
- Existing databases can be upgraded without any migration step; history records will accumulate only for writes made after the upgrade.
- The `HistoryManager` and `ConflictManager` are optional: if not injected into `Transaction`, the system falls back to the previous behavior (no history writes, no conflict artifacts).
- `Status::conflict_id` now holds the `conflict_set_id` when a conflict set is created (backwards compatible – callers that only inspect `conflict_id` still get a valid, unique ID they can use to look up the set).
- `HistoryManager::recordPut`/`recordDel` now return `std::optional<HLCTimestamp>` (was `HLCTimestamp`); callers must check for `std::nullopt` and propagate the error.

---

## 9. Code Structure

```
include/storage/history_manager.h    – HistoryRecord, ConflictRecord, ConflictSet, HistoryManager, ConflictManager
src/storage/history_manager.cpp      – Implementations
include/storage/rocksdb_wrapper.h    – TransactionWrapper::CommitFailureType, getLastCommitFailureType()
src/storage/rocksdb_wrapper.cpp      – Track commit failure type in commit()
include/storage/mvcc_store.h         – putInTxn(), delInTxn()
src/storage/mvcc_store.cpp           – Implementations
include/transaction/transaction_manager.h  – Extended Status (conflict_set_id); Transaction gains HistoryManager pointer
src/transaction/transaction_manager.cpp   – putEntity/eraseEntity write history; commit() writes ConflictRecord+ConflictSet
tests/test_mvcc_history.cpp          – Tests for all new functionality
```


## Overview

This document describes the design of the atomic History/Conflict layer integrated with the existing RocksDB TransactionDB MVCC infrastructure in ThemisDB.

The layer provides:
1. **Atomic history writes** – every write to a core "live" key (Entities, File Manifests) also writes an immutable history record within the same RocksDB transaction.
2. **Conflict artifacts** – on commit failure (write-write conflict, busy, timeout) a `ConflictRecord` is persisted in a separate keyspace and structured conflict info is returned to callers.
3. **Time-travel reads** – `getAtTimestamp(key, ts)` and `listVersions(key)` APIs over the history keyspace.
4. **Conflict resolution plumbing** – `ConflictManager` stores and retrieves conflict artifacts; resolution strategy is a follow-up concern.

---

## 1. Keyspace Layout (prefix-based, no new Column Families)

All keys in the history/conflict namespaces use non-printable prefix bytes to avoid collisions with existing live-key prefixes (`entity:`, `graph:`, `vector:`, `occ:ver:` etc.).

| Namespace | Prefix | Key Format | Notes |
|-----------|--------|------------|-------|
| Live keys | *(existing)* | `entity:<table>:<pk>` etc. | Unchanged |
| History | `hist:` | `hist:<base_key>\x00<8-byte-HLC-ts-BE>` | Per-version immutable entries |
| Conflicts | `conflict:` | `conflict:<conflict_id>` | One entry per detected conflict |

### History key encoding

```
hist:<base_key>\x00<8-byte-big-endian-HLC-timestamp>
```

- Prefix `hist:` (6 bytes) distinguishes history from live keys
- `<base_key>` is the exact live-key string (e.g., `entity:users:user1`)
- `\x00` separates the logical key from the timestamp
- `<8-byte-big-endian-HLC-timestamp>` ensures lexicographic order = chronological order

This means `db.scanPrefix("hist:<base_key>\x00")` returns all history versions of that key in ascending timestamp order.

### Conflict key encoding

```
conflict:<conflict_id>
```

- `<conflict_id>` is a string built from the HLC timestamp at detection time, e.g. `1234567890.5` (physical_ms.logical)

---

## 2. Record Formats

### HistoryRecord (JSON, version 1)

```json
{
  "v": 1,
  "base_key": "entity:users:user1",
  "ts": 1234567890123,
  "op": "put",
  "value": "<hex-encoded bytes>",
  "txn_id": 42
}
```

Fields:
- `v` – format version (always 1 for now)
- `base_key` – the logical live key
- `ts` – the HLC timestamp value (uint64, encoded as the raw 64-bit integer)
- `op` – operation type: `"put"` or `"del"`
- `value` – hex-encoded bytes of the written value (empty string for `"del"`)
- `txn_id` – originating transaction ID (0 if unknown)

### ConflictRecord (JSON, version 1)

```json
{
  "v": 1,
  "conflict_id": "1234567890123.5",
  "base_key": "entity:users:user1",
  "detected_at": 1234567890456,
  "txn_id": 42,
  "base_hex": "<hex-encoded bytes or empty>",
  "ours_hex": "<hex-encoded bytes or empty>",
  "theirs_hex": "<hex-encoded bytes or empty>"
}
```

Fields:
- `v` – format version
- `conflict_id` – globally unique ID (HLC timestamp string at detection time)
- `base_key` – the conflicting live key
- `detected_at` – HLC timestamp when the conflict was detected
- `txn_id` – transaction ID that encountered the conflict
- `base_hex` – hex-encoded bytes of the value at transaction start (snapshot read)
- `ours_hex` – hex-encoded bytes this transaction tried to write
- `theirs_hex` – hex-encoded bytes of the value committed by the conflicting transaction

---

## 3. Transactional Write Flow

### Entity Put (putEntity)

1. Read current live value via `mvcc_txn_->get(key)` → capture as `base`
2. Obtain HLC timestamp: `ts = clock_->now()`
3. Serialize entity → `value_bytes`
4. Write live key: `mvcc_txn_->put(key, value_bytes)` (existing)
5. Write history key: `mvcc_txn_->put(HistoryManager::historyKey(key, ts), serialize(HistoryRecord{...}))`
6. Track `{key, value_bytes, base_bytes}` in per-transaction maps
7. Both writes are in the same `TransactionWrapper`, guaranteeing atomicity

### Entity Erase (eraseEntity)

1. Read current live value → capture as `base`
2. Obtain HLC timestamp: `ts = clock_->now()`
3. Delete live key: `mvcc_txn_->del(key)` (existing)
4. Write tombstone history key: `mvcc_txn_->put(HistoryManager::historyKey(key, ts), serialize(HistoryRecord{op:"del",...}))`
5. Track in per-transaction maps

---

## 4. Base/Ours/Theirs Capture

| Value | How captured | When |
|-------|-------------|------|
| `base` | `mvcc_txn_->get(key)` before writing | At putEntity/eraseEntity call time |
| `ours` | The serialized value being written | At putEntity/eraseEntity call time |
| `theirs` | `db_.get(key)` directly after commit failure | At commit failure handling |

The `mvcc_txn_->get(key)` call reads from the transaction's snapshot, giving the value as of the transaction's start point.

---

## 5. Commit Failure → Conflict Artifact Flow

When `mvcc_txn_->commit()` fails:

1. For each key tracked in the transaction's write set:
   a. Read `theirs = db_.get(key)` (current committed value)
   b. Retrieve `base = base_values_[key]` and `ours = our_values_[key]`
   c. Build `ConflictRecord{conflict_id, base_key=key, detected_at=clock_->now(), txn_id, base, ours, theirs}`
2. Persist each conflict record using a **fresh write batch** (not a new transaction, to avoid another conflict):  
   `db_.put(ConflictManager::conflictKey(conflict_id), serialize(record))`
3. Return `Status::Conflict(message, conflict_id, affected_keys)` to the caller

The fresh write batch is non-transactional and will always succeed (there are no competing writers for conflict keys since they include a unique timestamp).

---

## 6. Time-Travel Read Flow

### getAtTimestamp(base_key, ts)

1. Construct history key prefix: `"hist:" + base_key + "\x00"`
2. Seek in RocksDB to `"hist:" + base_key + "\x00" + encode(ts+1)` (exclusive upper bound)
3. Step backward with `Prev()`
4. If the found key has the correct prefix and decoded ts ≤ requested ts, deserialize and return the `HistoryRecord`

### listVersions(base_key)

1. Construct history key prefix: `"hist:" + base_key + "\x00"`
2. Use `db_.scanPrefix(prefix, callback)` to iterate all history entries in ascending timestamp order
3. Deserialize and collect `HistoryRecord` entries

---

## 7. Retention/GC Strategy

The history keyspace participates in garbage collection through `MVCCStore::gcAllBefore(min_ts)`:

- History keys follow the same encoding as MVCC versioned keys (`<base>\x00<ts>`)
  but with a `hist:` prefix on `<base>`.
- The existing `gcVersionsBefore(key, min_ts)` logic applies to the history base-key `"hist:<live_key>"`.
- `GCOptions::min_versions_to_keep` (default 1) ensures at least one history version is retained.

Conflict records are not garbage collected automatically; operators can remove them by key prefix scan.

---

## 8. Backwards Compatibility & Migration

- **No new Column Families** are introduced in this version. All keys coexist in the default CF.
- The `hist:` and `conflict:` prefixes are not used by any existing code.
- Existing databases can be upgraded without any migration step; history records will accumulate only for writes made after the upgrade.
- The `HistoryManager` and `ConflictManager` are optional: if not injected into `Transaction`, the system falls back to the previous behavior (no history writes, no conflict artifacts).
- The `Status::conflict_id` field is an empty string in the legacy code path.

---

## 9. Code Structure

```
include/storage/history_manager.h    – HistoryRecord, ConflictRecord, HistoryManager, ConflictManager
src/storage/history_manager.cpp      – Implementations
include/storage/mvcc_store.h         – Added: putInTxn(), delInTxn()
src/storage/mvcc_store.cpp           – Implementations
include/transaction/transaction_manager.h  – Extended Status; Transaction gains HistoryManager pointer
src/transaction/transaction_manager.cpp   – putEntity/eraseEntity write history; commit() writes ConflictRecord
tests/test_mvcc_history.cpp          – Tests for all new functionality
```
