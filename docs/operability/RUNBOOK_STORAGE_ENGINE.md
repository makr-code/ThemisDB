# Runbook: Storage Engine — Wave D Operability

<!-- ThemisDB | docs/operability/RUNBOOK_STORAGE_ENGINE.md -->
<!-- Module: storage | Wave: D | Status: delivered Q1 2027 -->

## Overview

This runbook covers operator-critical incident scenarios for the ThemisDB
storage engine.  Each section describes the trigger condition, diagnostic steps,
log patterns to search, and remediation actions.

---

## Scenario 1 — WAL Corruption

### Trigger
The Write-Ahead Log contains one or more entries with invalid checksums, missing
sequence numbers, or truncated records.  Affected recovery or replay operations
will fail and the node will refuse to accept writes until the corruption is
addressed.

### Log pattern
```
[STORAGE:WALCorruption] segment=<path> offset=<bytes> expected_crc=<hex> actual_crc=<hex> seq_gap=<n>
```

### Diagnostic steps
1. Search for `[STORAGE:WALCorruption]` at the time of the incident.
2. Record `segment=` path and `offset=` value.
3. Run the WAL verification tool:
   ```
   themis-admin storage wal-verify --segment <path>
   ```
4. Determine the last clean checkpoint LSN:
   ```
   themis-admin storage checkpoint-status
   ```
5. Assess data loss window: `seq_gap` field shows how many write operations
   fall in the unrecoverable range.

### Remediation
| Cause | Action |
|-------|--------|
| Disk write fault | Replace disk; restore from the last clean checkpoint + replica catch-up. |
| Power loss / partial flush | Roll back to last checkpoint; replay from WAL up to the bad segment. |
| Software bug (unexpected) | Disable writes; escalate; restore from backup. |

**Recovery command:**
```bash
themis-admin storage recover --from-checkpoint <lsn> --skip-corrupt-segment <path>
```

### Escalation
WAL corruption is a P0 incident.  Activate the incident bridge immediately.
Do not attempt in-place repair without a verified backup available.

---

## Scenario 2 — Compaction Stall

### Trigger
The compaction process is not making progress.  Write amplification grows,
storage usage increases, and read latency degrades as the LSM level depth
increases.

### Log pattern
```
[STORAGE:CompactionStall] level=<n> pending_files=<n> stall_ms=<ms> reason=<resource_contention|priority_inversion|io_throttle>
```

### Diagnostic steps
1. Search for `[STORAGE:CompactionStall]` in the last 15 minutes.
2. Check `level=` and `pending_files=` — high pending files at L0 indicates
   an ingestion rate exceeding compaction throughput.
3. Inspect `storage_compaction_pending_bytes` and `storage_write_stall_active`
   metrics in Prometheus.
4. Check I/O utilization: `iostat -x 1 5` on the storage node.
5. Verify compaction thread count: `themis-admin storage compaction-status`.

### Remediation
| Cause | Action |
|-------|--------|
| I/O throttle too aggressive | Increase `storage.compaction_rate_limit_mb_s`. |
| Priority inversion | Set `storage.compaction_priority=high` and restart compaction worker. |
| Resource contention | Reduce `storage.max_background_jobs` to free I/O bandwidth. |
| Write ingestion too high | Temporarily enable write rate limiting: `storage.write_rate_limit_mb_s`. |

### Escalation
If compaction stall exceeds 5 minutes with >500 pending files, throttle writes
at the load balancer and page the storage on-call team.

---

## Scenario 3 — Blob Tier Mount Failure

### Trigger
A cold or warm blob storage backend (NFS, object store, or HDD mount) fails to
mount or becomes inaccessible.  Blob writes to the affected tier fail and
tiering decisions fall back to hot (NVMe) storage, potentially exhausting it.

### Log pattern
```
[STORAGE:BlobMountFailed] tier=<warm|cold> backend=<path_or_url> reason=<mount_error|auth_error|network_timeout> fallback=<hot|reject>
```

### Diagnostic steps
1. Search for `[STORAGE:BlobMountFailed]` in the last 10 minutes.
2. Note `tier=` and `backend=` to identify the affected backend.
3. Verify mount status:
   - NFS/local: `df -h` and `mount | grep <path>`
   - Object store: `themis-admin storage blob-backend-health --tier <warm|cold>`
4. Check `storage_blob_mount_error_total` metric by tier label.
5. If `fallback=hot`, monitor hot-tier disk usage immediately.

### Remediation
| Cause | Action |
|-------|--------|
| NFS mount dropped | Re-mount: `mount -t nfs <server>:<path> <mountpoint>`. |
| Object store auth expired | Rotate credentials; update `storage.cold.credentials_secret`. |
| Network timeout | Check connectivity to blob backend; review firewall rules. |
| Hot tier filling due to fallback | Reduce ingestion rate or expand hot-tier capacity. |

### Escalation
If cold-tier mount failure persists >10 minutes and hot-tier utilization
exceeds 80%, declare a storage capacity incident and activate overflow routing.

---

## Scenario 4 — PITR Restore Failure

### Trigger
A Point-in-Time Recovery (PITR) restore operation fails before completion.
Data at the requested timestamp is not fully restored.

### Log pattern
```
[STORAGE:PITRFailed] target_ts=<iso8601> last_clean_lsn=<n> reason=<missing_segment|checksum_mismatch|timeline_gap|decompression_error>
```

### Diagnostic steps
1. Search for `[STORAGE:PITRFailed]` at the time of the restore attempt.
2. Record `target_ts=` and `reason=`.
3. Check backup manifest integrity:
   ```
   themis-admin backup verify --target-ts <iso8601>
   ```
4. List available WAL segments covering the target timestamp:
   ```
   themis-admin storage wal-list --from <base_lsn> --to <target_ts>
   ```
5. If `reason=timeline_gap`, a failover may have created a divergent timeline;
   confirm whether the target timestamp pre-dates or post-dates the failover.

### Remediation
| Cause | Action |
|-------|--------|
| Missing WAL segment | Restore segment from backup archive; retry PITR. |
| Checksum mismatch | Re-download segment from remote backup; verify provider integrity. |
| Timeline gap | Use `--allow-timeline-switch` flag if the divergence is expected. |
| Decompression error | Ensure `zstd`/`lz4` is installed at the correct version on restore host. |

### Escalation
PITR failure during a production restore is a P1 incident.  Do not proceed
without a verified backup available.  Engage the backup-and-recovery team.

---

## Scenario 5 — Write Stall

### Trigger
The storage engine is temporarily refusing or severely throttling writes due
to memory buffer saturation, L0 file count limits, or pending compaction
pressure.  Clients observe high write latency or `STORAGE_EXHAUSTED` errors.

### Log pattern
```
[STORAGE:WriteStall] reason=<l0_limit|memtable_full|pending_compaction_bytes|write_buffer_full> stall_ms=<ms> recovery_action=<throttle|block|reject>
```

### Diagnostic steps
1. Search for `[STORAGE:WriteStall]` in the last 5 minutes.
2. Identify `reason=` and `recovery_action=`:
   - `l0_limit` — too many L0 SST files pending compaction.
   - `memtable_full` — write buffer exhausted; compaction cannot keep up.
   - `pending_compaction_bytes` — large compaction backlog.
3. Check `storage_write_stall_active` metric — value 1 confirms active stall.
4. Monitor `storage_memtable_usage_bytes` and `storage_l0_file_count` metrics.
5. Verify compaction is running: `themis-admin storage compaction-status`.

### Remediation
| Cause | Action |
|-------|--------|
| L0 limit hit | Increase `storage.level0_slowdown_writes_trigger`; ensure compaction is healthy. |
| Memtable full | Increase `storage.write_buffer_size_mb`; add compaction threads. |
| Pending compaction | Temporarily reduce write ingestion rate; let compaction catch up. |
| Disk I/O saturated | Add disk bandwidth or move compaction to off-peak hours. |

**Emergency write unblock:**
```bash
themis-admin storage write-stall-override --allow-writes --duration 5m
# Use only when data loss risk is accepted and team is on-call
```

### Escalation
Write stalls exceeding 30 seconds cause client timeouts.  If recovery is not
observed within 2 minutes of intervention, escalate to storage on-call and
consider read-only failover mode.

---

## Related Resources

- `docs/operability/WAVE_D_ROADMAP.md` — Wave D acceptance checklist
- `src/storage/ROADMAP.md` — Wave D contribution items
- `include/storage/storage_api_contract.h` — error code taxonomy
- `include/storage/storage_error_diagnostics.h` — structured diagnostic events
- Prometheus alerts: `storage_wal_corruption_total`, `storage_compaction_stall_total`, `storage_blob_mount_error_total`, `storage_write_stall_active`
