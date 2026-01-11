---
name: Git-like Features - Phase 2: Diff API
about: Implement Structured Diff API for MVCC system
title: '[FEATURE] Phase 2: Diff API (Structured Diff)'
labels: 'enhancement, mvcc, database, api, priority-medium-high, phase-2'
assignees: ''
---

# Phase 2: Diff API (Structured Diff)

## 📋 Summary

Implement a structured Diff API for ThemisDB's MVCC system, enabling detailed comparison of database states between two time points for audit reports, debugging, and compliance tracking.

## 🎯 Objectives

### Primary Goal
Implement a DiffEngine that enables:
1. **Structured diffs** between any two database states
2. **Filtering capabilities** by table, entity type, key prefix
3. **Pagination support** for large diff results
4. **Performance optimization** (<100ms for 10K changes)

### Success Criteria
- [x] Diff between arbitrary sequence numbers
- [x] Diff between named tags
- [x] Diff between timestamps
- [x] Filtering works (table, key prefix)
- [x] Pagination works correctly
- [x] Performance: <100ms for 10K changes, <1s for 100K changes
- [x] Test Coverage ≥ 95%
- [x] Documentation complete (EN + DE)

---

## 🏗️ Architecture Overview

```
┌──────────────────────────────────────┐
│      REST API Layer                  │
│  GET /api/v1/diff                    │
│  ?from=seq_100&to=seq_200            │
│  ?from=tag:v1.0&to=tag:v2.0          │
│  ?from=2026-01-01&to=2026-01-11      │
└──────────────┬───────────────────────┘
               │
┌──────────────▼───────────────────────┐
│      DiffEngine                      │
│  - computeDiff()                     │
│  - computeDiffByTag()                │
│  - computeDiffByTimestamp()          │
└──────────────┬───────────────────────┘
               │
     ┌─────────┴──────────┐
     │                    │
┌────▼────┐        ┌──────▼──────┐
│Changefeed│        │SnapshotMgr  │
│          │        │             │
└──────────┘        └─────────────┘
```

---

## 📦 Implementation Tasks

### Sprint 3 (Week 1-2): Core Implementation

#### Task 2.1: DiffEngine Header (1 day)
- **File:** `include/analytics/diff_engine.h`
- **Description:** Define structures and methods

**Data Structures:**
```cpp
class DiffEngine {
public:
    enum class ChangeType {
        ADDED,
        MODIFIED,
        DELETED
    };
    
    struct ChangeRecord {
        ChangeType type;
        std::string table;
        std::string primary_key;
        std::optional<std::string> old_value;  // For MODIFIED, DELETED
        std::optional<std::string> new_value;  // For ADDED, MODIFIED
        uint64_t sequence;
        int64_t timestamp_ms;
    };
    
    struct DiffStats {
        size_t added_count = 0;
        size_t modified_count = 0;
        size_t deleted_count = 0;
        size_t total_changes = 0;
        int64_t duration_ms = 0;
    };
    
    struct DiffResult {
        std::vector<ChangeRecord> added;
        std::vector<ChangeRecord> modified;
        std::vector<ChangeRecord> deleted;
        DiffStats stats;
        
        bool has_more = false;  // Pagination
        uint64_t next_sequence = 0;
    };
    
    struct DiffOptions {
        std::optional<std::string> table_filter;
        std::optional<std::string> key_prefix_filter;
        bool include_values = true;
        size_t limit = 1000;
        uint64_t offset_sequence = 0;
    };
    
    // Methods
    DiffResult computeDiff(uint64_t from_seq, uint64_t to_seq, 
                          const DiffOptions& options = {});
    DiffResult computeDiffByTag(const std::string& from_tag, 
                               const std::string& to_tag,
                               const DiffOptions& options = {});
    DiffResult computeDiffByTimestamp(int64_t from_ts, int64_t to_ts,
                                     const DiffOptions& options = {});
};
```

#### Task 2.2: DiffEngine Implementation (3-4 days)
- **File:** `src/analytics/diff_engine.cpp`
- **Description:** Implement diff computation logic

**Key Components:**
- Event processing from Changefeed
- Change categorization (Added/Modified/Deleted)
- Filtering logic (table, key prefix)
- Pagination support
- Performance optimization

**Algorithm:**
1. Query changefeed for events in range [from_seq, to_seq]
2. Apply filters (table, key prefix)
3. Categorize changes by type
4. Group by primary key to detect modifications
5. Apply pagination
6. Compute statistics

#### Task 2.3: REST API Handler (1-2 days)
- **Files:**
  - `include/server/diff_api_handler.h`
  - `src/server/diff_api_handler.cpp`
- **Endpoint:** `GET /api/v1/diff`

**Query Parameters:**
- `from` - Start point (sequence, tag:name, ISO timestamp)
- `to` - End point (sequence, tag:name, ISO timestamp)
- `table` - Optional table filter
- `key_prefix` - Optional key prefix filter
- `include_values` - Include actual values (default: true)
- `limit` - Max results (default: 1000, max: 10000)
- `offset` - Pagination offset sequence

**Response Format:**
```json
{
  "added": [
    {
      "table": "users",
      "primary_key": "user_123",
      "new_value": {"name": "John", "email": "john@example.com"},
      "sequence": 12350,
      "timestamp_ms": 1704985200000
    }
  ],
  "modified": [
    {
      "table": "users",
      "primary_key": "user_456",
      "old_value": {"name": "Jane", "email": "jane@old.com"},
      "new_value": {"name": "Jane", "email": "jane@new.com"},
      "sequence": 12351,
      "timestamp_ms": 1704985201000
    }
  ],
  "deleted": [
    {
      "table": "users",
      "primary_key": "user_789",
      "old_value": {"name": "Bob", "email": "bob@example.com"},
      "sequence": 12352,
      "timestamp_ms": 1704985202000
    }
  ],
  "stats": {
    "added_count": 1,
    "modified_count": 1,
    "deleted_count": 1,
    "total_changes": 3,
    "duration_ms": 45
  },
  "has_more": false,
  "next_sequence": null
}
```

#### Task 2.4: Unit Tests (2 days)
- **File:** `tests/test_diff_engine.cpp`
- **Coverage Target:** ≥95%

**Test Cases:**
- [x] Diff by sequence numbers (empty range, single change, multiple changes)
- [x] Diff by tags (valid tags, invalid tags)
- [x] Diff by timestamps
- [x] Filtering by table (single table, multiple tables)
- [x] Filtering by key prefix
- [x] Include/exclude values option
- [x] Pagination (first page, middle page, last page, overflow)
- [x] Change categorization (added, modified, deleted)
- [x] Statistics accuracy
- [x] Performance (10K changes <100ms)

### Sprint 4 (Week 3-4): Performance & Documentation

#### Task 2.5: Performance Optimization (2 days)
- **Techniques:**
  - Caching frequently requested diffs
  - Parallel event processing
  - Memory-efficient streaming for large diffs
  - Index optimization for changefeed queries

**Performance Targets:**
- 10K changes: <100ms
- 100K changes: <1s
- 1M changes: <10s (with pagination)

#### Task 2.6: Benchmarks (1 day)
- **File:** `benchmarks/bench_diff_engine.cpp`

**Benchmark Scenarios:**
- Small diff (100 changes): <10ms
- Medium diff (10K changes): <100ms
- Large diff (100K changes): <1s
- Filtered diff (10K changes, 1 table): <50ms
- Paginated diff (100K changes, 1000 per page): <100ms per page

#### Task 2.7: Documentation (2 days)
- **Files:**
  - `docs/en/features/features_diff.md`
  - `docs/de/features/features_diff.md`

**Content:**
- User guide with examples
- API reference
- Query parameter documentation
- Best practices for large diffs
- Use cases (audit reports, debugging, compliance)

---

## 📊 Deliverables

| File | Lines | Description |
|------|-------|-------------|
| `include/analytics/diff_engine.h` | ~200 | Header file |
| `src/analytics/diff_engine.cpp` | ~600 | Implementation |
| `include/server/diff_api_handler.h` | ~50 | API header |
| `src/server/diff_api_handler.cpp` | ~400 | API implementation |
| `tests/test_diff_engine.cpp` | ~800 | Unit tests |
| `tests/test_diff_integration.cpp` | ~200 | Integration tests |
| `benchmarks/bench_diff_engine.cpp` | ~200 | Benchmarks |
| `docs/en/features/features_diff.md` | ~600 | EN documentation |
| `docs/de/features/features_diff.md` | ~600 | DE documentation |
| **TOTAL** | **~3,650** | **LOC** |

---

## 🎯 Acceptance Criteria

- [ ] All unit tests pass (≥95% coverage)
- [ ] All integration tests pass
- [ ] Performance benchmarks met
- [ ] REST API fully functional
- [ ] Pagination works correctly
- [ ] Filtering works as expected
- [ ] Documentation complete in English and German
- [ ] Code review approved
- [ ] OpenAPI spec updated

---

## 📈 Performance Requirements

- **10K changes:** <100ms
- **100K changes:** <1s
- **Filtering:** <50% overhead vs. unfiltered
- **Pagination:** <100ms per page (1000 items)
- **Memory:** Efficient streaming, no OOM for large diffs

---

## 🔗 Dependencies

- Phase 1 (Named Snapshots) completed
- Changefeed for event queries
- SnapshotManager for tag-based diffs
- HTTP server framework for REST API

---

## 📚 Related Documentation

- [Implementation Plan](../../docs/research/IMPLEMENTATION_PLAN_GIT_FEATURES.md)
- [Research Analysis](../../docs/research/GIT_LIKE_FEATURES_FOR_MVCC.md)
- [Phase 1: Named Snapshots](git_features_phase1_named_snapshots.md)

---

## 💡 Use Case Examples

### Audit Report
```bash
# What changed in the last 24 hours?
GET /api/v1/diff?from=2026-01-10T00:00:00Z&to=2026-01-11T00:00:00Z&table=users

# Response shows all user changes
```

### Debug Investigation
```bash
# What changed between two deployments?
GET /api/v1/diff?from=tag:before_deploy&to=tag:after_deploy

# Identify unexpected changes
```

### Compliance Tracking
```bash
# Show all changes to sensitive data
GET /api/v1/diff?from=tag:q4_2025&to=tag:q1_2026&table=financial_records&include_values=true
```

### Data Quality Check
```bash
# Detect anomalies in bulk imports
GET /api/v1/diff?from=12000&to=12500&table=products

# Verify import correctness
```

---

## ⏱️ Timeline

- **Duration:** 3-4 weeks (2 sprints)
- **Priority:** ⭐⭐ Medium-High
- **Risk:** 🟢 Low
- **Estimated LOC:** ~3,650 lines

---

## 👥 Team Assignment

- **Developer 1:** Core implementation (DiffEngine, API)
- **Developer 2 (QA):** Testing, performance optimization
- **Tech Writer:** Documentation (EN + DE)

---

## 🚀 Next Phase

After Phase 2 completion, proceed to:
- **Phase 3:** Point-in-Time Recovery (PITR with safety features)
