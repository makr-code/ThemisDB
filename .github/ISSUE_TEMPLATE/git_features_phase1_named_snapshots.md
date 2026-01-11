---
name: Git-like Features - Phase 1: Named Snapshots
about: Implement Named Snapshots (Semantic Tagging) for MVCC system
title: '[FEATURE] Phase 1: Named Snapshots (Semantic Tagging)'
labels: 'enhancement, mvcc, database, priority-high, phase-1'
assignees: ''
---

# Phase 1: Named Snapshots (Semantic Tagging)

## 📋 Summary

Implement Named Snapshots feature for ThemisDB's MVCC system, enabling semantic tagging of important database states for disaster recovery, compliance, and schema migration rollback scenarios.

## 🎯 Objectives

### Primary Goal
Implement a SnapshotManager that enables:
1. **Semantic tagging** of database states with human-readable names
2. **Persistent tag storage** in RocksDB
3. **REST API** for tag management (CRUD operations)
4. **Foundation** for Point-in-Time Recovery

### Success Criteria
- [x] SnapshotManager can create, read, update, delete tags
- [x] Tags are persistent (survive DB restart)
- [x] REST API functions correctly
- [x] Test Coverage ≥ 95%
- [x] Performance benchmarks met (CreateTag <1ms, GetTag <0.5ms, ListTags <10ms for 100 tags)
- [x] Documentation complete (EN + DE)
- [x] No memory leaks (valgrind check)

---

## 🏗️ Architecture Overview

```
┌──────────────────────────────────────┐
│      REST API Layer                  │
│  POST /api/v1/snapshots/tags         │
│  GET  /api/v1/snapshots/tags         │
│  DELETE /api/v1/snapshots/tags/:name │
└──────────────┬───────────────────────┘
               │
┌──────────────▼───────────────────────┐
│      SnapshotManager                 │
│  - createTag()                       │
│  - getTag()                          │
│  - listTags()                        │
│  - deleteTag()                       │
└──────────────┬───────────────────────┘
               │
┌──────────────▼───────────────────────┐
│   RocksDB Column Family: "tags"      │
│   Key: tags:{tag_name}               │
│   Value: {seq, ts, desc, user}       │
└──────────────────────────────────────┘
```

---

## 📦 Implementation Tasks

### Sprint 1 (Week 1-2): Core Implementation

#### Task 1.1: SnapshotManager Header (1 day)
- **File:** `include/transaction/snapshot_manager.h`
- **Description:** Define structures and methods
  - Structures: `Snapshot`, `SnapshotStats`, `Status`
  - Methods: CRUD operations for tags
  - Dependencies: `RocksDBWrapper`, `Changefeed`

**API Design:**
```cpp
class SnapshotManager {
public:
    struct Snapshot {
        std::string tag_name;
        uint64_t sequence_number;
        int64_t timestamp_ms;
        std::string description;
        std::string created_by;
    };
    
    Status createTag(const std::string& name, 
                     const std::string& description,
                     const std::string& created_by = "");
    Status deleteTag(const std::string& name);
    std::vector<Snapshot> listTags() const;
    std::optional<Snapshot> getTag(const std::string& name) const;
};
```

#### Task 1.2: SnapshotManager Implementation (2-3 days)
- **File:** `src/transaction/snapshot_manager.cpp`
- **Description:** Implement tag CRUD operations
  - RocksDB Column Family "tags" usage
  - Serialization/Deserialization (JSON via nlohmann::json)
  - Input validation (tag name regex, description length)
  - Error handling

**Key Features:**
- Tag name validation: `^[a-z0-9_-]+$` (lowercase, numbers, underscore, hyphen)
- Description max length: 500 characters
- Duplicate tag detection
- Atomic operations

#### Task 1.3: REST API Handler (1-2 days)
- **Files:** 
  - `include/server/snapshot_api_handler.h`
  - `src/server/snapshot_api_handler.cpp`
- **Endpoints:**
  - `POST /api/v1/snapshots/tags` - Create tag
  - `GET /api/v1/snapshots/tags` - List all tags
  - `GET /api/v1/snapshots/tags/:name` - Get specific tag
  - `DELETE /api/v1/snapshots/tags/:name` - Delete tag

**Request/Response Format:**
```json
// POST /api/v1/snapshots/tags
{
  "name": "before_migration",
  "description": "Safe point before Q1 2026 schema migration",
  "created_by": "admin"
}

// Response
{
  "tag_name": "before_migration",
  "sequence_number": 12345,
  "timestamp_ms": 1704985200000,
  "description": "Safe point before Q1 2026 schema migration",
  "created_by": "admin"
}
```

#### Task 1.4: Unit Tests (2 days)
- **File:** `tests/test_snapshot_manager.cpp`
- **Coverage Target:** ≥95%

**Test Cases:**
- [x] Tag Creation (Success, Duplicate, Invalid Name, Description Too Long)
- [x] Tag Retrieval (Exists, Not Exists)
- [x] Tag Deletion (Success, Not Exists)
- [x] Tag Listing (Empty, Multiple, Sorted by timestamp)
- [x] Statistics (Empty, With Tags)
- [x] Tag Exists Check
- [x] Serialization/Deserialization
- [x] Tag Name Validation (Valid: `valid_tag`, `valid-tag`, `valid123`; Invalid: `Invalid_Tag`, `invalid@tag`, empty, too long)

### Sprint 2 (Week 3-4): Testing & Documentation

#### Task 1.5: Integration Tests (1 day)
- **File:** `tests/test_snapshot_integration.cpp`
- **Scenarios:**
  - DB restart with tags (persistence check)
  - Concurrent tag operations (race conditions)
  - Large number of tags (1000+ tags performance)

#### Task 1.6: Performance Benchmarks (1 day)
- **File:** `benchmarks/bench_snapshot_manager.cpp`
- **Targets:**
  - CreateTag: <1ms per tag
  - GetTag: <0.5ms per retrieval
  - ListTags (100 tags): <10ms

#### Task 1.7: Documentation (2 days)
- **Files:**
  - `docs/en/features/features_snapshots.md`
  - `docs/de/features/features_snapshots.md`
- **Content:**
  - User guide with examples
  - API reference
  - Best practices
  - Use case scenarios (DR, compliance, schema migrations)

#### Task 1.8: OpenAPI Spec Update (0.5 days)
- **File:** `openapi/openapi.yaml`
- **Content:** Document all snapshot endpoints with schemas

---

## 📊 Deliverables

| File | Lines | Description |
|------|-------|-------------|
| `include/transaction/snapshot_manager.h` | ~150 | Header file |
| `src/transaction/snapshot_manager.cpp` | ~400 | Implementation |
| `include/server/snapshot_api_handler.h` | ~30 | API header |
| `src/server/snapshot_api_handler.cpp` | ~300 | API implementation |
| `tests/test_snapshot_manager.cpp` | ~600 | Unit tests |
| `tests/test_snapshot_integration.cpp` | ~200 | Integration tests |
| `benchmarks/bench_snapshot_manager.cpp` | ~150 | Benchmarks |
| `docs/en/features/features_snapshots.md` | ~500 | EN documentation |
| `docs/de/features/features_snapshots.md` | ~500 | DE documentation |
| **TOTAL** | **~2,830** | **LOC** |

---

## 🎯 Acceptance Criteria

- [ ] All unit tests pass (≥95% coverage)
- [ ] All integration tests pass
- [ ] Performance benchmarks met
- [ ] No memory leaks detected by valgrind
- [ ] REST API fully functional and documented
- [ ] Documentation complete in English and German
- [ ] Code review approved
- [ ] OpenAPI spec updated

---

## 📈 Performance Requirements

- **CreateTag:** <1ms per operation
- **GetTag:** <0.5ms per operation
- **ListTags:** <10ms for 100 tags
- **Memory:** No leaks, efficient tag storage
- **Concurrency:** Thread-safe operations

---

## 🔗 Dependencies

- RocksDB with "tags" Column Family
- Changefeed for sequence number tracking
- nlohmann::json for serialization
- HTTP server framework for REST API

---

## 📚 Related Documentation

- [Implementation Plan](../../docs/research/IMPLEMENTATION_PLAN_GIT_FEATURES.md)
- [Research Analysis](../../docs/research/GIT_LIKE_FEATURES_FOR_MVCC.md)
- [Executive Summary](../../GIT_FEATURES_ZUSAMMENFASSUNG.md)

---

## 💡 Use Case Examples

### Disaster Recovery
```cpp
// Before critical operation
mgr.createTag("before_migration", "Safe point before schema migration");

// Perform migration
performSchemaMigration();

// Rollback if needed
if (!validateMigration()) {
    pitr.restoreToTag("before_migration");
}
```

### Compliance Checkpoints
```cpp
// Quarterly audit point
mgr.createTag("q1_2026_audit", "Q1 2026 financial quarter end");
```

### Schema Versioning
```cpp
// Track schema versions
mgr.createTag("schema_v2_0_0", "Upgraded to schema version 2.0.0");
```

---

## ⏱️ Timeline

- **Duration:** 3-4 weeks (2 sprints)
- **Priority:** ⭐⭐⭐ Highest
- **Risk:** 🟢 Low
- **Estimated LOC:** ~2,830 lines

---

## 👥 Team Assignment

- **Developer 1:** Core implementation (SnapshotManager, API)
- **Developer 2 (QA):** Testing (unit, integration, benchmarks)
- **Tech Writer:** Documentation (EN + DE)

---

## 🚀 Next Phase

After Phase 1 completion, proceed to:
- **Phase 2:** Diff API (Structured Diff between snapshots)
- **Phase 3:** Point-in-Time Recovery (PITR with safety features)
