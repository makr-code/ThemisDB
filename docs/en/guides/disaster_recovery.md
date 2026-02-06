# Disaster Recovery Guide for ThemisDB

**Version:** 1.0  
**Date:** February 6, 2026  
**Category:** Operations Guide  
**Status:** ✅ Production Ready

---

## Table of Contents

- [Overview](#overview)
- [Quick Start](#quick-start)
- [Recovery Scenarios](#recovery-scenarios)
- [Best Practices](#best-practices)
- [Troubleshooting](#troubleshooting)
- [Appendix](#appendix)

---

## Overview

This guide provides comprehensive procedures for recovering ThemisDB from various disaster scenarios using the Point-in-Time Recovery (PITR) system.

### When to Use This Guide

- **Data Corruption**: Database corruption after failed operations
- **Accidental Deletions**: Critical data accidentally removed
- **Failed Migrations**: Schema or data migrations that went wrong
- **Security Incidents**: Recovery after security breaches
- **Testing/Development**: Restoring to known-good states

### Key Capabilities

ThemisDB's PITR system provides Git-like recovery features:

- ✅ **Named Snapshots**: Create semantic tags at important points
- ✅ **Time Travel**: Restore to any sequence, tag, or timestamp
- ✅ **Preview Mode**: See what will change before restoring
- ✅ **Selective Restore**: Restore only specific tables
- ✅ **Auto-Backup**: Automatic backup before restore operations

---

## Quick Start

### 1. Create a Snapshot (Safe Point)

Before any risky operation, create a named snapshot:

```bash
curl -X POST http://localhost:8080/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d '{
    "tag_name": "before_migration",
    "description": "Safe point before schema migration",
    "created_by": "admin"
  }'
```

**Response:**
```json
{
  "tag_name": "before_migration",
  "sequence_number": 12345,
  "timestamp_ms": 1707217330417,
  "description": "Safe point before schema migration",
  "created_by": "admin"
}
```

### 2. Preview Restore Operation

Before restoring, preview what will happen:

```bash
curl -X POST http://localhost:8080/api/v1/restore/preview \
  -H "Content-Type: application/json" \
  -d '{
    "target": {
      "type": "tag",
      "value": "before_migration"
    }
  }'
```

**Response:**
```json
{
  "target_sequence": 12345,
  "current_sequence": 12567,
  "events_to_replay": 222,
  "affected_tables": ["users", "orders", "products"],
  "affected_keys": ["users:1", "users:2", "orders:100", "..."],
  "estimated_duration_sec": 5,
  "estimated_size_bytes": 524288
}
```

### 3. Execute Restore

Restore to the snapshot:

```bash
curl -X POST http://localhost:8080/api/v1/restore/pitr \
  -H "Content-Type: application/json" \
  -d '{
    "target": {
      "type": "tag",
      "value": "before_migration"
    },
    "options": {
      "create_backup": true,
      "dry_run": false,
      "backup_tag": "auto_backup_20260206"
    }
  }'
```

**Response:**
```json
{
  "ok": true,
  "message": "Restore completed successfully",
  "progress": {
    "phase": 6,
    "events_processed": 222,
    "total_events": 222,
    "progress_percent": 100.0,
    "elapsed_ms": 4832
  }
}
```

---

## Recovery Scenarios

### Scenario 1: Data Corruption After Failed Deployment

**Problem**: Database corrupted after deploying new application version.

**Solution**: Restore to pre-deployment snapshot.

#### Step-by-Step

1. **Identify the safe snapshot**:
```bash
curl http://localhost:8080/api/v1/snapshots/tags
```

2. **Stop application** (optional but recommended):
```bash
systemctl stop myapp
```

3. **Restore database**:
```bash
curl -X POST http://localhost:8080/api/v1/restore/pitr \
  -H "Content-Type: application/json" \
  -d '{
    "target": {
      "type": "tag",
      "value": "pre_deployment_v2.0"
    },
    "options": {
      "create_backup": true,
      "abort_on_first_error": true
    }
  }'
```

4. **Verify restore**:
```bash
# Check database state
curl http://localhost:8080/api/v1/health

# Check specific data
curl http://localhost:8080/api/v1/entities/users
```

5. **Restart application**:
```bash
systemctl start myapp
```

---

### Scenario 2: Accidental Mass Deletion

**Problem**: Critical data accidentally deleted by user or script.

**Solution**: Restore to point in time before deletion.

#### Step-by-Step

1. **Determine deletion time**:
```bash
# Check changefeed for deletion events
curl "http://localhost:8080/api/v1/changefeed?event_type=DELETE&limit=10"
```

2. **Find safe restore point**:
```bash
# List recent snapshots
curl "http://localhost:8080/api/v1/snapshots/tags?limit=10"
```

3. **Preview restore**:
```bash
curl -X POST http://localhost:8080/api/v1/restore/preview \
  -H "Content-Type: application/json" \
  -d '{
    "target": {
      "type": "timestamp",
      "value": 1707216000000
    }
  }'
```

4. **Execute restore**:
```bash
curl -X POST http://localhost:8080/api/v1/restore/pitr \
  -H "Content-Type: application/json" \
  -d '{
    "target": {
      "type": "timestamp",
      "value": 1707216000000
    },
    "options": {
      "create_backup": true,
      "backup_tag": "before_deletion_recovery"
    }
  }'
```

---

### Scenario 3: Failed Schema Migration

**Problem**: Schema migration script failed, database in inconsistent state.

**Solution**: Rollback to pre-migration snapshot.

#### Step-by-Step

1. **Stop migration process**:
```bash
# Kill migration script if still running
pkill -f migration_script.py
```

2. **Restore to pre-migration state**:
```bash
curl -X POST http://localhost:8080/api/v1/restore/pitr \
  -H "Content-Type: application/json" \
  -d '{
    "target": {
      "type": "tag",
      "value": "pre_schema_migration_v5"
    },
    "options": {
      "create_backup": true,
      "abort_on_first_error": true
    }
  }'
```

3. **Verify database schema**:
```bash
curl http://localhost:8080/api/v1/schema
```

4. **Fix migration script**:
```bash
# Review and fix migration errors
vim migration_script.py
```

5. **Retry migration** (after creating new snapshot):
```bash
# Create new safe point
curl -X POST http://localhost:8080/api/v1/snapshots/tags \
  -d '{"tag_name":"pre_migration_retry","description":"Before retry"}'

# Run migration
python migration_script.py
```

---

### Scenario 4: Selective Table Restore

**Problem**: Only one table needs restoration, others should remain unchanged.

**Solution**: Use selective restore with table filter.

#### Step-by-Step

1. **Identify affected table**:
```bash
# Assume "orders" table needs restoration
TABLE="orders"
```

2. **Preview selective restore**:
```bash
curl -X POST http://localhost:8080/api/v1/restore/preview \
  -H "Content-Type: application/json" \
  -d '{
    "target": {
      "type": "tag",
      "value": "hourly_backup_14h00"
    },
    "options": {
      "tables": ["orders"]
    }
  }'
```

3. **Execute selective restore**:
```bash
curl -X POST http://localhost:8080/api/v1/restore/pitr \
  -H "Content-Type: application/json" \
  -d '{
    "target": {
      "type": "tag",
      "value": "hourly_backup_14h00"
    },
    "options": {
      "create_backup": true,
      "tables": ["orders"],
      "backup_tag": "before_orders_restore"
    }
  }'
```

4. **Verify only affected table changed**:
```bash
# Check orders table
curl http://localhost:8080/api/v1/entities/orders | jq '.count'

# Verify other tables unchanged
curl http://localhost:8080/api/v1/entities/users | jq '.count'
curl http://localhost:8080/api/v1/entities/products | jq '.count'
```

---

### Scenario 5: Complete Database Restore

**Problem**: Complete database loss or catastrophic corruption.

**Solution**: Restore from most recent backup.

#### Step-by-Step

1. **Stop ThemisDB**:
```bash
systemctl stop themisdb
```

2. **Restore database files from backup**:
```bash
# Restore RocksDB files
rsync -av /backup/themisdb/data/ /var/lib/themisdb/data/
```

3. **Start ThemisDB**:
```bash
systemctl start themisdb
```

4. **Verify database**:
```bash
curl http://localhost:8080/api/v1/health
curl http://localhost:8080/api/v1/snapshots/tags
```

5. **Find latest snapshot and restore if needed**:
```bash
# List all tags
curl http://localhost:8080/api/v1/snapshots/tags

# Restore to latest known good state if necessary
curl -X POST http://localhost:8080/api/v1/restore/pitr \
  -d '{"target":{"type":"tag","value":"latest_good_state"}}'
```

---

## Best Practices

### 1. Regular Snapshots

Create snapshots at regular intervals and before major operations:

```bash
# Hourly snapshots (via cron)
0 * * * * curl -X POST http://localhost:8080/api/v1/snapshots/tags \
  -d "{\"tag_name\":\"hourly_$(date +\%Y\%m\%d_\%H00)\",\"description\":\"Hourly backup\"}"

# Before deployments
curl -X POST http://localhost:8080/api/v1/snapshots/tags \
  -d '{"tag_name":"pre_deploy_v1.2.3","description":"Before v1.2.3 deployment"}'

# Before schema changes
curl -X POST http://localhost:8080/api/v1/snapshots/tags \
  -d '{"tag_name":"pre_schema_change","description":"Before adding users.email column"}'
```

### 2. Always Preview First

**Never restore without previewing**:

```bash
# Preview shows what will change
curl -X POST http://localhost:8080/api/v1/restore/preview \
  -d '{"target":{"type":"tag","value":"my_snapshot"}}'

# Review the output carefully:
# - events_to_replay: How many changes will be reversed?
# - affected_tables: Which tables will change?
# - estimated_duration_sec: How long will it take?

# Only proceed if preview looks correct
```

### 3. Use Auto-Backup

Always enable auto-backup for production restores:

```json
{
  "options": {
    "create_backup": true,
    "backup_tag": "before_restore_20260206",
    "abort_on_first_error": true
  }
}
```

### 4. Monitor Restore Progress

For long-running restores, monitor progress:

```bash
# Check progress every few seconds
watch -n 2 'curl -s http://localhost:8080/api/v1/restore/progress | jq'

# Or poll in a script
while true; do
  curl -s http://localhost:8080/api/v1/restore/progress | jq '.progress_percent'
  sleep 5
done
```

### 5. Retention Policy

Implement a snapshot retention policy:

```bash
# Keep:
# - Last 24 hourly snapshots
# - Last 7 daily snapshots
# - Last 4 weekly snapshots
# - All deployment snapshots (indefinitely)

# Delete old snapshots (example)
curl -X DELETE http://localhost:8080/api/v1/snapshots/tags/hourly_20260130_0100
```

### 6. Test Recovery Procedures

**Regularly test disaster recovery**:

```bash
# Test restore in non-production environment
# 1. Create test snapshot
# 2. Make changes
# 3. Restore to snapshot
# 4. Verify data integrity

# Schedule quarterly DR drills
```

---

## Troubleshooting

### Restore Failed with "Invalid Sequence"

**Problem**: Target sequence doesn't exist or is in the future.

**Solution**:
```bash
# Check current sequence
curl http://localhost:8080/api/v1/changefeed?limit=1 | jq '.sequence'

# List available snapshots
curl http://localhost:8080/api/v1/snapshots/tags

# Use valid sequence/tag
```

### Restore Stuck at High Progress Percentage

**Problem**: Restore appears frozen near completion.

**Solution**:
```bash
# Check ThemisDB logs
tail -f /var/log/themisdb/themisdb.log

# Check database locks
curl http://localhost:8080/api/v1/admin/locks

# If truly stuck, may need to restart ThemisDB
systemctl restart themisdb
```

### Not Enough Disk Space

**Problem**: Restore fails with disk space error.

**Solution**:
```bash
# Check disk usage
df -h /var/lib/themisdb

# Free up space
# - Delete old snapshots
# - Clean up temporary files
# - Expand disk if needed

# Retry restore
```

### Tag Not Found

**Problem**: Snapshot tag doesn't exist.

**Solution**:
```bash
# List all tags
curl http://localhost:8080/api/v1/snapshots/tags | jq '.[] | .tag_name'

# Use correct tag name (case-sensitive!)
```

---

## Appendix

### A. Snapshot Naming Conventions

Recommended naming patterns:

- **Hourly**: `hourly_YYYYMMDD_HH00` (e.g., `hourly_20260206_1400`)
- **Daily**: `daily_YYYYMMDD` (e.g., `daily_20260206`)
- **Weekly**: `weekly_YYYYWW` (e.g., `weekly_202606`)
- **Deployment**: `pre_deploy_vX.Y.Z` (e.g., `pre_deploy_v1.2.3`)
- **Schema**: `pre_schema_DESCRIPTION` (e.g., `pre_schema_add_email_column`)
- **Incident**: `pre_incident_YYYYMMDD_HHMM` (e.g., `pre_incident_20260206_1430`)

### B. API Quick Reference

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/api/v1/snapshots/tags` | POST | Create snapshot |
| `/api/v1/snapshots/tags` | GET | List snapshots |
| `/api/v1/snapshots/tags/:name` | GET | Get snapshot |
| `/api/v1/snapshots/tags/:name` | DELETE | Delete snapshot |
| `/api/v1/restore/pitr` | POST | Execute restore |
| `/api/v1/restore/preview` | POST | Preview restore |
| `/api/v1/restore/progress` | GET | Get restore progress |

### C. Recovery Time Objectives (RTO)

Estimated restore times:

| Dataset Size | Estimated RTO |
|-------------|---------------|
| 1K events | < 1 second |
| 10K events | < 10 seconds |
| 100K events | < 2 minutes |
| 1M events | < 20 minutes |
| 10M events | < 3 hours |

*Times are approximate and depend on hardware and workload.*

### D. Emergency Contacts

Maintain an emergency contact list:

- **Database Administrator**: [your-dba@company.com]
- **DevOps Lead**: [devops-lead@company.com]
- **Security Team**: [security@company.com]
- **ThemisDB Support**: [support@themisdb.com]

### E. Additional Resources

- [Named Snapshots Documentation](features_snapshots.md)
- [PITR Feature Guide](features_pitr.md)
- [Backup Best Practices](../operations/backup_best_practices.md)
- [Security Guidelines](../security/security_guidelines.md)

---

**Document Version**: 1.0  
**Last Updated**: February 6, 2026  
**Maintained By**: ThemisDB Operations Team  
**Review Cycle**: Quarterly

---

## Feedback

For questions or improvements to this guide:
- 💬 [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- 🐛 [Report Issues](https://github.com/makr-code/ThemisDB/issues)
- 📧 Email: [support@themisdb.com](mailto:support@themisdb.com)
