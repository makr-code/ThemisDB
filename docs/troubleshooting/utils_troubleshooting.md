# Utils Troubleshooting Guide

The `utils` module provides shared utility components for ThemisDB, including an audit logger, cron parser, cursor management, error registry, compression utilities, build info, and capability management infrastructure.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `AuditLogger: write failed` | Audit log storage full or permissions wrong | Free disk space; fix permissions |
| Cron expression fails to parse | Non-standard syntax | Use 5-field POSIX cron syntax |
| `CursorManager: cursor expired` | Cursor TTL too short | Increase `utils.cursor.ttl_ms` |
| `ErrorRegistry: error code not found` | Custom error not registered | Register error code before use |
| Compression ratio worse than expected | Wrong algorithm for data type | Use `lz4` for speed, `zstd` for ratio |
| `BuildInfo: version mismatch` | Binary not rebuilt after source change | Rebuild all modules |
| Audit log PII leak | PII scrubbing disabled | Enable `utils.audit.pii_scrubbing` |
| `CapabilityAutoGenerator: no capabilities detected` | Service annotations missing | Add capability annotations |
| Compression CPU spike | High compression level | Reduce `utils.compression.level` |
| `cursor: EOF on valid cursor` | Cursor consumed but not cloned | Use `cursor.clone()` for multiple consumers |

## Common Issues

### Issue 1: Audit Logger Write Failure

**Description:** Audit log writes fail due to disk space or permissions.

**Symptoms:**
- Log: `AuditLogger: write failed: ENOSPC on /var/log/themisdb/audit.log`
- Audit records lost

**Cause:** Disk full or log directory not writable.

**Solution:**
```bash
# Check disk space
df -h /var/log/themisdb/

# Fix permissions
chown -R themisdb:themisdb /var/log/themisdb/
chmod 750 /var/log/themisdb/

# Rotate logs manually
logrotate --force /etc/logrotate.d/themisdb
```
```yaml
utils:
  audit:
    path: /var/log/themisdb/audit.log
    max_size_mb: 500
    max_files: 10
    compress: true
    async_write: true
    buffer_size: 10000
```

---

### Issue 2: Cron Expression Parse Failure

**Description:** A cron expression fails to parse at startup.

**Symptoms:**
- Log: `CronParser: invalid cron expression '@reboot': unsupported macro`
- Scheduled task not created

**Cause:** Non-standard cron syntax (macros like `@hourly`, `@reboot`, seconds field).

**Solution:**
```bash
# Test cron expression
themisdb-admin utils cron-parse "0 * * * *"

# Valid 5-field POSIX syntax
# minute hour day-of-month month day-of-week
# 0 2 * * *  = daily at 02:00
# */15 * * * * = every 15 minutes
```
```yaml
# Always use 5-field format
utils:
  cron:
    timezone: UTC
    allow_seconds: false          # 5-field only
    allow_macros: false
```

---

### Issue 3: Cursor Expires During Pagination

**Description:** A client paginating through results receives `cursor expired` error.

**Symptoms:**
- Error: `CursorManager: cursor c_abc123 has expired`
- Client must restart pagination from beginning

**Cause:** TTL too short for the client's pagination speed.

**Solution:**
```yaml
utils:
  cursor:
    ttl_ms: 3600000               # 1 hour instead of 5 minutes
    max_active_cursors: 10000
    cleanup_interval_ms: 60000
    extend_on_access: true        # reset TTL on each page access
```

---

### Issue 4: Audit Log Contains PII

**Description:** User data (emails, IPs) is present in the audit log unredacted.

**Symptoms:**
- Audit log contains `"user_email": "alice@example.com"`
- Compliance issue in audit report

**Cause:** PII scrubbing disabled.

**Solution:**
```yaml
utils:
  audit:
    pii_scrubbing:
      enabled: true
      fields:
        - user_email
        - ip_address
        - ssn
        - credit_card
      strategy: hash              # "hash" | "redact" | "remove"
```

---

### Issue 5: Compression CPU Overhead

**Description:** Compression utility uses excessive CPU.

**Symptoms:**
- `top` shows high CPU from compression threads
- Log: `CompressionMetrics: cpu_pct=80 for zstd level=19`

**Cause:** Compression level too high; zstd level 19 is very slow.

**Solution:**
```yaml
utils:
  compression:
    default_algorithm: lz4        # fast and low CPU
    level: 1                      # minimal compression effort
    # Or use zstd at lower level:
    # default_algorithm: zstd
    # level: 3
    thread_count: 2               # limit compression threads
```

---

### Issue 6: Error Registry Code Not Found

**Description:** An error code used by application code is not in the registry.

**Symptoms:**
- Error: `ErrorRegistry: error code E_CUSTOM_001 not found`
- Generic error returned instead of detailed one

**Cause:** Custom error code not registered at startup.

**Solution:**
```yaml
utils:
  error_registry:
    custom_errors_file: /etc/themisdb/custom_errors.yaml
    auto_load: true
```
```yaml
# /etc/themisdb/custom_errors.yaml
errors:
  - code: E_CUSTOM_001
    message: "Custom operation failed"
    http_status: 422
    retryable: true
```

## Diagnostic Commands

```bash
# Audit log statistics
themisdb-admin utils audit-stats

# Test cron expression
themisdb-admin utils cron-parse "0 2 * * *"

# Active cursors
themisdb-admin utils cursors list

# Compression benchmark
themisdb-admin utils compression-benchmark \
  --algorithm lz4 \
  --data-file /tmp/test.json

# Build info
themisdb --version
themisdb-admin utils build-info

# Error registry
themisdb-admin utils error-codes list

# Tail utils logs
journalctl -u themisdb -f | grep -E "audit.log|cursor|cron.parse|compress|error.reg"
```

## Configuration Reference

```yaml
utils:
  audit:
    path: /var/log/themisdb/audit.log
    max_size_mb: 500
    async_write: true
    pii_scrubbing:
      enabled: true
  cursor:
    ttl_ms: 1800000
    extend_on_access: true
  compression:
    default_algorithm: lz4
    level: 1
  cron:
    timezone: UTC
    allow_seconds: false
```

## Known Limitations

- Audit logger async write mode may lose up to `buffer_size` records on unclean shutdown.
- Cron parser does not support Quartz scheduler format (7-field with seconds); use 5-field POSIX.
- Cursor state is in-memory only; cursors are lost on server restart.
- Compression utility does not support streaming decompression for very large payloads.

## Related Documentation

- [Utils Module ROADMAP](../../src/utils/ROADMAP.md)
- [Observability Troubleshooting](./observability_troubleshooting.md)
- [Audit Log Retention](../ARCHIVED/implementation-summaries/AUDIT_LOG_RETENTION_IMPLEMENTATION.md)
- [Compression Configuration](../build-guide/compression_configuration.md)
- [Compression and Encoding Strategies](../performance/compression_and_encoding_strategies.md)
