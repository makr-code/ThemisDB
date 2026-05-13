# Backup & Recovery System Documentation

## Overview

ThemisDB's Backup & Recovery system provides comprehensive data protection capabilities with support for multiple backup strategies, integrity verification, and point-in-time recovery.

## Features

### Backup Types

#### 1. Full Backup
- Complete database snapshot using RocksDB checkpoint API
- Includes all data files and Write-Ahead Log (WAL) files
- Foundation for incremental and differential backups
- **Duration:** Medium (depends on database size)
- **Storage:** Full database size

```cpp
auto result = backup_mgr->createFullBackup("/backups");
if (result) {
    std::cout << "Backup created at: " << *result << std::endl;
} else {
    std::cerr << "Backup failed: " << result.error().message() << std::endl;
}
```

#### 2. Incremental Backup
- Captures only changes since the last backup (full or incremental)
- Small storage footprint
- Fast execution
- **Duration:** Fast
- **Storage:** Small (only changed data)

```cpp
auto result = backup_mgr->createIncrementalBackup("/backups");
```

#### 3. Differential Backup
- Captures changes since the last **full** backup
- Medium storage footprint
- Faster restore than incremental chain
- **Duration:** Fast
- **Storage:** Medium (accumulated changes since full backup)

```cpp
auto result = backup_mgr->createDifferentialBackup("/backups");
```

### Backup Verification

All backups include integrity verification using:
- SHA-256 checksums for data validation
- Manifest file validation
- Structure verification (checkpoint, WAL, metadata)
- RAID5/6 shard completeness checks

```cpp
auto result = backup_mgr->verifyBackup(backup_path);
if (result) {
    std::cout << "Backup integrity verified" << std::endl;
}
```

### Compression

Backups can be compressed to save storage space:

```cpp
// Compress a backup
auto compressed = backup_mgr->compressBackup(backup_path);
if (compressed) {
    std::cout << "Compressed to: " << *compressed << std::endl;
}

// Decompress for restore
auto decompressed = backup_mgr->decompressBackup(compressed_file, dest_dir);
```

### WAL Archiving

Continuous archiving of Write-Ahead Log files for point-in-time recovery:

```cpp
auto result = backup_mgr->archiveWAL("/wal_archive");
```

## Backup Strategies

### Strategy 1: Full + Incremental
- Weekly full backup
- Daily incremental backups
- **Pros:** Minimal backup time, small storage
- **Cons:** Longer restore time (need to apply all incrementals)

### Strategy 2: Full + Differential
- Weekly full backup
- Daily differential backups
- **Pros:** Faster restore (only need full + latest differential)
- **Cons:** Larger backup sizes over time

### Strategy 3: Full Only
- Daily full backups
- **Pros:** Simplest restore process
- **Cons:** Highest storage and time requirements

## RAID5/6 Support

For RAID5/6 configurations, the backup system ensures all shards (data + parity) are included:

```cpp
// Automatic RAID detection
auto raid_config = BackupManager::detectRAIDConfiguration();

// Verify RAID backup completeness
auto result = backup_mgr->isBackupComplete(backup_path, raid_config);
```

**Important:** For RAID5/6, ALL shards must be backed up together to ensure complete data recovery.

## Recovery Procedures

### Full Database Recovery

```cpp
auto result = backup_mgr->restoreFromBackup(backup_path);
if (result) {
    std::cout << "Database restored successfully" << std::endl;
}
```

### Point-in-Time Recovery (PITR)

PITR is handled by the `PITRManager` class (see `include/storage/pitr_manager.h`):

```cpp
PITRManager pitr(db, changefeed, snapshot_mgr);

// Restore to specific sequence number
auto result = pitr.restoreToSequence(target_seq);

// Restore to timestamp
auto result = pitr.restoreToTimestamp(timestamp_ms);

// Restore to named snapshot
auto result = pitr.restoreToTag("before_migration");
```

## Backup Directory Structure

```
backup_dir/
├── full_20260122_120000/
│   ├── checkpoint/          # RocksDB checkpoint data
│   ├── wal/                 # WAL files at checkpoint time
│   ├── raid_topology/       # RAID5/6: shard topology info
│   │   ├── shard_0/
│   │   ├── shard_1/
│   │   └── shard_parity/
│   └── MANIFEST.json        # Backup metadata
├── incr_20260123_120000/
│   ├── wal/                 # Incremental WAL files
│   └── MANIFEST.json
├── diff_20260124_120000/
│   ├── wal/                 # Differential WAL files
│   └── MANIFEST.json
└── latest -> full_20260122_120000/  # Symlink to latest backup
```

## Error Handling

The backup system uses `Result<T>` pattern for type-safe error handling:

```cpp
auto result = backup_mgr->createFullBackup(dest_dir);
if (!result) {
    auto error = result.error();
    
    // Check error code
    if (error.code() == ErrorCode::ERR_STORAGE_DISK_FULL) {
        // Handle disk full
    }
    
    // Get error message with context
    std::cerr << "Error: " << error.message() << std::endl;
    
    // Get metadata for user guidance
    auto metadata = error.metadata();
    std::cerr << "Solution: " << metadata.solution << std::endl;
}
```

### Backup Error Codes

| Error Code | Description | Severity |
|------------|-------------|----------|
| `ERR_BACKUP_CREATION_FAILED` | Backup creation failed | Error |
| `ERR_BACKUP_RESTORATION_FAILED` | Backup restoration failed | Error |
| `ERR_BACKUP_VERIFICATION_FAILED` | Backup integrity check failed | Warning |
| `ERR_BACKUP_NOT_FOUND` | Backup does not exist | Error |
| `ERR_BACKUP_INVALID_TYPE` | Unsupported backup type | Error |
| `ERR_BACKUP_INCOMPLETE` | Missing backup components | Critical |
| `ERR_BACKUP_COMPRESSION_FAILED` | Compression failed | Error |
| `ERR_BACKUP_DECOMPRESSION_FAILED` | Decompression failed | Error |
| `ERR_BACKUP_CHECKSUM_MISMATCH` | Checksum verification failed | Critical |
| `ERR_BACKUP_MANIFEST_CORRUPT` | Manifest file corrupted | Error |
| `ERR_BACKUP_WAL_ARCHIVE_FAILED` | WAL archiving failed | Error |

## Best Practices

### 1. Regular Testing
- Test restore procedures regularly
- Verify backup integrity after creation
- Practice recovery scenarios

### 2. Backup Retention
- Keep multiple generations of backups
- Implement retention policies based on RPO/RTO requirements
- Archive old backups offsite

### 3. Monitoring
- Monitor backup success/failure
- Track backup sizes and durations
- Alert on backup verification failures

### 4. Security
- Encrypt backups for sensitive data
- Secure backup storage locations
- Implement access controls

### 5. RAID Considerations
- For RAID5/6, coordinate backup across all shards
- Verify all shards are present in backups
- Test cross-shard restoration

## Performance Considerations

### Backup Performance
- Full backup: ~10-50 MB/s (depends on I/O)
- Incremental/Differential: Very fast (only WAL files)
- Compression: Adds CPU overhead but saves storage

### Restore Performance
- Full restore: Similar to backup speed
- Incremental chain: Slower (need to apply all changes)
- Differential: Faster than incremental chain

### Optimization Tips
1. Schedule backups during low-traffic periods
2. Use differential backups for faster restore
3. Consider parallel backup/restore (future feature)
4. Compress backups if storage is limited
5. Use local SSD storage for backup destination

## Example: Complete Backup Workflow

```cpp
#include "storage/backup_manager.h"
#include "storage/rocksdb_wrapper.h"

// Initialize
auto db = std::make_shared<RocksDBWrapper>(config);
db->open();

auto backup_mgr = std::make_unique<BackupManager>(db);

// Weekly full backup
auto full_result = backup_mgr->createFullBackup("/backups");
if (!full_result) {
    std::cerr << "Full backup failed: " << full_result.error().message() << std::endl;
    return;
}

// Verify backup
auto verify_result = backup_mgr->verifyBackup(*full_result);
if (!verify_result) {
    std::cerr << "Backup verification failed: " << verify_result.error().message() << std::endl;
    return;
}

// Compress for storage
auto compress_result = backup_mgr->compressBackup(*full_result);
if (compress_result) {
    std::cout << "Backup compressed: " << *compress_result << std::endl;
}

// Daily incremental backup
auto incr_result = backup_mgr->createIncrementalBackup("/backups");

// List all backups
auto backups = backup_mgr->listBackups("/backups");
for (const auto& backup : backups) {
    std::cout << "Backup: " << backup << std::endl;
}

// Restore if needed
if (disaster_occurred) {
    auto latest_backup = backups.back();
    auto restore_result = backup_mgr->restoreFromBackup(
        "/backups/" + latest_backup
    );
    
    if (restore_result) {
        std::cout << "Database restored successfully" << std::endl;
    }
}
```

## Future Enhancements

### Planned Features
- [ ] Parallel backup/restore for faster performance
- [ ] Backup deduplication to reduce storage
- [ ] Cloud backup support (S3, Azure Blob, GCS)
- [ ] Backup encryption
- [ ] Automatic retention policy enforcement
- [ ] Recovery time estimation
- [ ] Backup catalog/metadata tracking
- [ ] Cross-region replication
- [ ] Snapshot-based backups

### Configuration (Future)

```yaml
backup:
  schedule:
    full: "0 2 * * 0"      # Weekly Sunday 2 AM
    incremental: "0 2 * * 1-6"  # Daily except Sunday
  
  retention:
    full: 4                 # Keep 4 full backups
    incremental: 30         # Keep 30 days of incrementals
  
  compression: true
  verify_after_backup: true
  
  destinations:
    - type: local
      path: /backup/local
    - type: s3
      bucket: themis-backups
      region: us-east-1
```

## Troubleshooting

### Backup Fails with Disk Full
```bash
# Check disk space
df -h /backups

# Clean old backups
rm -rf /backups/old_*

# Implement retention policy
```

### Restore Fails with Checksum Mismatch
```bash
# Don't use this backup - it's corrupted
# Try previous backup
ls -lt /backups/

# Check for disk errors
dmesg | grep -i error
```

### RAID Backup Incomplete
```bash
# Verify all shards are accessible
# Check RAID configuration
echo $THEMIS_RAID_GROUP
echo $THEMIS_SHARDS

# Ensure all shard nodes are running
# Create new coordinated backup
```

## References

- [RocksDB Checkpoint Documentation](https://github.com/facebook/rocksdb/wiki/Checkpoints)
- [Write-Ahead Logging (WAL)](https://github.com/facebook/rocksdb/wiki/Write-Ahead-Log)
- PITR Manager: `include/storage/pitr_manager.h`
- Error Registry: `include/utils/error_registry.h`
- Test Examples: `tests/test_backup_manager_enhanced.cpp`

---

## Operational Procedures

This section describes the admin-facing backup and restore procedures. For the programmatic C++ API, see sections above.

### Install / Initial Setup

Before taking the first backup, ensure:

1. **Storage destination is provisioned** with sufficient capacity (at minimum 2× DB size for full backups + incremental retention)
2. **Backup user credentials** are configured if using remote storage destinations
3. **Schedules are enabled** via the maintenance API (see [Maintenance Schedule](maintenance/README.md))

```bash
# Verify DB size
du -sh /var/lib/themis/data

# Verify backup destination is writable
touch /backups/.write_test && rm /backups/.write_test

# Enable daily incremental backup schedule
curl -X PATCH -H "Authorization: Bearer $TOKEN" \
     -H "Content-Type: application/json" \
     -d '{"enabled": true}' \
     https://themisdb/api/v1/maintenance/schedules/<daily-schedule-id>
```

### Upgrade Procedure (Pre/Post Backup)

Always create a full backup before and after a major upgrade:

```bash
# 1. Pre-upgrade full backup
themisdb-cli backup create --type full --dest /backups/pre-upgrade-$(date +%Y%m%d)

# 2. Verify backup integrity
themisdb-cli backup verify /backups/pre-upgrade-$(date +%Y%m%d)

# 3. Perform upgrade (see Upgrade Runbook)

# 4. Post-upgrade verification backup
themisdb-cli backup create --type full --dest /backups/post-upgrade-$(date +%Y%m%d)
```

→ Full upgrade procedure: [production/RUNBOOKS/UPGRADE_RUNBOOK.md](production/RUNBOOKS/UPGRADE_RUNBOOK.md)

### Backup Execution (Manual)

```bash
# Full backup
themisdb-cli backup create --type full --dest /backups

# Incremental backup (since last backup)
themisdb-cli backup create --type incremental --dest /backups

# Differential backup (since last full backup)
themisdb-cli backup create --type differential --dest /backups

# Verify a backup
themisdb-cli backup verify /backups/full_<timestamp>

# List available backups
themisdb-cli backup list /backups
```

### Restore Procedure

```bash
# 1. Stop the ThemisDB service
sudo systemctl stop themisdb

# 2. Identify the target backup
themisdb-cli backup list /backups

# 3. Restore from full backup
themisdb-cli backup restore /backups/full_<timestamp>

# 4. Apply incrementals if needed (for Full+Incremental strategy)
themisdb-cli backup restore --apply-incrementals /backups/incr_<timestamp>

# 5. Verify restored data
themisdb-cli health

# 6. Restart service
sudo systemctl start themisdb
```

→ Full restore guide: [production/RUNBOOKS/RESTORE_RUNBOOK.md](production/RUNBOOKS/RESTORE_RUNBOOK.md)

### Point-in-Time Recovery (PITR)

```bash
# Restore to a specific timestamp
themisdb-cli pitr restore --timestamp "2026-05-12T14:00:00Z"

# Restore to a specific sequence number
themisdb-cli pitr restore --sequence 1234567

# Restore to a named snapshot tag
themisdb-cli pitr restore --tag before_migration
```

### Disaster Recovery

In a disaster scenario, follow the full DR plan:

1. Consult the [Disaster Recovery Plan](production/DISASTER_RECOVERY_PLAN.md) for RTO/RPO targets
2. Execute [DR Checklists](operations/disaster-recovery/DR_CHECKLISTS.md)
3. Use the [Restore Runbook](production/RUNBOOKS/RESTORE_RUNBOOK.md) for step-by-step recovery
4. Run [DR Testing](operations/disaster-recovery/DR_TESTING.md) quarterly to validate readiness

### Monitoring Backup Health

Configure alerts for backup failures via Prometheus:

```yaml
# prometheus/alerts/backup-rules.yml
- alert: BackupFailed
  expr: themis_backup_last_success_seconds > 86400  # No successful backup in 24h
  for: 1h
  labels:
    severity: critical
  annotations:
    summary: "ThemisDB backup has not completed in 24 hours"

- alert: BackupVerificationFailed
  expr: themis_backup_verification_errors_total > 0
  for: 5m
  labels:
    severity: warning
  annotations:
    summary: "ThemisDB backup integrity verification failed"
```

→ Full monitoring setup: [production/MONITORING.md](production/MONITORING.md)

---

**Related Documentation**

| Document | Purpose |
|----------|---------|
| [production/RUNBOOKS/RESTORE_RUNBOOK.md](production/RUNBOOKS/RESTORE_RUNBOOK.md) | Full step-by-step restore procedure |
| [production/RUNBOOKS/UPGRADE_RUNBOOK.md](production/RUNBOOKS/UPGRADE_RUNBOOK.md) | Pre/post-upgrade backup procedures |
| [production/DISASTER_RECOVERY_PLAN.md](production/DISASTER_RECOVERY_PLAN.md) | DR plan with RTO/RPO targets |
| [operations/disaster-recovery/DR_CHECKLISTS.md](operations/disaster-recovery/DR_CHECKLISTS.md) | DR execution checklists |
| [maintenance/README.md](maintenance/README.md) | Automated maintenance and backup schedules |
| [production/MONITORING.md](production/MONITORING.md) | Backup health monitoring and alerting |
| [CDC_OPERATIONS_RUNBOOK.md](CDC_OPERATIONS_RUNBOOK.md) | CDC-specific backup and recovery procedures |
