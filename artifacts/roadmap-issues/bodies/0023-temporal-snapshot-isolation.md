### Context

This issue implements the roadmap item 'Snapshot Isolation' for the temporal domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Immediate (≤ v1.4.0) and targets milestone v1.1.0.

Primary detail section: Snapshot Isolation

### Goal

Deliver the scoped changes for Snapshot Isolation in src/temporal/ and complete the linked detail section in a release-ready state for v1.1.0.

### Detailed Scope

### Snapshot Isolation
**Priority:** High  
**Target Version:** v1.1.0

Transactional isolation for temporal queries.

**Features:**
- Consistent snapshot creation
- Repeatable read isolation for temporal queries
- Snapshot versioning
- Snapshot garbage collection
- Distributed snapshot coordination

**Implementation:**
```cpp
class TemporalSnapshotManager {
public:
    // Create consistent snapshot
    Result<SnapshotHandle> createSnapshot(
        const std::vector<std::string>& tables
    );
    
    // Query using snapshot
    Result<std::vector<Document>> querySnapshot(
        const SnapshotHandle& snapshot,
        const std::string& query
    );
    
    // Release snapshot
    Result<bool> releaseSnapshot(
        const SnapshotHandle& snapshot
    );
};

struct SnapshotHandle {
    std::string snapshot_id;
    std::chrono::system_clock::time_point creation_time;
    std::vector<std::string> included_tables;
    uint64_t version_number;
};
```

---

### Acceptance Criteria

- [ ] Consistent snapshot creation
- [ ] Repeatable read isolation for temporal queries
- [ ] Snapshot versioning
- [ ] Snapshot garbage collection
- [ ] Distributed snapshot coordination

### Relationships

- Roadmap row: #23 (🟠 High Priority — Immediate (≤ v1.4.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/temporal/FUTURE_ENHANCEMENTS.md#snapshot-isolation
- Source key: roadmap:23:temporal:v1.1.0:snapshot-isolation

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:23:temporal:v1.1.0:snapshot-isolation -->
<!-- roadmap-ref: row=23;module=temporal;target=v1.1.0 -->
<!-- roadmap-detail: src/temporal/FUTURE_ENHANCEMENTS.md#snapshot-isolation -->
