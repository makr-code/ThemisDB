# Persistent Branches - Git-like Branch Management for MVCC

**Version:** 1.5.0  
**Category:** 🌿 Transaction Management  
**Status:** ✅ Implemented (Phase 4 - Optional)

---

## 📑 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [API Reference](#api-reference)
- [Usage Examples](#usage-examples)
- [Integration](#integration)
- [Performance](#performance)
- [Best Practices](#best-practices)

---

## Overview

Persistent Branches bring Git-like branch semantics to ThemisDB's MVCC system. Create named, persistent branches to enable parallel development workflows, schema migration testing, A/B testing scenarios, and what-if analysis.

### Key Capabilities

- **Named Branches**: Create meaningful branch names (e.g., `main`, `feature/new-schema`, `release/v2.0`)
- **Parent Tracking**: Branches track their parent for lineage and merge operations
- **Branch Switching**: Switch between branches to work in different database contexts
- **Fast-Forward Merges**: Merge branches with fast-forward strategy
- **Persistent Storage**: Branches stored in RocksDB, survive database restarts
- **Integration**: Works seamlessly with Snapshots, Diff API, and PITR

### Use Cases

- **Schema Migration Testing**: Create a branch to test schema changes before merging to main
- **A/B Testing**: Maintain parallel data states for experimentation
- **What-If Analysis**: Create branches to explore hypothetical scenarios
- **Parallel Development**: Multiple teams work on isolated branches
- **Release Management**: Create release branches for stable versions

---

## Features

### ✅ Implemented

- Branch CRUD operations (create, read, list, switch, delete)
- Persistent storage in RocksDB
- Branch validation (alphanumeric, hyphens, underscores, forward slashes, 1-128 chars)
- Parent branch tracking
- Active branch management
- Sorting by name, timestamp, or active status
- Create branches from tags, sequences, or timestamps
- Fast-forward merge support
- Statistics API
- REST API endpoints

### 🔜 Future Enhancements

- Three-way merge with conflict detection
- Cherry-pick operations
- Branch rebase
- Branch protection rules
- Branch permissions/access control

---

## API Reference

### Endpoints

#### POST /api/v1/branches

Create a new branch.

**Request Body:**

```json
{
  "branch_name": "feature/new-schema",
  "parent_branch": "main",
  "description": "Branch for testing new user schema",
  "created_by": "alice",
  "from_tag": "v1.0.0",
  "set_active": false
}
```

**Parameters:**
- `branch_name` (required): Unique branch identifier (1-128 chars, alphanumeric, `-`, `_`, `/`)
- `parent_branch` (optional): Parent branch name (default: "main")
- `description` (required): Human-readable description
- `created_by` (optional): User/service that created the branch (default: "system")
- `from_tag` (optional): Create branch from a named snapshot
- `from_sequence` (optional): Create branch from specific sequence number
- `from_timestamp` (optional): Create branch from timestamp
- `set_active` (optional): Make this branch active immediately (default: false)

**Response (201 Created):**

```json
{
  "branch_name": "feature/new-schema",
  "parent_branch": "main",
  "creation_sequence": 12345,
  "creation_timestamp_ms": 1736657231000,
  "description": "Branch for testing new user schema",
  "created_by": "alice",
  "is_active": false
}
```

**Error Responses:**
- `400 Bad Request`: Branch already exists, invalid name, or parent not found
- `500 Internal Server Error`: Database write failure

---

#### GET /api/v1/branches

List all branches.

**Query Parameters:**
- `limit` (optional): Maximum number of branches to return (0 = all, default: 0)
- `sort_by` (optional): Sort field: "name" (default), "timestamp", "active"
- `ascending` (optional): Sort direction: true (default) or false

**Example Request:**

```bash
GET /api/v1/branches?limit=10&sort_by=timestamp&ascending=false
```

**Response (200 OK):**

```json
[
  {
    "branch_name": "main",
    "parent_branch": "",
    "creation_sequence": 0,
    "creation_timestamp_ms": 1736657200000,
    "description": "Default main branch",
    "created_by": "system",
    "is_active": true
  },
  {
    "branch_name": "feature/new-schema",
    "parent_branch": "main",
    "creation_sequence": 12345,
    "creation_timestamp_ms": 1736657231000,
    "description": "Branch for testing new user schema",
    "created_by": "alice",
    "is_active": false
  }
]
```

---

#### GET /api/v1/branches/:name

Get specific branch metadata.

**Example Request:**

```bash
GET /api/v1/branches/feature/new-schema
```

**Response (200 OK):**

```json
{
  "branch_name": "feature/new-schema",
  "parent_branch": "main",
  "creation_sequence": 12345,
  "creation_timestamp_ms": 1736657231000,
  "description": "Branch for testing new user schema",
  "created_by": "alice",
  "is_active": false
}
```

**Error Responses:**
- `404 Not Found`: Branch does not exist

---

#### GET /api/v1/branches/active

Get currently active branch.

**Response (200 OK):**

```json
{
  "active_branch": "main"
}
```

---

#### POST /api/v1/branches/:name/switch

Switch to a different branch.

**Example Request:**

```bash
POST /api/v1/branches/feature/new-schema/switch
```

**Response (200 OK):**

```json
{
  "success": true,
  "message": "Switched to branch: feature/new-schema",
  "active_branch": "feature/new-schema"
}
```

**Error Responses:**
- `400 Bad Request`: Branch does not exist

---

#### POST /api/v1/branches/merge

Merge one branch into another.

**Request Body:**

```json
{
  "source_branch": "feature/new-schema",
  "target_branch": "main",
  "fast_forward": true,
  "abort_on_conflict": true
}
```

**Parameters:**
- `source_branch` (required): Branch to merge from
- `target_branch` (required): Branch to merge into
- `fast_forward` (optional): Allow fast-forward merges (default: true)
- `abort_on_conflict` (optional): Stop on first conflict (default: true)
- `merge_strategy` (optional): Merge strategy name (default: "default")

**Response (200 OK - Fast-Forward):**

```json
{
  "success": true,
  "message": "Fast-forward merge completed",
  "conflicts": [],
  "merged_sequence": 12345
}
```

**Response (409 Conflict - Non-Fast-Forward):**

```json
{
  "success": false,
  "message": "Non-fast-forward merge not yet implemented. Use force merge or rebase source branch.",
  "conflicts": [],
  "merged_sequence": 0
}
```

---

#### DELETE /api/v1/branches/:name

Delete a branch.

**Query Parameters:**
- `force` (optional): Force deletion even if not merged (default: false)

**Example Request:**

```bash
DELETE /api/v1/branches/feature/old-branch?force=true
```

**Response (200 OK):**

```json
{
  "success": true,
  "message": "Branch deleted: feature/old-branch"
}
```

**Error Responses:**
- `400 Bad Request`: Branch is active, is default branch, or not fully merged (without force)

---

#### GET /api/v1/branches/stats

Get branch statistics.

**Response (200 OK):**

```json
{
  "total_branches": 5,
  "active_branches": 1,
  "oldest_creation_timestamp_ms": 1736657200000,
  "newest_creation_timestamp_ms": 1736657500000,
  "default_branch": "main"
}
```

---

## Usage Examples

### Example 1: Basic Branch Workflow

```bash
# Create a feature branch
curl -X POST http://localhost:8080/api/v1/branches \
  -H "Content-Type: application/json" \
  -d '{
    "branch_name": "feature/user-profiles",
    "parent_branch": "main",
    "description": "Add user profile functionality",
    "created_by": "alice"
  }'

# Switch to the feature branch
curl -X POST http://localhost:8080/api/v1/branches/feature/user-profiles/switch

# ... make changes in this branch ...

# Switch back to main
curl -X POST http://localhost:8080/api/v1/branches/main/switch

# Merge feature branch into main
curl -X POST http://localhost:8080/api/v1/branches/merge \
  -H "Content-Type: application/json" \
  -d '{
    "source_branch": "feature/user-profiles",
    "target_branch": "main"
  }'

# Delete feature branch
curl -X DELETE "http://localhost:8080/api/v1/branches/feature/user-profiles?force=true"
```

### Example 2: Create Branch from Tag

```bash
# First, create a snapshot tag
curl -X POST http://localhost:8080/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d '{
    "tag_name": "v1.0.0",
    "description": "Release 1.0"
  }'

# Create branch from that tag
curl -X POST http://localhost:8080/api/v1/branches \
  -H "Content-Type: application/json" \
  -d '{
    "branch_name": "hotfix/security-patch",
    "parent_branch": "main",
    "description": "Security patch for v1.0",
    "from_tag": "v1.0.0"
  }'
```

### Example 3: List and Filter Branches

```bash
# List all branches sorted by creation time
curl "http://localhost:8080/api/v1/branches?sort_by=timestamp&ascending=false"

# List only the 5 most recent branches
curl "http://localhost:8080/api/v1/branches?limit=5&sort_by=timestamp&ascending=false"

# Get current active branch
curl http://localhost:8080/api/v1/branches/active
```

### Example 4: Branch Statistics

```bash
# Get branch statistics
curl http://localhost:8080/api/v1/branches/stats
```

---

## Integration

### Integration with Snapshots

Branches can be created from snapshot tags:

```cpp
// C++ API Example
BranchManager::CreateBranchOptions options;
options.from_tag = "v1.0.0";

auto branch = branch_manager.createBranch(
    "release/v1.0",
    "main",
    "Release branch for v1.0",
    "system",
    options
);
```

### Integration with Diff API

Compare changes between branches:

```bash
# Get the creation sequences of two branches
curl http://localhost:8080/api/v1/branches/feature/new-schema
# Returns: {"creation_sequence": 1000, ...}

curl http://localhost:8080/api/v1/branches/main
# Returns: {"creation_sequence": 500, ...}

# Use Diff API to compare
curl "http://localhost:8080/api/v1/diff?from_sequence=500&to_sequence=1000"
```

### Integration with PITR

Restore to a branch creation point:

```bash
# Get branch creation sequence
curl http://localhost:8080/api/v1/branches/release/v1.0
# Returns: {"creation_sequence": 12345, ...}

# Use PITR to restore to that point
curl -X POST http://localhost:8080/api/v1/restore/pitr \
  -H "Content-Type: application/json" \
  -d '{
    "target_sequence": 12345,
    "dry_run": true
  }'
```

---

## Performance

### Benchmarks

Performance targets for branch operations:

| Operation | Target | Typical |
|-----------|--------|---------|
| Create Branch | < 5ms | ~2ms |
| Get Branch | < 1ms | ~0.5ms |
| List Branches (100) | < 20ms | ~10ms |
| Switch Branch | < 2ms | ~1ms |
| Delete Branch | < 3ms | ~1.5ms |
| Branch Exists | < 0.5ms | ~0.3ms |

### Performance Tips

1. **Minimize Branch Count**: Keep the number of active branches reasonable (< 1000)
2. **Use Pagination**: When listing many branches, use `limit` parameter
3. **Cache Active Branch**: Cache the active branch name in your application
4. **Fast-Forward Merges**: Fast-forward merges are much faster than three-way merges

---

## Best Practices

### Naming Conventions

Follow Git-style naming conventions:

- **Feature branches**: `feature/description` (e.g., `feature/user-auth`)
- **Bugfix branches**: `bugfix/issue-number` (e.g., `bugfix/issue-123`)
- **Release branches**: `release/version` (e.g., `release/v2.0`)
- **Hotfix branches**: `hotfix/description` (e.g., `hotfix/security-patch`)

### Branch Lifecycle

1. **Create**: Create branch from main or a specific tag
2. **Develop**: Switch to branch and make changes
3. **Test**: Validate changes in the branch
4. **Merge**: Merge back to main when ready
5. **Delete**: Delete branch after successful merge

### Reserved Names

Avoid using these reserved names:
- `HEAD`
- `FETCH_HEAD`
- `ORIG_HEAD`

### Branch Deletion

- **Cannot delete**: Default branch (`main`)
- **Cannot delete**: Currently active branch
- **Safety**: Branches must be fully merged unless using `force=true`

### Branch Merging

Currently, only fast-forward merges are supported:
- Source branch creation sequence must be >= target branch creation sequence
- Full three-way merge with conflict resolution is planned for future releases

### Integration with CI/CD

Branches integrate well with CI/CD workflows:

```yaml
# Example GitHub Actions workflow
on:
  push:
    branches: [ feature/* ]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - name: Create test branch
        run: |
          curl -X POST http://db:8080/api/v1/branches \
            -d '{"branch_name": "test/${{ github.sha }}", "parent_branch": "main"}'
      
      - name: Run tests on branch
        run: |
          curl -X POST http://db:8080/api/v1/branches/test/${{ github.sha }}/switch
          # Run your tests here
```

---

## Troubleshooting

### Branch Already Exists

**Error**: `Failed to create branch. Branch may already exist.`

**Solution**: Check if branch exists first:

```bash
curl http://localhost:8080/api/v1/branches/feature/my-branch
```

### Cannot Delete Branch

**Error**: `Failed to delete branch. Branch may be active or not fully merged.`

**Solutions**:
1. Switch to a different branch first
2. Use `force=true` to force deletion
3. Ensure branch is not the default branch

### Parent Branch Not Found

**Error**: `Failed to create branch. Parent branch not found.`

**Solution**: Verify parent branch exists:

```bash
curl http://localhost:8080/api/v1/branches
```

---

## See Also

- [Named Snapshots](features_snapshots.md) - Create semantic tags for database states
- [Diff API](features_diff.md) - Compare changes between branches
- [Point-in-Time Recovery](features_pitr.md) - Restore to specific points in time
- [Transaction Best Practices](TRANSACTION_BEST_PRACTICES.md) - MVCC transaction patterns

---

**Last Updated:** April 2026  
**Documentation Version:** 1.0
