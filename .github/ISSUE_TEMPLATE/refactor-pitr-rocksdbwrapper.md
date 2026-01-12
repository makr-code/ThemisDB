---
title: "[REFACTOR] PITR Implementation violates RocksDBWrapper Abstraction"
labels: 
  - "priority:P0"
  - "type:refactoring"
  - "area:storage"
  - "breaking-change"
  - "tech-debt"
assignees: []
---

# Problem Description

The Phase 3 PITR (Point-in-Time Recovery) implementation in the `develop` branch directly uses `rocksdb::TransactionDB*` and `rocksdb::ColumnFamilyHandle*`, bypassing the `RocksDBWrapper` abstraction layer. This violates ThemisDB's core architectural principle of storage engine independence.

## Current Implementation (Incorrect)

**File:** `include/transaction/snapshot_manager.h`

```cpp
// Constructor uses direct RocksDB pointers
SnapshotManager(rocksdb::TransactionDB* db,
                rocksdb::ColumnFamilyHandle* cf,
                Changefeed* changefeed)
```

**Problems:**
1. Direct dependency on RocksDB implementation details
2. Breaks abstraction layer established for storage engine independence
3. Prevents future migration to alternative storage engines (TiKV, FoundationDB, etc.)
4. Inconsistent with other ThemisDB components that use RocksDBWrapper
5. Creates tight coupling with RocksDB TransactionDB API

## Correct Implementation (Reference: Phase 1)

**File:** Phase 1 Named Snapshots PR implementation

```cpp
// Constructor uses RocksDBWrapper abstraction
SnapshotManager(RocksDBWrapper& db, Changefeed& changefeed)
```

**Benefits:**
1. ✅ Maintains storage engine independence
2. ✅ Follows established ThemisDB architectural principles
3. ✅ Enables future storage engine flexibility
4. ✅ Consistent API across all storage-dependent components
5. ✅ Better testability (can mock RocksDBWrapper)

## Why RocksDBWrapper Exists

The `RocksDBWrapper` abstraction was specifically designed to:
- Allow potential future replacement of RocksDB with other storage engines
- Provide a consistent API across all ThemisDB components
- Centralize storage-layer changes
- Improve testability through mocking

**Reference:** `include/storage/rocksdb_wrapper.h`

## Impact

**Severity:** 🔴 Critical - Architectural violation
**Scope:** PITR implementation in develop branch
**Components Affected:**
- `include/transaction/snapshot_manager.h`
- `src/transaction/snapshot_manager.cpp`
- `include/storage/pitr_manager.h` (if exists)
- `src/storage/pitr_manager.cpp` (if exists)
- Any PITR-related integration code

## Required Changes

### 1. Refactor SnapshotManager Constructor

**Current (Incorrect):**
```cpp
SnapshotManager(rocksdb::TransactionDB* db,
                rocksdb::ColumnFamilyHandle* cf,
                Changefeed* changefeed);
```

**Required (Correct):**
```cpp
SnapshotManager(RocksDBWrapper& db, Changefeed& changefeed);
```

### 2. Replace Direct RocksDB Calls

Replace all direct RocksDB operations with RocksDBWrapper methods:
- `db_->Put()` → `db_.put()`
- `db_->Get()` → `db_.get()`
- `db_->Delete()` → `db_.erase()`
- `cf_->...` → Use default column family or RocksDBWrapper methods

### 3. Update PITR Manager

If `PITRManager` also uses direct RocksDB access, refactor it similarly.

### 4. Update Integration Points

Update all code that instantiates SnapshotManager to pass RocksDBWrapper instead of raw RocksDB pointers.

## Acceptance Criteria

- [ ] SnapshotManager uses `RocksDBWrapper&` instead of `rocksdb::TransactionDB*`
- [ ] No direct RocksDB API calls in snapshot/PITR code
- [ ] All PITR functionality continues to work correctly
- [ ] Tests pass with refactored implementation
- [ ] Code follows established ThemisDB architectural patterns
- [ ] Documentation updated to reflect correct architecture

## Related Issues

- Phase 1 Named Snapshots PR: Correct implementation using RocksDBWrapper
- Merge conflict between Phase 1 and Phase 3 implementations

## Context

This issue was identified during the merge of Phase 1 Named Snapshots into develop branch. The Phase 1 implementation correctly uses RocksDBWrapper, while the Phase 3 PITR implementation in develop uses direct RocksDB access. This architectural inconsistency must be resolved.

**Priority Justification:** P0 (Critical) because:
1. Violates core architectural principle
2. Creates technical debt
3. Blocks clean integration of Phase 1
4. Prevents future storage engine flexibility
5. Sets poor precedent for future development

## Recommended Approach

1. Review Phase 1 Named Snapshots implementation as reference
2. Refactor PITR code to use RocksDBWrapper
3. Ensure all tests pass
4. Update documentation
5. Code review with architecture focus

## Technical Debt

If not fixed immediately, this creates:
- Architectural inconsistency across codebase
- Increased maintenance burden
- Difficult future migration path
- Risk of other components copying this anti-pattern

---

**Note:** The RocksDBWrapper abstraction is not optional - it is a fundamental architectural decision that must be respected throughout the codebase.
