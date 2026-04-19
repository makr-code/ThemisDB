# Storage Troubleshooting Guide

The `storage` module provides the primary persistence layer for ThemisDB, wrapping RocksDB with MVCC, WAL, PITR, compaction management, and pluggable blob backends (S3, Azure Blob, WebDAV, filesystem).

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `RocksDBWrapper::open: IO error opening db` | Wrong data path or permissions | Check `storage.rocksdb.data_path` and directory permissions |
| Disk usage grows unbounded | Compaction disabled or too slow | Increase `storage.rocksdb.max_background_compactions` |
| Blob upload fails with `403 Forbidden` | Expired or wrong cloud credentials | Rotate credentials; check `storage.blob.s3.access_key_id` |
| WAL replay takes > 5 min on startup | Large WAL, low `wal_ttl_seconds` not set | Set `storage.rocksdb.wal_ttl_seconds: 3600` |
| `PITR restore failed: snapshot not found` | Snapshot older than retention window | Extend `storage.pitr.retention_days` before backup |
| High write latency (> 50 ms P99) | Write stall due to L0 file accumulation | Tune `storage.rocksdb.level0_slowdown_writes_trigger` |
| `BackupManager: backup destination unreachable` | Network or credentials issue for remote backup | Verify remote backup endpoint and credentials |
| Compaction uses 100% CPU | Too many background threads for host | Lower `storage.rocksdb.max_background_jobs` |
| `blob_redundancy_manager: quorum not met` | Fewer blob replicas than quorum threshold | Ensure `storage.blob.redundancy.min_replicas` nodes are reachable |
| Data corruption on read (`checksum mismatch`) | Disk hardware error or RocksDB block corruption | Run `themisdb-repair --data-dir /var/lib/themisdb/data` |

## Common Issues

### Issue 1: Database Fails to Open After Crash

**Description:** ThemisDB cannot reopen the RocksDB instance after an unclean shutdown.

**Symptoms:**
- Log line: `RocksDBWrapper::open: Corruption: MANIFEST file not found`
- Server exits immediately on startup

**Cause:** WAL or MANIFEST file was partially written before crash.

**Solution:**
```bash
# Attempt automated repair
themisdb-admin storage repair --data-dir /var/lib/themisdb/data

# If repair fails, restore from the latest PITR snapshot
themisdb-admin pitr restore \
  --snapshot-id $(themisdb-admin pitr list --latest) \
  --target-dir /var/lib/themisdb/data
```

---

### Issue 2: Write Stalls Under Load

**Description:** Writes slow dramatically when L0 SST file count exceeds threshold.

**Symptoms:**
- Log: `[RocksDB] Stalling writes because we have N level-0 files`
- P99 write latency spikes to >200 ms

**Cause:** Compaction cannot keep up with write rate; `level0_slowdown_writes_trigger` is too low.

**Solution:**
```yaml
# config/themisdb.yaml
storage:
  rocksdb:
    level0_slowdown_writes_trigger: 20   # default: 8 – increase headroom
    level0_stop_writes_trigger: 36
    max_background_compactions: 4
    max_background_flushes: 2
```

---

### Issue 3: S3 Blob Backend Returns 403

**Description:** Blob writes to S3 fail with permission errors.

**Symptoms:**
- Log: `BlobBackendS3::put: S3 error 403 AccessDenied`
- Ingestion pipeline stalls

**Cause:** IAM credentials expired or the bucket policy lacks `s3:PutObject`.

**Solution:**
```yaml
storage:
  blob:
    backend: s3
    s3:
      bucket: themisdb-blobs
      region: eu-central-1
      access_key_id: ${THEMIS_S3_KEY}      # use env-var injection
      secret_access_key: ${THEMIS_S3_SECRET}
      endpoint: ""                          # leave empty for AWS; set for MinIO
```
```bash
# Verify credentials externally
aws s3 ls s3://themisdb-blobs/ --region eu-central-1

# Rotate credentials via admin API
curl -X POST http://localhost:9090/admin/storage/blob/rotate-credentials \
     -H "Authorization: Bearer $ADMIN_TOKEN"
```

---

### Issue 4: PITR Restore Skips Recent Data

**Description:** Point-in-time recovery restores to an earlier-than-expected state.

**Symptoms:**
- Restored data is missing the last N minutes of writes
- `themisdb-admin pitr list` shows no snapshot within the requested window

**Cause:** Snapshot interval is too large or retention window too short.

**Solution:**
```yaml
storage:
  pitr:
    enabled: true
    snapshot_interval_minutes: 5    # lower from default 60
    retention_days: 14
    storage_backend: s3             # or "filesystem"
```

---

### Issue 5: Compaction Never Completes

**Description:** The compaction queue grows indefinitely; disk fills up.

**Symptoms:**
- `themisdb-admin storage stats` shows `pending_compaction_bytes > 50GB`
- Log: `CompactionManager: compaction job queued but not started`

**Cause:** `max_background_jobs` set to 1 or compaction throttle too aggressive.

**Solution:**
```bash
# Check current compaction stats
themisdb-admin storage compaction-stats

# Trigger manual compaction on a specific column family
themisdb-admin storage compact --cf default --level 0
```
```yaml
storage:
  rocksdb:
    max_background_jobs: 8
    compaction_style: level            # "level" is most predictable
    target_file_size_base: 67108864   # 64 MB
```

---

### Issue 6: WebDAV Blob Backend Upload Timeouts

**Description:** Large blob writes to WebDAV backend time out intermittently.

**Symptoms:**
- Log: `BlobBackendWebDAV::put: connection timed out after 30s`
- Partial uploads leave `.tmp` files on the server

**Cause:** Default HTTP timeout is too short for large payloads over slow links.

**Solution:**
```yaml
storage:
  blob:
    backend: webdav
    webdav:
      base_url: https://dav.example.com/themisdb/
      username: ${WEBDAV_USER}
      password: ${WEBDAV_PASS}
      connect_timeout_ms: 10000
      transfer_timeout_ms: 120000    # increase from 30000
      retry_max: 3
```

---

### Issue 7: High Memory Usage from Block Cache

**Description:** ThemisDB process RSS grows to several GB with no query load.

**Symptoms:**
- `top` shows high RSS
- Log: `BlockBasedTable: block cache hit rate 12%`

**Cause:** Block cache sized too large relative to available RAM, or cache is cold.

**Solution:**
```yaml
storage:
  rocksdb:
    block_cache_size_mb: 512         # tune to ~25% of available RAM
    compressed_block_cache_size_mb: 128
    cache_index_and_filter_blocks: true
```

---

### Issue 8: Azure Blob Backend Authentication Failure

**Description:** Blob writes fail with `401 Unauthorized` against Azure Blob Storage.

**Symptoms:**
- Log: `BlobBackendAzure::put: HTTP 401 AuthenticationFailed`

**Cause:** SAS token expired or connection string is malformed.

**Solution:**
```yaml
storage:
  blob:
    backend: azure
    azure:
      account_name: themisdbstorage
      container_name: blobs
      # Use either connection_string OR sas_token, not both
      connection_string: ${AZURE_STORAGE_CONNECTION_STRING}
      sas_token: ""
```
```bash
# Validate the connection string manually
az storage container list \
  --connection-string "$AZURE_STORAGE_CONNECTION_STRING"
```

---

### Issue 9: Disk Space Monitor False Alerts

**Description:** `DiskSpaceMonitor` triggers alerts even when disk is not actually full.

**Symptoms:**
- Alert: `disk_usage_pct{mount="/var/lib/themisdb"} > 90` fires continuously
- `df -h` shows only 70% usage

**Cause:** Monitor sampling interval is too short; inode exhaustion rather than block exhaustion.

**Solution:**
```bash
# Check inode usage
df -i /var/lib/themisdb

# Tune monitor thresholds
```
```yaml
storage:
  disk_monitor:
    check_interval_seconds: 60
    warn_threshold_pct: 80
    critical_threshold_pct: 90
    check_inodes: true               # enable inode checks
```

---

### Issue 10: Backup Fails Mid-Transfer

**Description:** `BackupManager` fails with a partial backup artifact.

**Symptoms:**
- Log: `BackupManager: backup incomplete – 3 of 47 SST files transferred`
- Subsequent restores fail checksum validation

**Cause:** Network interruption or target storage quota exceeded.

**Solution:**
```bash
# List backup status
themisdb-admin backup list --show-incomplete

# Delete incomplete backups and retry
themisdb-admin backup delete --id <incomplete-id>
themisdb-admin backup create \
  --destination s3://themisdb-backups/$(date +%Y%m%d) \
  --verify-checksums
```

## Diagnostic Commands

```bash
# Overall storage health
themisdb-admin storage health

# RocksDB statistics dump
themisdb-admin storage stats --verbose

# Compaction queue depth
themisdb-admin storage compaction-stats

# List PITR snapshots
themisdb-admin pitr list

# Check blob backend connectivity
themisdb-admin storage blob-check

# WAL file listing
ls -lh /var/lib/themisdb/data/*.log

# Live metrics via Prometheus
curl -s http://localhost:9100/metrics | grep themisdb_storage

# Tail storage-related log lines
journalctl -u themisdb -f | grep -E "storage|rocksdb|WAL|blob|compaction"
```

## Configuration Reference

```yaml
storage:
  rocksdb:
    data_path: /var/lib/themisdb/data
    wal_dir: /var/lib/themisdb/wal
    max_open_files: 10000
    max_background_jobs: 4
    max_background_compactions: 2
    max_background_flushes: 1
    block_cache_size_mb: 512
    write_buffer_size_mb: 64
    max_write_buffer_number: 3
    level0_slowdown_writes_trigger: 20
    level0_stop_writes_trigger: 36
    target_file_size_base: 67108864
    compaction_style: level          # "level" | "universal" | "fifo"
    wal_ttl_seconds: 3600
    enable_statistics: true
  pitr:
    enabled: true
    snapshot_interval_minutes: 30
    retention_days: 7
  blob:
    backend: filesystem              # "filesystem" | "s3" | "azure" | "webdav"
    redundancy:
      min_replicas: 2
  disk_monitor:
    warn_threshold_pct: 80
    critical_threshold_pct: 90
```

**Common misconfigurations:**

| Key | Wrong | Correct |
|-----|-------|---------|
| `max_background_jobs` | `1` | `≥ 4` on SSDs |
| `block_cache_size_mb` | `8192` on 8 GB host | `≤ 2048` |
| `wal_ttl_seconds` | `0` (disabled) | `3600` |
| `compaction_style` | `universal` under high write load | `level` |

## Known Limitations

- RocksDB `universal` compaction style can cause temporary 2× space amplification during full compaction.
- PITR snapshots for databases >1 TB may take >10 minutes; plan maintenance windows accordingly.
- Azure Blob backend does not support server-side copy for blobs larger than 256 MB (uses chunked upload).
- `BlobRedundancyManager` quorum writes add ~30% latency over single-node writes.
- `disk_space_monitor` does not monitor per-column-family disk usage, only the data directory total.
- WAL TTL cleanup is approximate; actual cleanup occurs at the next RocksDB flush, not on a strict timer.

## Related Documentation

- [Storage Module ROADMAP](../../src/storage/ROADMAP.md)
- [Backup & Recovery Guide](../backup_recovery_system.md)
- [PITR Implementation](../ARCHIVED/implementation-summaries/PITR_IMPLEMENTATION_COMPLETE.md)
- [RocksDB Wrapper Audit](../Audit/ROCKSDB_WRAPPER_AUDIT_REPORT.md)
- [Cloud Blob Backends](../storage/CLOUD_BLOB_BACKENDS.md)
- [WAL & gRPC mTLS Configuration](../architecture/WAL_GRPC_MTLS_CONFIGURATION.md)
