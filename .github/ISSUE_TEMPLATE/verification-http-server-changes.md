---
title: "[VERIFICATION] Review Recent http_server.cpp Changes for RocksDBWrapper Abstraction Compliance"
labels: 
  - "priority:P1"
  - "type:verification"
  - "area:architecture"
  - "area:storage"
  - "effort:small"
assignees: []
---

# Verification Task: http_server.cpp RocksDBWrapper Abstraction Compliance

## Overview

This issue template provides a checklist to verify that all recent changes to `http_server.cpp` (from the last 10-12 PRs) correctly use the `RocksDBWrapper` abstraction layer instead of direct RocksDB API calls.

## Background

ThemisDB uses the `RocksDBWrapper` abstraction to maintain storage engine independence. Direct usage of `rocksdb::TransactionDB*` or `rocksdb::ColumnFamilyHandle*` violates this architectural principle and must be refactored.

**Reference:** Issue #[PITR_REFACTOR_ISSUE_NUMBER] - PITR Implementation RocksDBWrapper Abstraction

## Scope

**Files to Review:**
- `src/server/http_server.cpp`
- `include/server/http_server.h`
- Any new API handler files created in recent PRs

**Time Period:** Last 10-12 merged PRs that touched these files

## Verification Checklist

### Phase 1: Identify Recent Changes

- [ ] List all PRs from the last 2-3 months that modified http_server.cpp
- [ ] Identify which PRs introduced new storage-related features
- [ ] Document PR numbers and titles for reference

### Phase 2: Code Review - Direct RocksDB Usage

Check for the following anti-patterns:

- [ ] **Direct TransactionDB Pointers**
  ```cpp
  // ❌ INCORRECT
  rocksdb::TransactionDB* db = ...;
  
  // ✅ CORRECT
  RocksDBWrapper& db = ...;
  ```

- [ ] **Direct ColumnFamilyHandle Pointers**
  ```cpp
  // ❌ INCORRECT
  rocksdb::ColumnFamilyHandle* cf = ...;
  
  // ✅ CORRECT
  // Use RocksDBWrapper methods or default column family
  ```

- [ ] **Raw RocksDB API Calls**
  ```cpp
  // ❌ INCORRECT
  rocksdb::Status s = raw_db->Put(...);
  
  // ✅ CORRECT
  bool success = db_wrapper.put(...);
  ```

### Phase 3: Specific Areas to Check

#### 3.1 Component Initialization

- [ ] SnapshotManager initialization uses `RocksDBWrapper&` reference
  ```cpp
  // ✅ CORRECT (line ~321)
  snapshot_manager_ = std::make_unique<SnapshotManager>(*storage_, *changefeed_);
  ```

- [ ] PITRManager initialization uses `RocksDBWrapper*` pointer
  ```cpp
  // ✅ CORRECT
  pitr_manager_ = std::make_unique<PITRManager>(
      storage_.get(), changefeed_.get(), snapshot_manager_.get()
  );
  ```

- [ ] Changefeed initialization (currently uses raw DB - documented exception)
  ```cpp
  // Note: Changefeed still uses raw DB pointer - future refactoring target
  changefeed_ = std::make_unique<Changefeed>(storage_->getRawDB(), nullptr);
  ```

#### 3.2 API Handler Initialization

Check all API handler constructors for correct abstraction usage:

- [ ] TimeSeriesApiHandler - No direct RocksDB usage
- [ ] CacheApiHandler - No direct RocksDB usage  
- [ ] SnapshotApiHandler - No direct RocksDB usage
- [ ] PITRApiHandler (if exists) - No direct RocksDB usage
- [ ] [Add other handlers as needed]

#### 3.3 Storage Operations

- [ ] All PUT operations use `RocksDBWrapper::put()`
- [ ] All GET operations use `RocksDBWrapper::get()`
- [ ] All DELETE operations use `RocksDBWrapper::del()` or `RocksDBWrapper::erase()`
- [ ] Batch operations use `WriteBatchWrapper` or `WriteBatchWithIndexWrapper`
- [ ] Transactions use `TransactionWrapper`

#### 3.4 Column Family Usage

- [ ] Column families obtained via `RocksDBWrapper::getOrCreateColumnFamily()`
- [ ] Column family handles properly managed (no manual delete)
- [ ] Default column family used when appropriate

### Phase 4: Test Coverage

- [ ] All new features have integration tests
- [ ] Tests use RocksDBWrapper abstraction
- [ ] No tests directly instantiate `rocksdb::TransactionDB`
- [ ] Mock-friendly test structure using RocksDBWrapper

### Phase 5: Documentation

- [ ] Architecture decisions documented
- [ ] API handler documentation references RocksDBWrapper
- [ ] No documentation showing direct RocksDB usage patterns
- [ ] Migration guides updated if applicable

## Anti-Pattern Detection Script

Run the following commands to detect potential violations:

```bash
# Check for direct rocksdb:: usage in http_server files
grep -n "rocksdb::TransactionDB\*" src/server/http_server.cpp include/server/http_server.h
grep -n "rocksdb::ColumnFamilyHandle\*" src/server/http_server.cpp include/server/http_server.h

# Check for raw RocksDB API calls
grep -n "->Put\|->Get\|->Delete\|->CreateColumnFamily" src/server/http_server.cpp

# List recent changes to http_server.cpp
git log --oneline --since="3 months ago" -- src/server/http_server.cpp
```

## Known Exceptions

The following components are documented exceptions that still use raw RocksDB pointers:

1. **Changefeed** - Uses `rocksdb::TransactionDB*` and `rocksdb::ColumnFamilyHandle*`
   - Status: Planned for future refactoring
   - Tracking Issue: [Create separate issue]

2. **Direct RocksDB Access for Advanced Features**
   - Some low-level operations may require `getRawDB()` access
   - Must be justified and documented in code comments
   - Should be minimal and isolated

## Acceptance Criteria

- [ ] All recent PRs reviewed for RocksDBWrapper compliance
- [ ] No unjustified direct RocksDB usage found
- [ ] All violations documented with remediation plan
- [ ] Tests confirm all storage operations work correctly
- [ ] Documentation updated to reflect correct patterns

## Remediation Template

For any violations found, use this template:

```markdown
### Violation #N: [Brief Description]

**Location:** `[file]:[line]`
**PR:** #[PR_NUMBER]
**Pattern:** [Direct TransactionDB/ColumnFamilyHandle/API Call]

**Current Code:**
```cpp
// Violating code here
```

**Correct Implementation:**
```cpp
// Corrected code here
```

**Priority:** [P0/P1/P2]
**Estimated Effort:** [Small/Medium/Large]
```

## Related Issues

- #[PITR_REFACTOR_ISSUE_NUMBER] - PITR Implementation RocksDBWrapper Abstraction
- [Add other related issues]

## References

- `include/storage/rocksdb_wrapper.h` - RocksDBWrapper API
- `docs/ROCKSDB_WRAPPER_AUDIT_REPORT.md` - Architecture documentation
- `BASEENTITY_PRINCIPLE.md` - ThemisDB architectural principles

---

**Effort:** Small | **Priority:** P1 | **Type:** Verification | **Area:** Architecture, Storage
