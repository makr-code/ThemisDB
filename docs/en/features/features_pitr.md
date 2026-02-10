# Point-in-Time Recovery (PITR) for ThemisDB

**Version:** 1.0  
**Date:** January 12, 2026  
**Status:** ✅ Implemented  
**Category:** Features

---

## Table of Contents

- [Overview](#overview)
- [Key Features](#key-features)
- [Architecture](#architecture)
- [Usage Guide](#usage-guide)
  - [Named Snapshots](#named-snapshots)
  - [Point-in-Time Recovery](#point-in-time-recovery)
  - [Dry-Run Preview](#dry-run-preview)
- [API Reference](#api-reference)
- [Best Practices](#best-practices)
- [Troubleshooting](#troubleshooting)

---

## Overview

Point-in-Time Recovery (PITR) is a Git-like feature for ThemisDB's MVCC system that enables database administrators to:

- **Create named snapshots** (semantic tags) at important points in time
- **Restore the database** to any previous state by sequence, tag, or timestamp
- **Preview restore operations** before applying them
- **Selectively restore** specific tables

### Why PITR?

PITR provides critical capabilities for:

- **Disaster Recovery**: Restore after data corruption or accidental deletions
- **Schema Migration Rollback**: Undo failed schema changes
- **Compliance**: Maintain historical audit points
- **Testing**: Restore to known-good states
- **DevOps**: Safe points before deployments

---

## Key Features

### 1. Named Snapshots (Semantic Tagging)

Create immutable snapshots with meaningful names:

```cpp
// Create a snapshot before a critical operation
snapshot_mgr.createTag("before_migration", "Before Q1 2026 schema migration");

// List all snapshots
auto snapshots = snapshot_mgr.listTags(true); // Sort by time

// Get a specific snapshot
auto snapshot = snapshot_mgr.getTag("before_migration");

// Delete old snapshots
snapshot_mgr.deleteTag("old_snapshot");
```

**Features:**
- ✅ Persistent storage in RocksDB "tags" column family
- ✅ Validation of tag names (lowercase, alphanumeric, hyphens, underscores)
- ✅ Description up to 500 characters
- ✅ Automatic timestamp and sequence number capture
- ✅ User attribution

### 2. Point-in-Time Recovery

Restore database to any previous state:

```cpp
// Restore to a named snapshot
pitr_mgr.restoreToTag("before_migration");

// Restore to a specific sequence number
pitr_mgr.restoreToSequence(12345);

// Restore to a specific timestamp
pitr_mgr.restoreToTimestamp(1705045200000); // Unix timestamp in ms
```

**Features:**
- ✅ Restore by sequence number, tag name, or timestamp
- ✅ Automatic backup before restore
- ✅ Rollback on errors
- ✅ Progress tracking
- ✅ Selective table restore

### 3. Safety Features

Multiple layers of protection:

```cpp
PITRManager::RestoreOptions options;
options.dry_run = true;              // Preview only, don't apply
options.create_backup = true;        // Auto-backup before restore
options.abort_on_first_error = true; // Stop on first error
options.tables = {"users", "orders"}; // Selective restore
options.max_events_to_replay = 10000; // Limit events

// Preview the restore
auto preview = pitr_mgr.previewRestore(target_sequence, options);
std::cout << "Events to replay: " << preview.events_to_replay << "\n";
std::cout << "Affected tables: " << preview.affected_tables.size() << "\n";
std::cout << "Estimated duration: " << preview.estimated_duration_sec << "s\n";

// Apply the restore if preview looks good
auto status = pitr_mgr.restoreToSequence(target_sequence, options);
```

---

## Architecture

### Components

```
┌──────────────────────────────────────────┐
│         SnapshotManager                   │
│  - createTag(name, description)          │
│  - getTag(name)                          │
│  - listTags()                            │
│  - deleteTag(name)                       │
└──────────────┬───────────────────────────┘
               │
               │ Uses
               │
┌──────────────▼───────────────────────────┐
│          PITRManager                      │
│  - restoreToSequence(seq)                │
│  - restoreToTag(name)                    │
│  - restoreToTimestamp(ts)                │
│  - previewRestore(seq)                   │
└──────────────┬───────────────────────────┘
               │
      ┌────────┴────────┐
      │                 │
┌─────▼─────┐    ┌──────▼──────┐
│Changefeed │    │ RocksDB     │
│           │    │ TransactionDB│
└───────────┘    └─────────────┘
```

### Data Flow

1. **Snapshot Creation**:
   - Capture current changefeed sequence
   - Store tag metadata in "tags" column family
   - Persist to RocksDB

2. **Point-in-Time Recovery**:
   - Retrieve target sequence (from tag/timestamp)
   - Create auto-backup snapshot
   - Replay changefeed events backward
   - Apply reverse operations (PUT→DELETE)
   - Commit or rollback

### Storage

**Tags Column Family**:
```
Key: tag:{tag_name}
Value: {
  "tag_name": "before_migration",
  "sequence_number": 12345,
  "timestamp_ms": 1705045200000,
  "description": "Before Q1 2026 schema migration",
  "created_by": "admin"
}
```

---

## Usage Guide

### Named Snapshots

#### Create a Snapshot

```cpp
#include "transaction/snapshot_manager.h"

// Initialize
SnapshotManager snapshot_mgr(db, tags_cf, changefeed);

// Create snapshot
auto status = snapshot_mgr.createTag(
    "quarterly_backup_2026_q1",
    "Q1 2026 quarterly backup for compliance",
    "admin"
);

if (!status.ok) {
    std::cerr << "Failed to create snapshot: " << status.message << "\n";
}
```

#### List Snapshots

```cpp
// List all snapshots sorted by time (newest first)
auto snapshots = snapshot_mgr.listTags(true);

for (const auto& snap : snapshots) {
    std::cout << "Tag: " << snap.tag_name << "\n";
    std::cout << "  Sequence: " << snap.sequence_number << "\n";
    std::cout << "  Time: " << snap.timestamp_ms << "\n";
    std::cout << "  Description: " << snap.description << "\n";
    std::cout << "  Created by: " << snap.created_by << "\n\n";
}

// Sort by name instead
auto snapshots_by_name = snapshot_mgr.listTags(false);
```

#### Get Snapshot Statistics

```cpp
auto stats = snapshot_mgr.getStats();

std::cout << "Total snapshots: " << stats.total_snapshots << "\n";
std::cout << "Storage size: " << stats.total_size_bytes << " bytes\n";
std::cout << "Oldest: " << stats.oldest_timestamp_ms << "\n";
std::cout << "Newest: " << stats.newest_timestamp_ms << "\n";
```

### Point-in-Time Recovery

#### Restore to a Named Snapshot

```cpp
#include "storage/pitr_manager.h"

// Initialize
PITRManager pitr_mgr(db_wrapper, changefeed, snapshot_mgr);

// Configure restore options
PITRManager::RestoreOptions options;
options.create_backup = true;       // Auto-backup before restore
options.backup_tag = "before_restore_2026_01_12";

// Restore
auto status = pitr_mgr.restoreToTag("before_migration", options);

if (status.ok) {
    std::cout << "Restore completed successfully\n";
    if (status.progress.has_value()) {
        std::cout << "Events processed: " << status.progress->events_processed << "\n";
        std::cout << "Duration: " << status.progress->getElapsedMs() << "ms\n";
    }
} else {
    std::cerr << "Restore failed: " << status.message << "\n";
}
```

#### Restore to a Specific Sequence

```cpp
uint64_t target_sequence = 12000;

PITRManager::RestoreOptions options;
options.create_backup = true;

auto status = pitr_mgr.restoreToSequence(target_sequence, options);
```

#### Restore to a Timestamp

```cpp
// Restore to January 10, 2026, 12:00:00 UTC
int64_t timestamp_ms = 1704888000000;

PITRManager::RestoreOptions options;
options.create_backup = true;

auto status = pitr_mgr.restoreToTimestamp(timestamp_ms, options);
```

### Dry-Run Preview

Always preview before restoring:

```cpp
uint64_t target_sequence = 10000;

PITRManager::RestoreOptions options;
options.dry_run = true; // Preview only

auto preview = pitr_mgr.previewRestore(target_sequence, options);

std::cout << "Restore Preview:\n";
std::cout << "  Target sequence: " << preview.target_sequence << "\n";
std::cout << "  Current sequence: " << preview.current_sequence << "\n";
std::cout << "  Events to replay: " << preview.events_to_replay << "\n";
std::cout << "  Affected tables: ";
for (const auto& table : preview.affected_tables) {
    std::cout << table << " ";
}
std::cout << "\n";
std::cout << "  Sample keys (first 10):\n";
for (size_t i = 0; i < std::min(preview.affected_keys.size(), size_t(10)); ++i) {
    std::cout << "    - " << preview.affected_keys[i] << "\n";
}
std::cout << "  Estimated duration: " << preview.estimated_duration_sec << "s\n";
std::cout << "  Estimated size: " << preview.estimated_size_bytes << " bytes\n";

// Confirm with user
std::cout << "Proceed? (yes/no): ";
std::string confirm;
std::cin >> confirm;

if (confirm == "yes") {
    options.dry_run = false;
    auto status = pitr_mgr.restoreToSequence(target_sequence, options);
}
```

### Selective Restore

Restore only specific tables:

```cpp
PITRManager::RestoreOptions options;
options.tables = {"users", "orders"}; // Only restore these tables
options.create_backup = true;

auto status = pitr_mgr.restoreToTag("before_migration", options);
```

### Progress Tracking

Monitor long-running restore operations:

```cpp
// Start restore in background (would need threading)
PITRManager::RestoreOptions options;
auto status = pitr_mgr.restoreToSequence(1000, options);

// Check progress
auto progress = pitr_mgr.getProgress();
if (progress.has_value()) {
    std::cout << "Phase: " << static_cast<int>(progress->phase) << "\n";
    std::cout << "Progress: " << progress->getProgressPercent() << "%\n";
    std::cout << "Events processed: " << progress->events_processed 
              << "/" << progress->total_events << "\n";
    std::cout << "Current table: " << progress->current_table << "\n";
    std::cout << "Elapsed: " << progress->getElapsedMs() << "ms\n";
}
```

---

## API Reference

### SnapshotManager

#### createTag
```cpp
Status createTag(const std::string& tag_name,
                const std::string& description,
                const std::string& created_by = "system");
```
Creates a new snapshot tag.

**Parameters:**
- `tag_name`: Unique identifier (lowercase, alphanumeric, hyphens, underscores, 1-64 chars)
- `description`: Human-readable description (max 500 chars)
- `created_by`: User identifier (optional)

**Returns:** Status indicating success or failure

**Errors:**
- `ALREADY_EXISTS`: Tag name already exists
- `INVALID_NAME`: Tag name contains invalid characters
- `INVALID_DESCRIPTION`: Description too long

#### getTag
```cpp
std::optional<Snapshot> getTag(const std::string& tag_name) const;
```
Retrieves snapshot metadata by tag name.

**Returns:** Snapshot if exists, nullopt otherwise

#### listTags
```cpp
std::vector<Snapshot> listTags(bool sort_by_time = true) const;
```
Lists all snapshots.

**Parameters:**
- `sort_by_time`: If true, sort by timestamp (newest first), otherwise by name

**Returns:** Vector of all snapshots

#### deleteTag
```cpp
Status deleteTag(const std::string& tag_name);
```
Deletes a snapshot tag.

**Returns:** Status indicating success or failure

**Errors:**
- `NOT_FOUND`: Tag does not exist

#### getStats
```cpp
Stats getStats() const;
```
Gets statistics about snapshots.

**Returns:** Stats structure with total count, storage size, and timestamp range

### PITRManager

#### restoreToSequence
```cpp
Status restoreToSequence(uint64_t target_sequence, 
                        const RestoreOptions& options = {});
```
Restores database to a specific sequence number.

**Parameters:**
- `target_sequence`: Target sequence to restore to
- `options`: Restore options (dry_run, backup, etc.)

**Returns:** Status with progress information

**Errors:**
- `INVALID_SEQUENCE`: Target sequence is invalid or in the future
- `BACKUP_FAILED`: Auto-backup creation failed
- `REPLAY_FAILED`: Event replay failed

#### restoreToTag
```cpp
Status restoreToTag(const std::string& tag_name, 
                   const RestoreOptions& options = {});
```
Restores database to a named snapshot tag.

**Parameters:**
- `tag_name`: Tag identifier
- `options`: Restore options

**Returns:** Status with progress information

**Errors:**
- `TAG_NOT_FOUND`: Tag does not exist
- (plus all errors from restoreToSequence)

#### restoreToTimestamp
```cpp
Status restoreToTimestamp(int64_t timestamp_ms, 
                         const RestoreOptions& options = {});
```
Restores database to a specific timestamp.

**Parameters:**
- `timestamp_ms`: Unix timestamp in milliseconds
- `options`: Restore options

**Returns:** Status with progress information

**Errors:**
- `NO_EVENTS_AT_TIME`: No events found at or before timestamp
- (plus all errors from restoreToSequence)

#### previewRestore
```cpp
RestorePreview previewRestore(uint64_t target_sequence, 
                             const RestoreOptions& options = {}) const;
```
Previews a restore operation (dry-run).

**Parameters:**
- `target_sequence`: Target sequence to restore to
- `options`: Restore options (only tables filter is used)

**Returns:** Preview with estimated impact

---

## Best Practices

### 1. Always Create Backups

Create snapshots before critical operations:

```cpp
// Before schema migration
snapshot_mgr.createTag("before_migration_2026_01", "Before user schema v2.0");
migrate_schema();

// Before large data import
snapshot_mgr.createTag("before_import_2026_01", "Before bulk import");
import_data();
```

### 2. Use Dry-Run First

Always preview restore operations:

```cpp
// Preview first
PITRManager::RestoreOptions preview_opts;
preview_opts.dry_run = true;
auto preview = pitr_mgr.restoreToTag("before_migration", preview_opts);

// Review and confirm
if (preview.events_to_replay < 10000) {
    preview_opts.dry_run = false;
    pitr_mgr.restoreToTag("before_migration", preview_opts);
}
```

### 3. Implement Retention Policies

Delete old snapshots regularly:

```cpp
auto snapshots = snapshot_mgr.listTags(true); // Sorted by time

// Keep only last 10 snapshots
if (snapshots.size() > 10) {
    for (size_t i = 10; i < snapshots.size(); ++i) {
        snapshot_mgr.deleteTag(snapshots[i].tag_name);
    }
}
```

### 4. Use Selective Restore

Restore only affected tables:

```cpp
PITRManager::RestoreOptions options;
options.tables = {"users"}; // Only restore users table
options.create_backup = true;

pitr_mgr.restoreToTag("before_user_migration", options);
```

### 5. Monitor Progress

Track progress for long operations:

```cpp
// Check if restore is in progress
if (pitr_mgr.isRestoreInProgress()) {
    auto progress = pitr_mgr.getProgress();
    if (progress.has_value()) {
        log_progress(progress.value());
    }
}
```

### 6. Tag Naming Convention

Use consistent tag naming:

```cpp
// Good:
"before_migration_2026_q1"
"quarterly_backup_2026_01"
"hotfix_rollback_2026_01_12"

// Bad:
"Backup1"  // Uppercase not allowed
"backup 1" // Spaces not allowed
"backup@1" // Special chars not allowed
```

---

## gRPC API

In addition to the C++ API and REST API, PITR functionality is also available via gRPC for high-performance binary communication.

### Service Definition

**Service:** `themis.core.PITRService`  
**Proto file:** `proto/themis_core.proto`

### Available Methods

| Method | Purpose | Request | Response |
|--------|---------|---------|----------|
| `CreateSnapshot` | Create named snapshot | `CreateSnapshotRequest` | `CreateSnapshotResponse` |
| `ListSnapshots` | List all snapshots | `ListSnapshotsRequest` | `ListSnapshotsResponse` |
| `GetSnapshot` | Get snapshot details | `GetSnapshotRequest` | `GetSnapshotResponse` |
| `DeleteSnapshot` | Delete snapshot | `DeleteSnapshotRequest` | `DeleteSnapshotResponse` |
| `PreviewRestore` | Preview restore (dry-run) | `PreviewRestoreRequest` | `PreviewRestoreResponse` |
| `ExecuteRestore` | Execute restore operation | `ExecuteRestoreRequest` | `ExecuteRestoreResponse` |
| `GetRestoreProgress` | Get restore progress | `GetRestoreProgressRequest` | `GetRestoreProgressResponse` |

### Usage Example (grpcurl)

```bash
# Create snapshot
grpcurl -plaintext -d '{
  "tag_name": "before_deploy_v2.0",
  "description": "Before version 2.0 deployment",
  "created_by": "admin"
}' localhost:50051 themis.core.PITRService/CreateSnapshot

# List snapshots
grpcurl -plaintext -d '{
  "limit": 10,
  "sort_by": "timestamp",
  "ascending": false
}' localhost:50051 themis.core.PITRService/ListSnapshots

# Preview restore
grpcurl -plaintext -d '{
  "restore_type": "TAG",
  "target": "before_deploy_v2.0",
  "tables": ["users", "orders"]
}' localhost:50051 themis.core.PITRService/PreviewRestore

# Execute restore
grpcurl -plaintext -d '{
  "restore_type": "TAG",
  "target": "before_deploy_v2.0",
  "dry_run": false,
  "create_backup": true,
  "abort_on_first_error": true
}' localhost:50051 themis.core.PITRService/ExecuteRestore

# Get restore progress
grpcurl -plaintext localhost:50051 themis.core.PITRService/GetRestoreProgress
```

### Message Structures

**RestoreType enum:**
- `SEQUENCE` (0) - Restore to specific sequence number
- `TAG` (1) - Restore to named snapshot
- `TIMESTAMP` (2) - Restore to specific timestamp

**RestoreProgress.Phase enum:**
- `NOT_STARTED` - Restore not yet initiated
- `CREATING_BACKUP` - Creating automatic backup
- `VALIDATING` - Validating parameters
- `REPLAYING_EVENTS` - Replaying events backward
- `COMMITTING` - Committing changes
- `COMPLETED` - Restore completed successfully
- `FAILED` - Restore failed with error
- `ROLLED_BACK` - Restore rolled back

For complete message definitions, see [proto/themis_core.proto](../../../proto/themis_core.proto).

---

## Troubleshooting

### Problem: Tag creation fails with "Invalid tag name"

**Solution:** Tag names must be lowercase, alphanumeric, hyphens, and underscores only:

```cpp
// Invalid
snapshot_mgr.createTag("Backup_2026", "..."); // Uppercase
snapshot_mgr.createTag("backup 2026", "..."); // Space
snapshot_mgr.createTag("1backup", "..."); // Starts with digit

// Valid
snapshot_mgr.createTag("backup_2026", "...");
snapshot_mgr.createTag("backup-2026", "...");
snapshot_mgr.createTag("b2026", "...");
```

### Problem: Restore fails with "Target sequence must be less than current sequence"

**Solution:** You cannot restore to the current or future sequence. Use a past sequence:

```cpp
uint64_t current = changefeed->getLatestSequence();
uint64_t target = current - 100; // Restore to 100 sequences ago

pitr_mgr.restoreToSequence(target);
```

### Problem: Restore is very slow

**Solution:** Use selective restore and limit events:

```cpp
PITRManager::RestoreOptions options;
options.tables = {"critical_table"}; // Only restore one table
options.max_events_to_replay = 10000; // Limit events

pitr_mgr.restoreToSequence(target, options);
```

### Problem: "Tag not found" error

**Solution:** Verify tag exists before restoring:

```cpp
if (!snapshot_mgr.getTag("my_tag").has_value()) {
    std::cerr << "Tag 'my_tag' does not exist\n";
    
    // List available tags
    auto tags = snapshot_mgr.listTags();
    std::cout << "Available tags:\n";
    for (const auto& tag : tags) {
        std::cout << "  - " << tag.tag_name << "\n";
    }
    return;
}

pitr_mgr.restoreToTag("my_tag");
```

### Problem: Running out of disk space

**Solution:** Implement retention policy and delete old snapshots:

```cpp
// Get statistics
auto stats = snapshot_mgr.getStats();
std::cout << "Total size: " << stats.total_size_bytes << " bytes\n";

// Delete old snapshots
auto snapshots = snapshot_mgr.listTags(true);
for (size_t i = 20; i < snapshots.size(); ++i) {
    snapshot_mgr.deleteTag(snapshots[i].tag_name);
}
```

---

## Related Documentation

- [MVCC Architecture](../architecture/architecture_mvcc.md)
- [Changefeed Documentation](../cdc/changefeed.md)
- [Disaster Recovery Guide](../guides/disaster_recovery.md)
- [Git-like Features Research](../research/GIT_LIKE_FEATURES_FOR_MVCC.md)

---

**Version History:**
- v1.0 (2026-01-12): Initial implementation
- Phase 3 of Git-like features for MVCC

**Author:** ThemisDB Development Team  
**License:** MIT License (Community Edition)
