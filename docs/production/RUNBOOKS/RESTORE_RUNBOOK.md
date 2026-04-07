# Restore Runbook

**Version:** 1.0  
**Last Updated:** April 2026  
**Target Audience:** Operations Teams, DBAs

---

## Overview

This runbook provides procedures for restoring ThemisDB from backups in various scenarios.

### Restore Scenarios

| Scenario | RTO | RPO | Procedure |
|----------|-----|-----|-----------|
| Single table corruption | 30 min | 15 min | Point-in-time restore |
| Complete data loss | 4 hours | 1 hour | Full restore |
| Ransomware recovery | 6 hours | 1 hour | Clean backup restore |
| Accidental deletion | 1 hour | 15 min | Selective restore |

---

## Pre-Restore Checklist

- [ ] Identify backup to restore from
- [ ] Verify backup integrity
- [ ] Assess impact and downtime
- [ ] Notify stakeholders
- [ ] Prepare rollback plan

---

## Full Database Restore

### Step 1: Identify Backup

```bash
# List available backups
themisdb-cli backup list --last 30-days

# Verify backup integrity
themisdb-cli backup verify --backup-id <backup-id> --deep-check

# Get backup metadata
themisdb-cli backup info --backup-id <backup-id>
```

### Step 2: Prepare for Restore

```bash
# Stop all write operations
themisdb-cli maintenance-mode enable --read-only

# Create snapshot of current state (for forensics)
themisdb-cli backup create --type full --label "before-restore-$(date +%Y%m%d-%H%M%S)"

# Drain all nodes
for NODE in $(themisdb-cli cluster list-nodes); do
  themisdb-cli node drain $NODE --wait
done
```

### Step 3: Execute Restore

```bash
# Restore from backup
themisdb-cli restore \
  --backup-id <backup-id> \
  --verify-consistency \
  --parallel-streams 4

# Expected output:
# ✓ Downloading backup from S3
# ✓ Verifying checksums
# ✓ Extracting data
# ✓ Restoring to /var/lib/themisdb/data
# ✓ Replaying WAL logs
# ✓ Rebuilding indexes
# ✓ Verifying data integrity
# ✓ Restore complete in 45m 32s
```

### Step 4: Verify and Validate

```bash
# Start database in verification mode
themisdb-cli start --verify-mode

# Run integrity checks
themisdb-cli data verify \
  --check-indexes \
  --check-constraints \
  --check-references

# Compare row counts
themisdb-cli metrics compare \
  --baseline backup-metadata.json \
  --current now \
  --tolerance 0.1%

# Run smoke tests
themisdb-cli test smoke --suite full
```

### Step 5: Resume Operations

```bash
# Disable maintenance mode
themisdb-cli maintenance-mode disable

# Undrain nodes
for NODE in $(themisdb-cli cluster list-nodes); do
  themisdb-cli node undrain $NODE
done

# Notify stakeholders
themisdb-cli incident update --status resolved --notify all
```

---

## Point-in-Time Restore (PITR)

### Overview

Restore database to a specific timestamp using continuous backups and WAL replay.

### Procedure

```bash
# Step 1: Identify target timestamp
TARGET_TIME="2026-01-24T14:30:00Z"

# Step 2: Find base backup before target time
BACKUP_ID=$(themisdb-cli backup find-before --timestamp $TARGET_TIME)

# Step 3: Perform PITR
themisdb-cli restore pitr \
  --target-time $TARGET_TIME \
  --backup-id $BACKUP_ID \
  --verify-consistency \
  --dry-run

# Step 4: Review dry-run results
themisdb-cli restore pitr-status

# Step 5: Execute if acceptable
themisdb-cli restore pitr \
  --target-time $TARGET_TIME \
  --backup-id $BACKUP_ID \
  --verify-consistency \
  --execute

# Step 6: Verify restoration
themisdb-cli data verify --timestamp $TARGET_TIME
```

---

## Selective Table Restore

### Procedure

```bash
# Step 1: Identify corrupted table
TABLE_NAME="users"

# Step 2: Export table from backup
themisdb-cli restore extract-table \
  --backup-id <backup-id> \
  --table $TABLE_NAME \
  --output /tmp/users-restore.sql

# Step 3: Create temporary table
themisdb-cli exec "CREATE TABLE users_temp AS SELECT * FROM users WHERE 1=0"

# Step 4: Import data to temp table
themisdb-cli restore import \
  --file /tmp/users-restore.sql \
  --table users_temp \
  --verify

# Step 5: Verify data
themisdb-cli data compare \
  --table-a users \
  --table-b users_temp \
  --sample-size 10000

# Step 6: Swap tables (within transaction)
themisdb-cli exec "
  BEGIN;
  ALTER TABLE users RENAME TO users_old;
  ALTER TABLE users_temp RENAME TO users;
  COMMIT;
"

# Step 7: Rebuild indexes
themisdb-cli index rebuild --table users
```

---

## Cross-Region Restore

### Procedure

```bash
# Restore from backup in different region
themisdb-cli restore \
  --backup-id <backup-id> \
  --source-region us-east-1 \
  --target-region us-west-2 \
  --transfer-mode direct \
  --encryption in-transit

# Monitor transfer progress
watch -n 5 'themisdb-cli restore status'
```

---

## Troubleshooting

### Restore Fails with Checksum Error

```bash
# Verify backup integrity
themisdb-cli backup verify --backup-id <backup-id> --repair

# Retry restore
themisdb-cli restore --backup-id <backup-id> --force-verify
```

### Out of Disk Space During Restore

```bash
# Check disk space
df -h /var/lib/themisdb

# Clean up old data
themisdb-cli storage cleanup --aggressive

# Increase disk size or use alternate location
themisdb-cli restore --backup-id <backup-id> --temp-dir /mnt/large-volume
```

### Slow Restore Performance

```bash
# Increase parallelism
themisdb-cli restore --backup-id <backup-id> --parallel-streams 8

# Use faster storage for temp files
themisdb-cli restore --backup-id <backup-id> --temp-dir /mnt/nvme

# Disable verification for faster restore (risky)
themisdb-cli restore --backup-id <backup-id> --skip-verify
```

---

## Validation Tests

```bash
# Standard validation suite
themisdb-cli validate suite --suite post-restore

# Custom validation queries
themisdb-cli exec "SELECT COUNT(*) FROM users" # Compare with expected
themisdb-cli exec "SELECT MAX(created_at) FROM events" # Verify timestamp

# Performance baseline
themisdb-cli benchmark --suite standard --compare-to baseline.json
```

---

## Success Criteria

- [ ] Backup restored successfully
- [ ] All integrity checks pass
- [ ] Row counts match expected values
- [ ] All tests passing
- [ ] Performance within acceptable range
- [ ] No data corruption detected

---

**Emergency Contact**: DBA On-Call +1-555-0123  
**Escalation Path**: DBA → Engineering Manager → CTO
