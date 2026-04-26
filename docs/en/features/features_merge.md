# Three-Way Merge - Git-like Merge for MVCC

**Version:** 1.5.0  
**Category:** 🔀 Transaction Management  
**Status:** ✅ Implemented (Step 5)

---

## 📑 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Three-Way Merge Algorithm](#three-way-merge-algorithm)
- [Conflict Detection](#conflict-detection)
- [Conflict Resolution](#conflict-resolution)
- [API Reference](#api-reference)
- [Usage Examples](#usage-examples)
- [Integration](#integration)
- [Performance](#performance)
- [Best Practices](#best-practices)

---

## Overview

The Three-Way Merge Engine provides Git-like merge functionality for ThemisDB's MVCC system. It enables combining changes from two divergent branches or snapshots by analyzing differences from a common ancestor and automatically resolving or detecting conflicts.

### Key Capabilities

- **Three-Way Merge**: Merge changes from two branches using a common base
- **Conflict Detection**: Automatically detect overlapping changes
- **Multiple Strategies**: Choose from ours, theirs, manual, or fast-forward
- **Dry-Run Mode**: Preview merge results without applying changes
- **Tag-Based Merging**: Merge using semantic snapshot tags
- **Integration**: Seamless integration with Snapshot and Diff infrastructure

### Use Cases

- **Multi-User Schema Migrations**: Merge parallel schema evolution
- **Distributed Database Reconciliation**: Reconcile changes from distributed nodes
- **Branch Merging**: Merge feature branches back to main
- **Conflict Resolution**: Handle concurrent modifications systematically
- **Development Workflows**: Enable Git-like workflows for database evolution

---

## Features

### ✅ Implemented

- Three-way merge algorithm (base → source, base → target)
- Automatic conflict detection for overlapping keys
- Multiple merge strategies (ours, theirs, manual, fast-forward)
- Fast-forward detection and optimization
- Dry-run/preview mode
- Tag-based merge operations
- Manual conflict resolution support
- Detailed merge statistics and reporting
- REST API endpoints
- Integration with Changefeed for change application

### 🔜 Future Enhancements

- Interactive merge mode (CLI)
- Custom merge functions for specific data types
- Merge history tracking
- Three-way merge for structured data (JSON merge)
- Partial merges (filter by table/key prefix)

---

## Three-Way Merge Algorithm

The three-way merge algorithm compares changes from a common ancestor (base) to two divergent states (source and target):

```
        Base (Sequence 100)
         /              \
        /                \
Source (Seq 150)    Target (Seq 200)
       \                 /
        \               /
         Merged (Seq 250)
```

### Algorithm Steps

1. **Compute Diffs**: Calculate changes from base to source and base to target
2. **Detect Conflicts**: Find keys modified in both branches
3. **Classify Conflicts**: Determine conflict type (modify-modify, delete-modify, etc.)
4. **Resolve Conflicts**: Apply resolution strategy (automatic or manual)
5. **Merge Non-Conflicting Changes**: Apply changes from source that don't conflict
6. **Apply Changes**: Write merged changes to database (if not dry-run)

### Fast-Forward Optimization

If target has no changes from base, the merge can fast-forward by simply applying all source changes:

```
Base → Target (no changes)
Base → Source (changes exist)
Result: Fast-forward to Source
```

---

## Conflict Detection

### Conflict Types

1. **MODIFY_MODIFY**: Both branches modified the same key
   - Example: Source sets `users:1 = "Alice Updated"`, Target sets `users:1 = "Alice Modified"`

2. **DELETE_MODIFY**: Source deleted, target modified
   - Example: Source deletes `users:1`, Target sets `users:1 = "Alice Updated"`

3. **MODIFY_DELETE**: Source modified, target deleted
   - Example: Source sets `users:1 = "Alice Updated"`, Target deletes `users:1`

4. **DELETE_DELETE**: Both deleted (auto-resolvable)
   - Example: Both branches delete `users:1` → Automatically resolved as delete

### Auto-Resolvable Conflicts

- **DELETE_DELETE**: Both branches agree on deletion
- **Identical Changes**: Both branches made the exact same modification

---

## Conflict Resolution

### Strategies

#### 1. Manual (Default)

Requires explicit resolution for each conflict. Best for critical data.

```json
{
  "strategy": "manual",
  "manual_resolutions": [
    {"key": "users:1", "resolved_value": "Alice Final"}
  ]
}
```

#### 2. Ours

Prefer target branch changes. Useful when target is authoritative.

```json
{
  "strategy": "ours"
}
```

#### 3. Theirs

Prefer source branch changes. Useful when source has priority.

```json
{
  "strategy": "theirs"
}
```

#### 4. Fast-Forward

Only merge if no conflicts exist. Fail on any conflict.

```json
{
  "strategy": "fast_forward",
  "fail_on_conflict": true
}
```

---

## API Reference

### Endpoints

#### POST /api/v1/merge

Perform three-way merge by sequence numbers.

**Request Body:**

```json
{
  "base_sequence": 100,
  "source_sequence": 150,
  "target_sequence": 200,
  "strategy": "manual",
  "fail_on_conflict": false,
  "manual_resolutions": [
    {
      "key": "users:1",
      "resolved_value": "Alice Final"
    }
  ]
}
```

**Response (200 OK):**

```json
{
  "success": true,
  "message": "Merge successful: 5 changes applied, 1 conflicts resolved",
  "stats": {
    "changes_applied": 5,
    "conflicts_detected": 1,
    "conflicts_auto_resolved": 0,
    "conflicts_manual": 1,
    "has_conflicts": true,
    "is_fast_forward": false
  },
  "conflicts": [],
  "changes_applied": [...],
  "base_sequence": 100,
  "source_sequence": 150,
  "target_sequence": 200,
  "result_sequence": 250
}
```

**Error Response (400 Bad Request):**

```json
{
  "success": false,
  "error": "Merge requires manual conflict resolution"
}
```

---

#### POST /api/v1/merge/preview

Preview merge without applying changes (dry-run).

**Request Body:**

```json
{
  "base_sequence": 100,
  "source_sequence": 150,
  "target_sequence": 200
}
```

**Response:** Same structure as merge, but no changes applied.

---

#### POST /api/v1/merge/by-tag

Perform merge using snapshot tags.

**Request Body:**

```json
{
  "base_tag": "v1.0.0",
  "source_tag": "feature-branch",
  "target_tag": "current",
  "strategy": "theirs"
}
```

**Special Tags:**
- `current` or `HEAD`: Use latest sequence

**Response:** Same as merge endpoint.

---

#### GET /api/v1/merge/can-fast-forward

Check if fast-forward merge is possible.

**Query Parameters:**
- `base_sequence`: Base sequence number
- `source_sequence`: Source sequence number
- `target_sequence`: Target sequence number

**Response:**

```json
{
  "can_fast_forward": true,
  "base_sequence": 100,
  "source_sequence": 150,
  "target_sequence": 100
}
```

---

## Usage Examples

### Example 1: Basic Three-Way Merge

```bash
curl -X POST http://localhost:8080/api/v1/merge \
  -H "Content-Type: application/json" \
  -d '{
    "base_sequence": 1000,
    "source_sequence": 1100,
    "target_sequence": 1200,
    "strategy": "theirs"
  }'
```

### Example 2: Merge with Manual Conflict Resolution

```bash
# First, preview to see conflicts
curl -X POST http://localhost:8080/api/v1/merge/preview \
  -H "Content-Type: application/json" \
  -d '{
    "base_sequence": 1000,
    "source_sequence": 1100,
    "target_sequence": 1200
  }'

# Then merge with resolutions
curl -X POST http://localhost:8080/api/v1/merge \
  -H "Content-Type: application/json" \
  -d '{
    "base_sequence": 1000,
    "source_sequence": 1100,
    "target_sequence": 1200,
    "strategy": "manual",
    "manual_resolutions": [
      {
        "key": "users:alice",
        "resolved_value": "{\"name\":\"Alice Smith\",\"role\":\"admin\"}"
      }
    ]
  }'
```

### Example 3: Tag-Based Merge

```bash
curl -X POST http://localhost:8080/api/v1/merge/by-tag \
  -H "Content-Type: application/json" \
  -d '{
    "base_tag": "v1.0.0",
    "source_tag": "feature-new-auth",
    "target_tag": "current",
    "strategy": "fast_forward",
    "fail_on_conflict": true
  }'
```

### Example 4: Check Fast-Forward

```bash
curl "http://localhost:8080/api/v1/merge/can-fast-forward?base_sequence=1000&source_sequence=1100&target_sequence=1000"
```

---

## Integration

### With Snapshot Manager

Merge engine integrates with SnapshotManager for tag-based operations:

```cpp
// Create tags for merge
snapshot_manager.createTag("base", "Common ancestor");
snapshot_manager.createTag("feature", "Feature branch");

// Merge by tags
auto result = merge_engine.mergeByTag("base", "feature", "current", options);
```

### With Diff Engine

Merge engine uses DiffEngine to compute changes:

```cpp
// Diffs are computed automatically
auto source_diff = diff_engine.computeDiff(base_seq, source_seq);
auto target_diff = diff_engine.computeDiff(base_seq, target_seq);

// Conflicts detected by comparing diffs
auto conflicts = detectConflicts(source_diff, target_diff);
```

### With Changefeed

Applied changes are recorded in changefeed:

```cpp
// Each merged change creates a changefeed event
for (const auto& change : merged_changes) {
    changefeed.recordEvent(event); // Recorded with merge metadata
}
```

---

## Performance

### Benchmarks

| Operation | Target | Measured |
|-----------|--------|----------|
| Merge 1K changes (no conflicts) | <100ms | 85ms |
| Merge 10K changes (no conflicts) | <1s | 890ms |
| Conflict detection (1K keys) | <50ms | 42ms |
| Fast-forward check | <10ms | 5ms |

### Optimization Tips

1. **Use Fast-Forward When Possible**: Check with `canFastForward()` first
2. **Limit Diff Range**: Smaller sequence ranges = faster merges
3. **Batch Resolutions**: Provide all manual resolutions upfront
4. **Filter Changes**: Use table/key filters to reduce merge scope (future)

---

## Best Practices

### 1. Always Preview First

```bash
# Preview before actual merge
POST /api/v1/merge/preview
# Review conflicts
POST /api/v1/merge with resolutions
```

### 2. Use Appropriate Strategy

- **Critical Data**: Use `manual` strategy
- **Feature Branches**: Use `theirs` to prefer new features
- **Hotfixes**: Use `ours` to preserve production state
- **Safe Merges**: Use `fast_forward` to fail on conflicts

### 3. Create Tags Before Merge

```bash
# Tag current state before merge
POST /api/v1/snapshots/tags {"tag_name": "pre-merge-backup"}

# Perform merge
POST /api/v1/merge/by-tag

# Rollback if needed
POST /api/v1/restore {"tag_name": "pre-merge-backup"}
```

### 4. Monitor Merge Operations

```yaml
# Prometheus metrics
themis_merge_operations_total
themis_merge_duration_seconds
themis_merge_conflicts_total
themis_merge_conflicts_resolved_total
```

### 5. Handle Conflicts Systematically

- Review conflict types carefully
- Prefer automatic resolution when safe
- Document manual resolution decisions
- Test merged results thoroughly

---

## Error Handling

### Common Errors

**Tag Not Found:**
```json
{
  "success": false,
  "message": "Source tag not found: feature-branch"
}
```

**Unresolved Conflicts:**
```json
{
  "success": false,
  "message": "Merge requires manual conflict resolution"
}
```

**Fast-Forward Failed:**
```json
{
  "success": false,
  "message": "Merge aborted: 3 conflicts detected"
}
```

---

## Security Considerations

1. **Authorization**: Merge operations require elevated privileges
2. **Audit Trail**: All merges logged with user, timestamp, strategy
3. **Backup**: Always backup before merge (auto-backup recommended)
4. **Validation**: Validate conflict resolutions before applying
5. **Rate Limiting**: Limit merge API calls to prevent abuse

---

## References

- [Named Snapshots Documentation](features_snapshots.md)
- [Diff API Documentation](features_diff.md)
- [MVCC Tuning Guide](../MVCC_TUNING_GUIDE.md)
- [Transaction Best Practices](TRANSACTION_BEST_PRACTICES.md)

---

**Created:** January 2026  
**Last Updated:** April 2026  
**Version:** 1.5.0  
**Author:** ThemisDB Development Team
