# Named Snapshots (Semantic Tagging)

**Version:** 1.4.0+  
**Status:** ✅ Production Ready  
**Category:** MVCC, Disaster Recovery, Compliance

---

## Overview

Named Snapshots is a git-like feature for ThemisDB's MVCC (Multi-Version Concurrency Control) system that enables semantic tagging of important database states. This feature provides named, persistent markers for critical database states, making disaster recovery, compliance auditing, and schema migrations safer and more manageable.

### Key Benefits

- 🔄 **Disaster Recovery**: Create named recovery points before critical operations
- 📋 **Compliance**: Maintain audit-ready snapshots for regulatory requirements
- 🚀 **DevOps**: Safe deployment checkpoints with easy rollback
- 🧪 **Testing**: Repeatable test states with semantic names
- 🔧 **Schema Migrations**: Named rollback points for database schema changes

---

## Core Concepts

### What is a Named Snapshot?

A named snapshot is a semantic tag that marks a specific point in the database's transaction history. Each snapshot captures:

- **Tag Name**: A unique, human-readable identifier (e.g., `before_q1_2026_migration`)
- **Sequence Number**: The MVCC sequence number at snapshot time
- **Timestamp**: Precise moment when the snapshot was created
- **Description**: Optional human-readable context
- **Creator**: User or system that created the snapshot

### Storage Model

Snapshots are stored persistently in RocksDB with the following format:

```
Key: tags:{tag_name}
Value: {
  "tag_name": "before_migration_2026_q1",
  "sequence_number": 12345,
  "timestamp_ms": 1736629200000,
  "description": "Snapshot before Q1 2026 schema migration",
  "created_by": "admin"
}
```

---

## REST API Reference

All snapshot endpoints are under the `/api/v1/snapshots/` namespace.

### Create Snapshot

**Endpoint:** `POST /api/v1/snapshots/tags`

Creates a new named snapshot at the current database state.

**Request Body:**
```json
{
  "tag_name": "before_migration_2026_q1",
  "description": "Snapshot before Q1 2026 schema migration",
  "created_by": "admin"
}
```

**Response (201 Created):**
```json
{
  "tag_name": "before_migration_2026_q1",
  "sequence_number": 12345,
  "timestamp_ms": 1736629200000,
  "description": "Snapshot before Q1 2026 schema migration",
  "created_by": "admin"
}
```

**Example:**
```bash
curl -X POST http://localhost:8765/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d '{
    "tag_name": "before_migration_2026_q1",
    "description": "Snapshot before Q1 2026 schema migration",
    "created_by": "admin"
  }'
```

### List Snapshots

**Endpoint:** `GET /api/v1/snapshots/tags`

Returns all snapshots, sorted by timestamp (newest first).

**Response (200 OK):**
```json
{
  "tags": [
    {
      "tag_name": "before_migration_2026_q1",
      "sequence_number": 12345,
      "timestamp_ms": 1736629200000,
      "description": "Snapshot before Q1 2026 schema migration",
      "created_by": "admin"
    },
    {
      "tag_name": "quarterly_backup_2025_q4",
      "sequence_number": 10000,
      "timestamp_ms": 1704067200000,
      "description": "Q4 2025 quarterly backup",
      "created_by": "system"
    }
  ],
  "total": 2
}
```

**Example:**
```bash
curl http://localhost:8765/api/v1/snapshots/tags
```

### Get Specific Snapshot

**Endpoint:** `GET /api/v1/snapshots/tags/:name`

Retrieves a specific snapshot by its tag name.

**Response (200 OK):**
```json
{
  "tag_name": "before_migration_2026_q1",
  "sequence_number": 12345,
  "timestamp_ms": 1736629200000,
  "description": "Snapshot before Q1 2026 schema migration",
  "created_by": "admin"
}
```

**Example:**
```bash
curl http://localhost:8765/api/v1/snapshots/tags/before_migration_2026_q1
```

### Delete Snapshot

**Endpoint:** `DELETE /api/v1/snapshots/tags/:name`

Deletes a snapshot tag.

**Response (200 OK):**
```json
{
  "message": "Tag 'before_migration_2026_q1' deleted successfully"
}
```

**Example:**
```bash
curl -X DELETE http://localhost:8765/api/v1/snapshots/tags/before_migration_2026_q1
```

### Get Statistics

**Endpoint:** `GET /api/v1/snapshots/stats`

Returns statistics about all snapshots.

**Response (200 OK):**
```json
{
  "total_tags": 5,
  "oldest_sequence": 100,
  "newest_sequence": 12345,
  "oldest_timestamp_ms": 1704067200000,
  "newest_timestamp_ms": 1736629200000
}
```

**Example:**
```bash
curl http://localhost:8765/api/v1/snapshots/stats
```

---

## Usage Examples

### Disaster Recovery Scenario

```bash
# 1. Create snapshot before critical operation
curl -X POST http://localhost:8765/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d '{
    "tag_name": "before_data_import_2026_01_12",
    "description": "Before importing 1M customer records",
    "created_by": "data_team"
  }'

# 2. Perform the critical operation
# ... import data ...

# 3. If something goes wrong, you can reference this snapshot
#    for recovery operations (future PITR feature will use this)
```

### Compliance Auditing

```bash
# Create quarterly compliance snapshots
curl -X POST http://localhost:8765/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d '{
    "tag_name": "compliance_q1_2026",
    "description": "Q1 2026 compliance snapshot for audit",
    "created_by": "compliance_system"
  }'

# List all compliance snapshots
curl http://localhost:8765/api/v1/snapshots/tags | jq '.tags[] | select(.tag_name | startswith("compliance_"))'
```

### Schema Migration Workflow

```bash
# 1. Create pre-migration snapshot
curl -X POST http://localhost:8765/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d '{
    "tag_name": "pre_schema_v2_0",
    "description": "Before upgrading to schema v2.0",
    "created_by": "migration_script"
  }'

# 2. Run migration
# ... apply schema changes ...

# 3. Create post-migration snapshot
curl -X POST http://localhost:8765/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d '{
    "tag_name": "post_schema_v2_0",
    "description": "After upgrading to schema v2.0",
    "created_by": "migration_script"
  }'

# 4. Verify by listing both snapshots
curl http://localhost:8765/api/v1/snapshots/tags | jq '.tags[] | select(.tag_name | contains("schema_v2_0"))'
```

---

## Configuration

### Enabling Named Snapshots

Named Snapshots requires the CDC (Change Data Capture) feature to be enabled:

```cpp
// In HttpServer configuration
HttpServer::Config config;
config.feature_cdc = true;  // Required for snapshots
```

Or via environment variable:
```bash
THEMIS_ENABLE_CDC=true
```

### Tag Name Validation

Tag names must follow these rules:

- **Allowed characters**: Alphanumeric (a-z, A-Z, 0-9), hyphens (-), underscores (_)
- **Maximum length**: 128 characters
- **Minimum length**: 1 character
- **Uniqueness**: Tag names must be unique

**Valid examples:**
- `before_migration_2026_q1`
- `quarterly-backup-2025-Q4`
- `test_state_001`

**Invalid examples:**
- `before migration` (spaces not allowed)
- `backup@2026` (special characters not allowed)
- `` (empty string)

---

## Performance Characteristics

### Operations Complexity

| Operation | Time Complexity | Notes |
|-----------|----------------|-------|
| Create Tag | O(1) | Direct RocksDB put operation |
| Get Tag | O(1) | Direct RocksDB get operation |
| List Tags | O(n) | Sequential scan, sorted in-memory |
| Delete Tag | O(1) | Direct RocksDB delete operation |
| Get Stats | O(n) | Sequential scan for statistics |

### Resource Usage

- **Storage**: ~200-500 bytes per snapshot (depends on description length)
- **Memory**: Minimal (no caching, direct RocksDB access)
- **CPU**: Negligible for typical workloads (<1000 snapshots)

### Scalability

- Tested with 1000+ snapshots without performance degradation
- List operation remains fast (<50ms) with 1000 snapshots
- Thread-safe with no lock contention under normal load

---

## Best Practices

### Naming Conventions

Use consistent, descriptive naming patterns:

```
# Date-based
YYYY_MM_DD_description
before_YYYY_MM_DD_event

# Event-based
before_operation_description
after_operation_description

# Compliance
compliance_period_identifier
audit_YYYY_QX
```

### Snapshot Lifecycle

1. **Create**: Before critical operations
2. **Document**: Include meaningful descriptions
3. **Verify**: Check snapshot exists after creation
4. **Cleanup**: Delete old snapshots when no longer needed
5. **Audit**: Maintain compliance snapshots per policy

### Integration with CI/CD

```bash
#!/bin/bash
# Example deployment script

# Create pre-deployment snapshot
SNAPSHOT_NAME="pre_deploy_$(date +%Y%m%d_%H%M%S)"
curl -X POST http://db.example.com/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d "{
    \"tag_name\": \"$SNAPSHOT_NAME\",
    \"description\": \"Pre-deployment snapshot for build $BUILD_ID\",
    \"created_by\": \"ci_cd_pipeline\"
  }"

# Deploy application
./deploy.sh

# Verify deployment
if ./verify.sh; then
  echo "Deployment successful"
else
  echo "Deployment failed - rollback to $SNAPSHOT_NAME recommended"
  exit 1
fi
```

---

## Error Handling

### Common Errors

**400 Bad Request**
- Invalid tag name format
- Empty tag name
- Description too long (>1024 chars)
- Missing required fields

**404 Not Found**
- Tag does not exist (GET/DELETE operations)

**409 Conflict**
- Tag name already exists (CREATE operation)

**503 Service Unavailable**
- CDC feature not enabled
- Database not ready

### Error Response Format

```json
{
  "error": "Tag 'invalid@name' contains invalid characters (use only alphanumeric, hyphens, underscores)"
}
```

---

## Security Considerations

### Access Control

Snapshot operations should be restricted to authorized users:

- Create: Requires `snapshot:write` permission
- List/Get: Requires `snapshot:read` permission
- Delete: Requires `snapshot:delete` permission
- Stats: Requires `snapshot:read` permission

### Audit Logging

All snapshot operations are logged to the audit trail:
- Who created/deleted snapshots
- When operations occurred
- What changes were made

### Data Privacy

- Snapshots contain only metadata, not actual data
- Tag names and descriptions should not contain sensitive information
- Follow your organization's data classification policies

---

## Limitations

### Current Limitations

1. **No automatic retention policy**: Manual cleanup required
2. **No restore functionality**: Point-in-Time Recovery (PITR) planned for Phase 3
3. **No snapshot validation**: No verification of snapshot integrity
4. **No compression**: Metadata stored as plain JSON

### Future Enhancements (Planned)

- **Phase 2**: Diff API - Compare database states between snapshots
- **Phase 3**: Point-in-Time Recovery - Restore to any snapshot
- **Future**: Automatic retention policies with configurable rules
- **Future**: Snapshot export/import for backup portability

---

## Troubleshooting

### Snapshot Not Created

**Symptom**: POST request returns error

**Solutions**:
1. Check CDC is enabled: `config.feature_cdc = true`
2. Verify tag name follows validation rules
3. Check description length (<1024 chars)
4. Ensure unique tag name

### Cannot List Snapshots

**Symptom**: GET request returns empty or error

**Solutions**:
1. Verify CDC feature is enabled
2. Check database is running and accessible
3. Verify no RocksDB corruption

### Performance Degradation

**Symptom**: Slow list operations

**Solutions**:
1. Check snapshot count (>10000 may be slow)
2. Consider implementing retention policy
3. Delete unused old snapshots

---

## Related Features

- **Change Data Capture (CDC)**: Foundation for snapshots
- **Changefeed**: Transaction history tracking
- **Temporal Graphs**: Time-based graph queries
- **Audit Logging**: Compliance and security tracking

---

## References

- [Implementation Plan](../../research/IMPLEMENTATION_PLAN_GIT_FEATURES.md)
- [Git-like Features Research](../../research/GIT_LIKE_FEATURES_FOR_MVCC.md)
- [MVCC Architecture](../../architecture/architecture_mvcc.md)
- [Changefeed Documentation](features_cdc.md)

---

## Support

For issues or questions:
- GitHub Issues: [ThemisDB Issues](https://github.com/makr-code/ThemisDB/issues)
- Documentation: [ThemisDB Docs](https://github.com/makr-code/ThemisDB/tree/main/docs)

---

**Last Updated**: 2026-01-12  
**Version**: 1.4.0+
