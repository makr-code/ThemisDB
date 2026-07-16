### Context

This issue implements the roadmap item '`ManifestDatabase`: Delete Associated Files on Entry Removal' for the updates domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: `ManifestDatabase`: Delete Associated Files on Entry Removal

### Goal

Deliver the scoped changes for `ManifestDatabase`: Delete Associated Files on Entry Removal in src/updates/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `ManifestDatabase`: Delete Associated Files on Entry Removal
**Priority:** Medium
**Target Version:** v1.8.0

`manifest_database.cpp` line 479: "TODO: Delete associated files from registry". When a manifest entry is removed, the associated binary files are not cleaned up from the registry directory, causing accumulation of orphaned files.

**Implementation Notes:**
- `[ ]` In `ManifestDatabase::removeEntry()`, after removing the RocksDB manifest record, enumerate associated file paths from the entry metadata and call `std::filesystem::remove()` for each.
- `[ ]` Guard against race: delete files only after the RocksDB entry is committed; use a tombstone key during the deletion window.
- `[ ]` Add test: insert manifest entry with 3 associated files, remove entry, verify all 3 files are deleted.

---


**Priority:** High  
**Target Version:** v1.7.0

Coordinate updates across all nodes in a ThemisDB cluster with Raft consensus.

**Features:**
- Raft-based consensus for cluster-wide updates
- Rolling updates (update one node at a time)
- Automatic health checks before/after updates
- Abort on failure with automatic rollback
- Version skew protection (max 1 minor version difference)
- Leader election for update coordination

**API:**
```cpp
ClusterUpdateManager cluster_updates(raft_manager);

// Initiate cluster-wide update
ClusterUpdateResult result = cluster_updates.updateCluster("1.7.0", {
    .rolling = true,                    // Rolling update
    .max_unavailable = 1,               // Max nodes down at once
    .health_check_timeout = 30s,        // Health check timeout
    .rollback_on_failure = true,        // Auto-rollback on failure
    .parallel_updates = false           // Sequential updates
});

// Monitor progress
cluster_updates.setProgressCallback([](const ClusterUpdateProgress& progress) {
    std::cout << "Updated: " << progress.nodes_updated << "/" << progress.total_nodes << "\n";
    std::cout << "Current node: " << progress.current_node << "\n";
    std::cout << "Status: " << progress.status << "\n";
});

// Wait for completion
if (result.wait()) {
    LOG_INFO("Cluster updated successfully");
} else {
    LOG_ERROR("Cluster update failed: {}", result.error_message);
}
```

**Rolling Update Procedure:**
```
1. Elect update coordinator (Raft leader)
2. For each node (excluding leader):
   a. Drain connections
   b. Download and verify update
   c. Backup current version
   d. Apply update
   e. Restart node
   f. Health check
   g. Rejoin cluster
3. Update leader last
4. Verify cluster health
```

**Use Cases:**
- Zero-downtime cluster upgrades
- Coordinated schema migrations
- Automatic failover during updates

---

### Acceptance Criteria

- [ ] In `ManifestDatabase::removeEntry()`, after removing the RocksDB manifest record, enumerate associated file paths from the entry metadata and call `std::filesystem::remove()` for each.
- [ ] Guard against race: delete files only after the RocksDB entry is committed; use a tombstone key during the deletion window.
- [ ] Add test: insert manifest entry with 3 associated files, remove entry, verify all 3 files are deleted.
- [ ] Raft-based consensus for cluster-wide updates
- [ ] Rolling updates (update one node at a time)
- [ ] Automatic health checks before/after updates
- [ ] Abort on failure with automatic rollback
- [ ] Version skew protection (max 1 minor version difference)
- [ ] Leader election for update coordination
- [ ] Zero-downtime cluster upgrades
- [ ] Coordinated schema migrations
- [ ] Automatic failover during updates

### Relationships

- Roadmap row: #214 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/updates/FUTURE_ENHANCEMENTS.md#manifestdatabase-delete-associated-files-on-entry-removal
- Source key: roadmap:214:updates:v1.8.0:manifestdatabase-delete-associated-files-on-entry-removal

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:214:updates:v1.8.0:manifestdatabase-delete-associated-files-on-entry-removal -->
<!-- roadmap-ref: row=214;module=updates;target=v1.8.0 -->
<!-- roadmap-detail: src/updates/FUTURE_ENHANCEMENTS.md#manifestdatabase-delete-associated-files-on-entry-removal -->
