# Named Snapshots - Semantic Tagging for MVCC

**Version:** 1.4.0  
**Category:** 🏷️ Transaction Management  
**Status:** ✅ Implemented (Phase 1)

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

Named Snapshots provide Git-like semantic tagging for ThemisDB's MVCC system. Create meaningful labels for database states to enable audit trails, deployment checkpoints, and point-in-time recovery.

### Key Capabilities

- **Semantic Tags**: Label database states with meaningful names (e.g., `v1.0.0`, `pre-migration`, `release-2024-01-12`)
- **Persistent Storage**: Tags stored in RocksDB, survive database restarts
- **Tag-based Diff**: Compute differences between tagged states (`GET /api/v1/diff?from_tag=v1.0&to_tag=v2.0`)
- **Audit Trail**: Track who created tags and when
- **Flexible Sorting**: List tags by timestamp, sequence, or name

### Use Cases

- **Deployment Checkpoints**: Tag database state before deployments
- **Audit/Compliance**: Create named snapshots for regulatory requirements
- **Testing**: Mark known-good states for test rollback
- **Release Management**: Tag each release for version tracking
- **Debugging**: Create snapshots before/after incidents

---

## Features

### ✅ Implemented

- Tag CRUD operations (create, read, list, delete)
- Persistent storage in RocksDB
- Tag validation (alphanumeric, hyphens, underscores, periods, 1-128 chars)
- Sorting by timestamp, sequence, or name (ascending/descending)
- Pagination support
- Statistics API
- Integration with Diff API for tag-based diff
- REST API endpoints

### 🔜 Future Enhancements

- Tag metadata (custom key-value pairs)
- Tag expiration/TTL
- Tag categories/namespaces
- Bulk tag operations

---

## API Reference

### Endpoints

#### POST /api/v1/snapshots/tags

Create a new snapshot tag.

**Request Body:**

```json
{
  "tag_name": "v1.0.0",
  "description": "Release 1.0 - Initial production release",
  "created_by": "admin"
}
```

**Parameters:**
- `tag_name` (required): Unique tag identifier (1-128 chars, alphanumeric, `-`, `_`, `.`)
- `description` (required): Human-readable description
- `created_by` (optional): User/service that created the tag (default: "system")

**Response (201 Created):**

```json
{
  "tag_name": "v1.0.0",
  "sequence_number": 12345,
  "timestamp_ms": 1736657231000,
  "description": "Release 1.0 - Initial production release",
  "created_by": "admin"
}
```

**Error Responses:**
- `400 Bad Request`: Invalid tag name or missing fields
- `409 Conflict`: Tag already exists

---

#### GET /api/v1/snapshots/tags

List all snapshot tags.

**Query Parameters:**
- `limit` (optional): Maximum tags to return (default: 0 = all)
- `sort_by` (optional): Sort field - `timestamp` (default), `sequence`, `name`
- `ascending` (optional): Sort direction - `true` or `false` (default: `false`)

**Response (200 OK):**

```json
[
  {
    "tag_name": "v2.0.0",
    "sequence_number": 50000,
    "timestamp_ms": 1736744231000,
    "description": "Release 2.0",
    "created_by": "admin"
  },
  {
    "tag_name": "v1.0.0",
    "sequence_number": 12345,
    "timestamp_ms": 1736657231000,
    "description": "Release 1.0",
    "created_by": "admin"
  }
]
```

---

#### GET /api/v1/snapshots/tags/:name

Get a specific snapshot tag.

**Response (200 OK):**

```json
{
  "tag_name": "v1.0.0",
  "sequence_number": 12345,
  "timestamp_ms": 1736657231000,
  "description": "Release 1.0 - Initial production release",
  "created_by": "admin"
}
```

**Error Response:**
- `404 Not Found`: Tag does not exist

---

#### DELETE /api/v1/snapshots/tags/:name

Delete a snapshot tag.

**Response (200 OK):**

```json
{
  "status": "success",
  "message": "Tag 'v1.0.0' deleted successfully"
}
```

**Error Response:**
- `404 Not Found`: Tag does not exist

---

#### GET /api/v1/snapshots/stats

Get snapshot statistics.

**Response (200 OK):**

```json
{
  "total_snapshots": 5,
  "oldest_timestamp_ms": 1736657231000,
  "newest_timestamp_ms": 1736744231000,
  "oldest_sequence": 12345,
  "newest_sequence": 50000
}
```

---

## Usage Examples

### Example 1: Create a Release Snapshot

```bash
curl -X POST http://localhost:8765/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d '{
    "tag_name": "v1.0.0",
    "description": "Release 1.0 - Initial production release",
    "created_by": "admin"
  }'
```

### Example 2: Create Pre-Deployment Snapshot

```bash
curl -X POST http://localhost:8765/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d '{
    "tag_name": "pre-deploy-2024-01-12",
    "description": "State before deployment on 2024-01-12",
    "created_by": "deploy-bot"
  }'
```

### Example 3: List All Snapshots (Newest First)

```bash
curl http://localhost:8765/api/v1/snapshots/tags?sort_by=timestamp&ascending=false
```

### Example 4: List Last 10 Snapshots

```bash
curl http://localhost:8765/api/v1/snapshots/tags?limit=10&sort_by=timestamp&ascending=false
```

### Example 5: Get Specific Snapshot

```bash
curl http://localhost:8765/api/v1/snapshots/tags/v1.0.0
```

### Example 6: Delete Old Snapshot

```bash
curl -X DELETE http://localhost:8765/api/v1/snapshots/tags/old-snapshot
```

### Example 7: Get Statistics

```bash
curl http://localhost:8765/api/v1/snapshots/stats
```

### Example 8: Tag-based Diff (Integration with Diff API)

```bash
# Compare changes between two tagged versions
curl "http://localhost:8765/api/v1/diff?from_tag=v1.0&to_tag=v2.0"
```

### Example 9: Using Python

```python
import requests
import json

# Create a snapshot
def create_snapshot(tag_name, description, created_by="system"):
    url = "http://localhost:8765/api/v1/snapshots/tags"
    data = {
        "tag_name": tag_name,
        "description": description,
        "created_by": created_by
    }
    response = requests.post(url, json=data)
    return response.json()

# Create release snapshot
snapshot = create_snapshot(
    tag_name="v1.0.0",
    description="Release 1.0",
    created_by="admin"
)
print(f"Created snapshot: {snapshot['tag_name']} at sequence {snapshot['sequence_number']}")

# List all snapshots
def list_snapshots(limit=0, sort_by="timestamp", ascending=False):
    url = "http://localhost:8765/api/v1/snapshots/tags"
    params = {
        "limit": limit,
        "sort_by": sort_by,
        "ascending": str(ascending).lower()
    }
    response = requests.get(url, params=params)
    return response.json()

# Get last 5 snapshots
snapshots = list_snapshots(limit=5)
for snap in snapshots:
    print(f"- {snap['tag_name']}: {snap['description']}")

# Compare versions
def diff_between_tags(from_tag, to_tag):
    url = "http://localhost:8765/api/v1/diff"
    params = {
        "from_tag": from_tag,
        "to_tag": to_tag
    }
    response = requests.get(url, params=params)
    return response.json()

# Get differences between releases
diff = diff_between_tags("v1.0.0", "v2.0.0")
print(f"Changes: {diff['stats']['total_changes']}")
print(f"  Added: {diff['stats']['added_count']}")
print(f"  Modified: {diff['stats']['modified_count']}")
print(f"  Deleted: {diff['stats']['deleted_count']}")
```

---

## Integration

### With Diff API

Snapshots integrate seamlessly with the Diff API to enable tag-based diffs:

```bash
# Compare database states between two tags
GET /api/v1/diff?from_tag=v1.0&to_tag=v2.0

# With filters
GET /api/v1/diff?from_tag=v1.0&to_tag=v2.0&table=users&limit=100
```

### Deployment Workflow Example

```bash
# 1. Create snapshot before deployment
curl -X POST http://localhost:8765/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d '{
    "tag_name": "pre-deploy-$(date +%Y%m%d-%H%M%S)",
    "description": "Pre-deployment snapshot",
    "created_by": "ci-cd"
  }'

# 2. Perform deployment
./deploy.sh

# 3. Create snapshot after deployment
curl -X POST http://localhost:8765/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d '{
    "tag_name": "post-deploy-$(date +%Y%m%d-%H%M%S)",
    "description": "Post-deployment snapshot",
    "created_by": "ci-cd"
  }'

# 4. Review changes
curl "http://localhost:8765/api/v1/diff?from_tag=pre-deploy-*&to_tag=post-deploy-*"
```

---

## Performance

### Benchmarks

| Operation | Target | Actual | Dataset | Notes |
|-----------|--------|--------|---------|-------|
| Create Tag | <1ms | ~0.5ms | N/A | Single tag creation |
| Get Tag | <0.5ms | ~0.2ms | N/A | Single tag retrieval |
| List Tags (10) | <5ms | ~2ms | 10 tags | List all tags |
| List Tags (100) | <10ms | ~8ms | 100 tags | List all tags |
| List Tags (1000) | <50ms | ~40ms | 1000 tags | List all tags |
| Delete Tag | <1ms | ~0.5ms | N/A | Single tag deletion |
| Tag Exists | <0.1ms | ~0.05ms | N/A | Existence check |
| Get Statistics | <5ms | ~3ms | 100 tags | Compute stats |

### Optimization Tips

1. **Use Pagination**: For large tag collections, use `limit` parameter
2. **Sort Once**: Choose appropriate sort order upfront
3. **Cache Locally**: Cache frequently accessed tags in application layer
4. **Batch Operations**: Create multiple tags in batch when possible
5. **Delete Old Tags**: Regularly clean up obsolete snapshots

---

## Best Practices

### Tag Naming Conventions

```bash
# ✅ Good: Clear, semantic names
v1.0.0
v2.1.3-beta
release-2024-01-12
pre-migration-users
post-rollback-incident-123

# ❌ Bad: Unclear, generic names
tag1
snapshot
backup
test
```

### Use Descriptive Descriptions

```bash
# ✅ Good: Detailed descriptions
curl -X POST http://localhost:8765/api/v1/snapshots/tags \
  -d '{
    "tag_name": "v1.0.0",
    "description": "Release 1.0 - Includes user auth, API v1, and dashboard. Deployed to production on 2024-01-12 14:30 UTC.",
    "created_by": "admin"
  }'

# ❌ Bad: Vague descriptions
curl -X POST http://localhost:8765/api/v1/snapshots/tags \
  -d '{
    "tag_name": "v1.0.0",
    "description": "Release",
    "created_by": "admin"
  }'
```

### Snapshot Lifecycle Management

```python
# Regular cleanup of old snapshots
import requests
from datetime import datetime, timedelta

def cleanup_old_snapshots(days_to_keep=90):
    """Delete snapshots older than specified days"""
    url = "http://localhost:8765/api/v1/snapshots/tags"
    response = requests.get(url)
    snapshots = response.json()
    
    cutoff_time = datetime.now() - timedelta(days=days_to_keep)
    cutoff_ms = int(cutoff_time.timestamp() * 1000)
    
    for snap in snapshots:
        if snap['timestamp_ms'] < cutoff_ms:
            delete_url = f"{url}/{snap['tag_name']}"
            requests.delete(delete_url)
            print(f"Deleted old snapshot: {snap['tag_name']}")

# Run weekly
cleanup_old_snapshots(days_to_keep=90)
```

### Audit Trail Pattern

```python
# Track all tag operations
def create_audited_snapshot(tag_name, description, user, reason):
    """Create snapshot with full audit trail"""
    import socket
    import getpass
    
    full_description = f"{description} | Reason: {reason} | Host: {socket.gethostname()}"
    
    data = {
        "tag_name": tag_name,
        "description": full_description,
        "created_by": user
    }
    
    response = requests.post(
        "http://localhost:8765/api/v1/snapshots/tags",
        json=data
    )
    return response.json()

# Usage
create_audited_snapshot(
    tag_name="pre-migration-2024-01-12",
    description="User table migration",
    user="admin@example.com",
    reason="TICKET-123: Prepare for schema update"
)
```

---

## Error Handling

### Common Errors

#### Tag Already Exists

```json
{
  "error": "Failed to create tag 'v1.0.0'. Tag may already exist or name is invalid.",
  "status": 409
}
```

**Solution**: Choose a different tag name or delete the existing tag first.

#### Invalid Tag Name

```json
{
  "error": "Failed to create tag 'invalid tag!'. Tag may already exist or name is invalid.",
  "status": 409
}
```

**Solution**: Use only alphanumeric characters, hyphens, underscores, and periods.

#### Tag Not Found

```json
{
  "error": "Tag 'nonexistent' not found",
  "status": 404
}
```

**Solution**: Verify the tag exists using `GET /api/v1/snapshots/tags`.

---

## See Also

- [Diff API Documentation](./features_diff.md)
- [MVCC Architecture](../architecture/architecture_mvcc.md)
- [Changefeed Documentation](../cdc/changefeed.md)
- [Point-in-Time Recovery](./features_pitr.md)
- [Git-like Features Research](../../research/GIT_LIKE_FEATURES_FOR_MVCC.md)

---

**Created:** 2026-01-12  
**Last Updated:** 2026-04-06  
**Version:** 1.0  
**Status:** Production Ready ✅
