# Disaster Recovery Guide - ThemisDB

**Version:** 1.0  
**Date:** January 2026  
**Status:** ✅ Production Ready  
**Target Audience:** Database Administrators, DevOps Engineers, SREs
# Disaster Recovery Guide for ThemisDB

**Version:** 1.0  
**Date:** February 6, 2026  
**Category:** Operations Guide  
**Status:** ✅ Production Ready

---

## Table of Contents

- [Overview](#overview)
- [Quick Reference](#quick-reference)
- [Recovery Scenarios](#recovery-scenarios)
- [Step-by-Step Procedures](#step-by-step-procedures)
- [Quick Start](#quick-start)
- [Recovery Scenarios](#recovery-scenarios)
- [Best Practices](#best-practices)
- [Troubleshooting](#troubleshooting)
- [Appendix](#appendix)

---

## Overview

This guide provides comprehensive procedures for recovering ThemisDB from various disaster scenarios using Point-in-Time Recovery (PITR) features.

### When to Use This Guide

Use this guide when you need to:
- **Recover from data corruption** or accidental deletions
- **Rollback failed schema migrations** or deployments
- **Restore to a known-good state** after incidents
- **Test disaster recovery procedures** (DR drills)
- **Comply with audit requirements** for data restoration

### Prerequisites

- ThemisDB v1.5.0+ with PITR enabled
- Admin access to ThemisDB REST API or CLI
- Named snapshots (tags) created at critical points
- Changefeed enabled and configured
- Valid backup strategy in place

---

## Quick Reference

### Critical Commands

```bash
# Create snapshot before critical operation
curl -X POST http://localhost:8080/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d '{
    "tag_name": "before_migration_2026_01",
    "description": "Before Q1 2026 schema migration",
    "created_by": "admin"
  }'

# Preview restore (dry-run)
curl -X POST http://localhost:8080/api/v1/restore/preview \
  -H "Content-Type: application/json" \
  -d '{
    "restore_type": "tag",
    "target": "before_migration_2026_01"
  }'

# Execute restore
curl -X POST http://localhost:8080/api/v1/restore/pitr \
  -H "Content-Type: application/json" \
  -d '{
    "restore_type": "tag",
    "target": "before_migration_2026_01",
    "dry_run": false,
    "create_backup": true
  }'

# Check restore progress
curl http://localhost:8080/api/v1/restore/progress
```

### gRPC API Support

ThemisDB PITR also provides gRPC API for binary server communication. This is useful for:
- High-performance applications requiring binary protocol
- Microservices architectures using gRPC
- Cross-language client implementations

**gRPC Service:** `themis.core.PITRService`

**Example (using grpcurl):**
```bash
# Create snapshot via gRPC
grpcurl -plaintext -d '{
  "tag_name": "before_migration_2026_01",
  "description": "Before Q1 2026 schema migration",
  "created_by": "admin"
}' localhost:50051 themis.core.PITRService/CreateSnapshot

# Preview restore via gRPC
grpcurl -plaintext -d '{
  "restore_type": "TAG",
  "target": "before_migration_2026_01"
}' localhost:50051 themis.core.PITRService/PreviewRestore

# Execute restore via gRPC
grpcurl -plaintext -d '{
  "restore_type": "TAG",
  "target": "before_migration_2026_01",
  "dry_run": false,
  "create_backup": true
}' localhost:50051 themis.core.PITRService/ExecuteRestore
```

**Available gRPC Methods:**
- `CreateSnapshot` - Create named snapshot
- `ListSnapshots` - List all snapshots
- `GetSnapshot` - Get snapshot details
- `DeleteSnapshot` - Delete snapshot
- `PreviewRestore` - Preview restore operation
- `ExecuteRestore` - Execute restore
- `GetRestoreProgress` - Get restore progress

See [proto/themis_core.proto](../../../proto/themis_core.proto) for full message definitions.

### Emergency Contact

- **On-Call Team:** pagerduty@themisdb.com
- **Support Slack:** #themisdb-incidents
- **Documentation:** https://docs.themisdb.com/guides/disaster-recovery
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

### Scenario 1: Accidental Data Deletion

**Symptoms:**
- Critical data missing from tables
- User reports of lost records
- Audit logs show DELETE operations

**Impact:** High - Data loss  
**Recovery Time:** Minutes to hours (depends on data volume)  
**Complexity:** Low

**Solution:** [Point-in-Time Recovery to Last Known Good State](#pitr-to-last-known-good-state)

---

### Scenario 2: Failed Schema Migration

**Symptoms:**
- Application errors after migration
- Schema incompatibility issues
- Failed foreign key constraints

**Impact:** Critical - Service disruption  
**Recovery Time:** 5-30 minutes  
**Complexity:** Medium

**Solution:** [Rollback to Pre-Migration Snapshot](#rollback-to-pre-migration-snapshot)

---

### Scenario 3: Data Corruption

**Symptoms:**
- Database reports corruption errors
- Inconsistent query results
- RocksDB compaction failures

**Impact:** Critical - Data integrity compromised  
**Recovery Time:** Hours (requires investigation)  
**Complexity:** High

**Solution:** [Selective Table Recovery](#selective-table-recovery)

---

### Scenario 4: Malicious Activity

**Symptoms:**
- Unauthorized data modifications
- Mass deletions or updates
- Security breach detected

**Impact:** Critical - Security incident  
**Recovery Time:** 1-4 hours  
**Complexity:** High

**Solution:** [Forensic Recovery with Audit Trail](#forensic-recovery)

---

### Scenario 5: Failed Deployment

**Symptoms:**
- Application crashes after deployment
- Data model incompatibility
- Rollback required

**Impact:** High - Service degradation  
**Recovery Time:** 10-20 minutes  
**Complexity:** Low

**Solution:** [Rollback to Pre-Deployment Tag](#rollback-to-pre-deployment-tag)

---

## Step-by-Step Procedures

### PITR to Last Known Good State

**Use case:** Recover from recent data corruption or deletion

**Steps:**

1. **Identify the incident time**
   ```bash
   # Check changefeed for recent events
   curl http://localhost:8080/api/v1/changefeed/events?limit=100
   ```

2. **Find appropriate restore point**
   ```bash
   # List available tags
   curl http://localhost:8080/api/v1/snapshots/tags
   
   # Or use timestamp (1 hour ago)
   TIMESTAMP=$(($(date +%s) - 3600))000  # Unix timestamp in ms
   ```

3. **Preview the restore operation**
   ```bash
   curl -X POST http://localhost:8080/api/v1/restore/preview \
     -H "Content-Type: application/json" \
     -d "{
       \"restore_type\": \"timestamp\",
       \"target\": \"$TIMESTAMP\"
     }"
   ```

4. **Review preview output**
   - Check `events_to_replay` count
   - Verify `affected_tables` list
   - Review `estimated_duration_sec`
   - Confirm `estimated_size_bytes`

5. **Execute restore**
   ```bash
   curl -X POST http://localhost:8080/api/v1/restore/pitr \
     -H "Content-Type: application/json" \
     -d "{
       \"restore_type\": \"timestamp\",
       \"target\": \"$TIMESTAMP\",
       \"dry_run\": false,
       \"create_backup\": true,
       \"backup_tag\": \"emergency_backup_$(date +%Y%m%d_%H%M%S)\"
     }"
   ```

6. **Monitor progress**
   ```bash
   # Check progress every 10 seconds
   while true; do
     curl http://localhost:8080/api/v1/restore/progress
     sleep 10
   done
   ```

7. **Verify restoration**
   ```bash
   # Check critical tables
   curl http://localhost:8080/api/v1/query \
     -H "Content-Type: application/json" \
     -d '{"query": "SELECT COUNT(*) FROM critical_table"}'
   ```

8. **Document the incident**
   - Record restore timestamp
   - Document data loss window
   - Update incident log

**Expected Duration:** 10-60 minutes

---

### Rollback to Pre-Migration Snapshot

**Use case:** Undo failed schema or data migration

**Steps:**

1. **Stop application traffic** (if possible)
   ```bash
   # Put application in maintenance mode
   kubectl scale deployment/app --replicas=0
   ```

2. **Identify migration tag**
   ```bash
   # Tags should follow naming convention: before_migration_YYYYMMDD
   curl http://localhost:8080/api/v1/snapshots/tags/before_migration_20260115
   ```

3. **Preview rollback**
   ```bash
   curl -X POST http://localhost:8080/api/v1/restore/preview \
     -H "Content-Type: application/json" \
     -d '{
       "restore_type": "tag",
       "target": "before_migration_20260115"
     }'
   ```

4. **Execute rollback**
   ```bash
   curl -X POST http://localhost:8080/api/v1/restore/pitr \
     -H "Content-Type: application/json" \
     -d '{
       "restore_type": "tag",
       "target": "before_migration_20260115",
       "dry_run": false,
       "create_backup": true,
       "abort_on_first_error": true
     }'
   ```

5. **Verify schema state**
   ```bash
   # Check table schemas
   curl http://localhost:8080/api/v1/schema/tables
   ```

6. **Resume application traffic**
   ```bash
   kubectl scale deployment/app --replicas=3
   ```

7. **Run smoke tests**
   - Test critical user flows
   - Verify data integrity
   - Check application logs

**Expected Duration:** 5-30 minutes

---

### Selective Table Recovery

**Use case:** Recover specific tables without affecting entire database

**Steps:**

1. **Identify affected tables**
   ```bash
   # List tables
   curl http://localhost:8080/api/v1/schema/tables
   ```

2. **Preview selective restore**
   ```bash
   curl -X POST http://localhost:8080/api/v1/restore/preview \
     -H "Content-Type: application/json" \
     -d '{
       "restore_type": "tag",
       "target": "before_incident",
       "tables": ["users", "orders"]
     }'
   ```

3. **Execute selective restore**
   ```bash
   curl -X POST http://localhost:8080/api/v1/restore/pitr \
     -H "Content-Type: application/json" \
     -d '{
       "restore_type": "tag",
       "target": "before_incident",
       "dry_run": false,
       "create_backup": true,
       "tables": ["users", "orders"]
     }'
   ```

4. **Verify affected tables only**
   ```bash
   # Check restored tables
   curl http://localhost:8080/api/v1/query \
     -H "Content-Type: application/json" \
     -d '{"query": "SELECT COUNT(*) FROM users"}'
   
   # Verify unaffected tables unchanged
   curl http://localhost:8080/api/v1/query \
     -H "Content-Type: application/json" \
     -d '{"query": "SELECT COUNT(*) FROM products"}'
   ```

**Expected Duration:** 5-20 minutes

---

### Forensic Recovery

**Use case:** Recover from security incident with audit trail preservation

**Steps:**

1. **DO NOT RESTORE YET** - Preserve evidence
   ```bash
   # Create forensic snapshot first
   curl -X POST http://localhost:8080/api/v1/snapshots/tags \
     -H "Content-Type: application/json" \
     -d '{
       "tag_name": "forensic_snapshot_$(date +%Y%m%d_%H%M%S)",
       "description": "Snapshot for security incident investigation",
       "created_by": "security_team"
     }'
   ```

2. **Export changefeed events** for analysis
   ```bash
   # Get events during incident window
   curl "http://localhost:8080/api/v1/changefeed/events?from_sequence=1000&to_sequence=2000" \
     > incident_events.json
   ```

3. **Identify malicious activity** patterns
   - Review event types (PUT, DELETE)
   - Check user attribution
   - Analyze affected keys/tables

4. **Create incident report**
   ```bash
   # Generate incident summary
   python3 analyze_incident.py incident_events.json > incident_report.txt
   ```

5. **Coordinate with security team**
   - Share incident report
   - Get approval for restoration
   - Plan communication strategy

6. **Execute recovery** (after approval)
   ```bash
   curl -X POST http://localhost:8080/api/v1/restore/pitr \
     -H "Content-Type: application/json" \
     -d '{
       "restore_type": "timestamp",
       "target": "TIMESTAMP_BEFORE_INCIDENT",
       "dry_run": false,
       "create_backup": true,
       "backup_tag": "before_security_restore"
     }'
   ```

7. **Implement security fixes**
   - Patch vulnerabilities
   - Update access controls
   - Enable additional monitoring

**Expected Duration:** 1-4 hours (investigation) + 10-60 minutes (recovery)

---

## Best Practices

### Before Disasters

#### 1. Proactive Snapshot Creation

Create snapshots at critical points:

```bash
# Before deployments
curl -X POST http://localhost:8080/api/v1/snapshots/tags \
  -d '{"tag_name": "before_deploy_v1.2.0", "description": "..."}'

# Before schema migrations
curl -X POST http://localhost:8080/api/v1/snapshots/tags \
  -d '{"tag_name": "before_migration_YYYYMMDD", "description": "..."}'

# Before major data operations
curl -X POST http://localhost:8080/api/v1/snapshots/tags \
  -d '{"tag_name": "before_bulk_import", "description": "..."}'
```

#### 2. Snapshot Naming Convention

Follow consistent naming:
- `before_deploy_<version>` - Pre-deployment snapshots
- `before_migration_<YYYYMMDD>` - Pre-migration snapshots
- `daily_backup_<YYYYMMDD>` - Regular backups
- `emergency_<YYYYMMDD_HHMMSS>` - Emergency snapshots

#### 3. Regular DR Drills

Test recovery procedures quarterly:

```bash
# DR Drill Script
#!/bin/bash
set -e

echo "Starting DR Drill..."

# 1. Create test snapshot
curl -X POST http://localhost:8080/api/v1/snapshots/tags \
  -d '{"tag_name": "dr_drill_start", "description": "DR Drill Start"}'

# 2. Make test changes
# (simulate incident)

# 3. Test restore
curl -X POST http://localhost:8080/api/v1/restore/pitr \
  -d '{"restore_type": "tag", "target": "dr_drill_start", "dry_run": true}'

echo "DR Drill Completed"
```

#### 4. Changefeed Retention

Configure appropriate retention:

```yaml
# config.yaml
changefeed:
  retention_days: 90  # Keep 90 days of events
  max_events: 10000000  # Limit total events
```

#### 5. Monitor Restore Capabilities

Set up alerts:

```yaml
# Prometheus alerts
- alert: PITRRestorable
  expr: themis_changefeed_events_count > 0
  for: 5m
  annotations:
    summary: "PITR restore capability verified"
```

### During Disasters

#### 1. Stay Calm and Follow Procedures

- **Don't panic** - Follow this guide step-by-step
- **Communicate** - Notify stakeholders immediately
- **Document** - Record all actions taken
- **Verify** - Always preview before executing restore

#### 2. Use Dry-Run First

Always test with `dry_run: true`:

```bash
curl -X POST http://localhost:8080/api/v1/restore/pitr \
  -d '{
    "restore_type": "tag",
    "target": "before_incident",
    "dry_run": true  # Always test first!
  }'
```

#### 3. Create Emergency Backup

Never skip the automatic backup:

```bash
curl -X POST http://localhost:8080/api/v1/restore/pitr \
  -d '{
    "restore_type": "tag",
    "target": "before_incident",
    "create_backup": true,  # Always create backup!
    "backup_tag": "emergency_backup_20260115_143000"
  }'
```

#### 4. Monitor Progress

Use progress endpoint to track restore:

```bash
# Monitor script
while true; do
  PROGRESS=$(curl -s http://localhost:8080/api/v1/restore/progress)
  echo "$(date): $PROGRESS"
  
  if echo "$PROGRESS" | grep -q '"in_progress":false'; then
    echo "Restore completed!"
    break
  fi
  
  sleep 10
done
```

### After Recovery

#### 1. Verify Data Integrity

Run validation queries:

```bash
# Count records in critical tables
curl http://localhost:8080/api/v1/query \
  -d '{"query": "SELECT table_name, COUNT(*) FROM information_schema.tables"}'

# Check key business metrics
curl http://localhost:8080/api/v1/query \
  -d '{"query": "SELECT COUNT(*) FROM users WHERE active=true"}'
```

#### 2. Document Incident

Create incident report:
- What happened?
- When did it happen?
- What was the root cause?
- What was restored?
- What was the data loss window?
- How long did recovery take?
- What can we do to prevent this?

#### 3. Update Runbooks

Improve procedures based on lessons learned.

#### 4. Clean Up Old Snapshots

Remove emergency snapshots after verification:

```bash
# List snapshots
curl http://localhost:8080/api/v1/snapshots/tags

# Delete old emergency snapshots
curl -X DELETE http://localhost:8080/api/v1/snapshots/tags/emergency_backup_20260110
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

### Problem: Restore Takes Too Long

**Symptoms:** Restore progress is very slow

**Solutions:**
1. Check `estimated_duration_sec` in preview
2. Use selective table restore if possible
3. Limit events with `max_events_to_replay`
4. Restore to closer sequence number

```bash
# Selective restore
curl -X POST http://localhost:8080/api/v1/restore/pitr \
  -d '{
    "restore_type": "sequence",
    "target": "12345",
    "tables": ["critical_table_only"]
  }'
```

---

### Problem: Restore Fails with Error

**Symptoms:** Restore returns error status

**Solutions:**
1. Check error message in response
2. Verify target sequence/tag exists
3. Ensure sufficient disk space
4. Check RocksDB logs

```bash
# Check logs
tail -f /var/log/themisdb/themisdb.log

# Verify target exists
curl http://localhost:8080/api/v1/snapshots/tags/target_tag
```

---

### Problem: Cannot Find Restore Point

**Symptoms:** No suitable snapshot or timestamp available

**Solutions:**
1. List all available snapshots
2. Use changefeed to find appropriate sequence
3. Consider restoring to closest available point

```bash
# List snapshots
curl http://localhost:8080/api/v1/snapshots/tags

# Find sequence by timestamp
curl "http://localhost:8080/api/v1/changefeed/events?from_timestamp=TIMESTAMP"
```

---

### Problem: Restore Doesn't Fix Issue

**Symptoms:** Problem persists after restore

**Solutions:**
1. Verify you restored to correct point
2. Check if problem existed before restore point
3. Consider restoring to earlier point
4. Check for external causes (hardware, network)
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

### A. Restore Types Comparison

| Type | Use Case | Example | Pros | Cons |
|------|----------|---------|------|------|
| **Sequence** | Precise restore | `12345` | Exact point | Need to know sequence |
| **Tag** | Named checkpoints | `before_deploy_v1.0` | Human-readable | Must create tags proactively |
| **Timestamp** | Time-based restore | `1705045200000` | Intuitive | Less precise |

### B. Restore Options Reference

```json
{
  "restore_type": "sequence|tag|timestamp",
  "target": "...",
  "dry_run": false,
  "create_backup": true,
  "abort_on_first_error": true,
  "tables": [],
  "max_events_to_replay": 0,
  "backup_tag": "before_pitr_restore"
}
```

### C. Progress Phases

1. `not_started` - Restore not yet initiated
2. `validating` - Validating restore parameters
3. `creating_backup` - Creating automatic backup
4. `replaying_events` - Replaying events backward
5. `committing` - Committing changes
6. `completed` - Restore completed successfully
7. `failed` - Restore failed with error
8. `rolled_back` - Restore rolled back due to error

### D. Example Scripts

#### Full DR Test Script

```bash
#!/bin/bash
# DR Test Script
set -e

TAG_NAME="dr_test_$(date +%Y%m%d_%H%M%S)"

echo "1. Creating snapshot..."
curl -X POST http://localhost:8080/api/v1/snapshots/tags \
  -H "Content-Type: application/json" \
  -d "{\"tag_name\": \"$TAG_NAME\", \"description\": \"DR Test\"}"

echo "2. Making test changes..."
# Insert test data
curl -X POST http://localhost:8080/api/v1/entities \
  -d '{"table": "test", "data": {"id": 999, "test": true}}'

echo "3. Preview restore..."
curl -X POST http://localhost:8080/api/v1/restore/preview \
  -H "Content-Type: application/json" \
  -d "{\"restore_type\": \"tag\", \"target\": \"$TAG_NAME\"}"

echo "4. Execute restore (dry-run)..."
curl -X POST http://localhost:8080/api/v1/restore/pitr \
  -H "Content-Type: application/json" \
  -d "{\"restore_type\": \"tag\", \"target\": \"$TAG_NAME\", \"dry_run\": true}"

echo "DR Test Completed Successfully!"
```

### E. Escalation Matrix

| Severity | Response Time | Escalation |
|----------|---------------|------------|
| **P0 - Critical** | Immediate | CTO, VP Engineering |
| **P1 - High** | < 15 minutes | Engineering Manager |
| **P2 - Medium** | < 1 hour | On-Call Team Lead |
| **P3 - Low** | < 4 hours | Support Team |

### F. Support Contacts

- **Emergency Hotline:** +1-XXX-XXX-XXXX
- **Support Email:** support@themisdb.com
- **Slack:** #themisdb-support
- **Documentation:** https://docs.themisdb.com
- **Status Page:** https://status.themisdb.com

---

## Conclusion

This guide provides comprehensive procedures for disaster recovery using ThemisDB's PITR capabilities. Regular practice, proactive snapshot creation, and following best practices will ensure successful recovery when needed.

**Remember:**
- Always preview before executing
- Always create backups
- Always verify after restoration
- Always document incidents
- Always learn from incidents

For additional support, contact the ThemisDB support team or refer to the online documentation at https://docs.themisdb.com.

---

**Document Control:**
- **Version:** 1.0
- **Last Updated:** April 2026
- **Next Review:** April 2026
- **Owner:** ThemisDB Operations Team
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

- [Named Snapshots Documentation](../features/features_snapshots.md)
- [PITR Feature Guide](../features/features_pitr.md)
- [Backup Best Practices](../operations/backup_best_practices.md)
- [Security Guidelines](../security/security_guidelines.md)

---

**Document Version**: 1.0  
**Last Updated**: April 2026  
**Maintained By**: ThemisDB Operations Team  
**Review Cycle**: Quarterly

---

## Feedback

For questions or improvements to this guide:
- 💬 [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
- 🐛 [Report Issues](https://github.com/makr-code/ThemisDB/issues)
- 📧 Email: [support@themisdb.com](mailto:support@themisdb.com)
