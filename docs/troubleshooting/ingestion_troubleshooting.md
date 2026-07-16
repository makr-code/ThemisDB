# Ingestion Troubleshooting Guide

The `ingestion` module manages data ingestion pipelines for ThemisDB from multiple sources, including HuggingFace Hub datasets, filesystem paths, and external HTTP/REST APIs, with support for checkpointing, quarantine, and parallel workers.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `HuggingfaceConnector: 401 Unauthorized` | Missing or expired HF token | Set `ingestion.huggingface.token` |
| `FilesystemIngester: path not found` | Wrong glob pattern or path | Check `ingestion.filesystem.path` |
| `ApiConnector: rate limited` | External API rate limit exceeded | Enable `ingestion.api.rate_limiter` |
| Ingestion stalls at checkpoint | Checkpoint storage full | Clear old checkpoints; increase storage |
| `IngestionManager: queue overflow` | Too many concurrent jobs | Increase `ingestion.queue_size` |
| Quarantine files not retried | Auto-retry disabled | Set `ingestion.quarantine.auto_retry: true` |
| Large dataset download OOM | Full download to memory | Enable `ingestion.streaming: true` |
| Duplicate records ingested | Deduplication not enabled | Enable `ingestion.dedup.enabled: true` |
| Ingestion job silent fail | Error not logged | Enable `ingestion.log_errors: true` |
| HuggingFace pagination stalls | Page size too large | Reduce `ingestion.huggingface.page_size` |

## Common Issues

### Issue 1: HuggingFace Token Authentication Failure

**Description:** HuggingFace dataset ingestion fails with authentication error.

**Symptoms:**
- Log: `HuggingfaceConnector: HTTP 401 Unauthorized`
- Private dataset cannot be downloaded

**Cause:** HuggingFace access token not configured or expired.

**Solution:**
```yaml
ingestion:
  huggingface:
    token: ${HF_TOKEN}
    base_url: https://huggingface.co
    timeout_ms: 60000
    retry_max: 3
```
```bash
# Test HF token
curl -H "Authorization: Bearer $HF_TOKEN" \
  "https://huggingface.co/api/datasets/squad"
```

---

### Issue 2: Filesystem Ingestion Path Not Found

**Description:** Filesystem ingester cannot find files matching the configured glob pattern.

**Symptoms:**
- Log: `FilesystemIngester: no files matched pattern '/data/exports/*.jsonl'`
- Ingestion job completes with 0 records

**Cause:** Path or glob pattern is wrong; files have different extension.

**Solution:**
```yaml
ingestion:
  filesystem:
    path: /data/exports/
    pattern: "**/*.jsonl"         # recursive glob
    recursive: true
    follow_symlinks: false
    exclude_patterns:
      - "*.tmp"
      - ".hidden*"
```
```bash
# Test glob pattern
find /data/exports/ -name "*.jsonl"

# List matched files
themisdb-admin ingestion filesystem-list \
  --path /data/exports/ \
  --pattern "*.jsonl"
```

---

### Issue 3: External API Rate Limited

**Description:** Ingestion from an external REST API is throttled.

**Symptoms:**
- Log: `ApiConnector: HTTP 429 Too Many Requests`
- Ingestion rate drops to near zero

**Cause:** External API has rate limits; ThemisDB is not respecting them.

**Solution:**
```yaml
ingestion:
  api:
    rate_limiter:
      enabled: true
      requests_per_second: 10      # throttle to 10 RPS
      burst: 20
    retry:
      on_rate_limit: true
      backoff_ms: 5000             # wait 5s after rate limit response
      max_retries: 10
    pagination:
      strategy: cursor             # "cursor" | "page" | "offset"
      page_size: 100
```

---

### Issue 4: Ingestion OOM for Large Datasets

**Description:** Ingesting a large HuggingFace dataset exhausts system memory.

**Symptoms:**
- Process OOM-killed during download
- Log: `HuggingfaceConnector: downloading entire dataset to memory`

**Cause:** Streaming not enabled; full dataset loaded.

**Solution:**
```yaml
ingestion:
  streaming: true
  batch_size: 1000
  max_memory_mb: 2048
  huggingface:
    use_streaming: true           # use HF streaming API
    split: train                  # only ingest training split
    max_rows: 0                   # 0 = no limit
```

---

### Issue 5: Quarantined Files Not Retried

**Description:** Files that failed validation are moved to quarantine and never retried.

**Symptoms:**
- Quarantine directory grows over time
- Log: `IngestionManager: file quarantined: /var/lib/themisdb/quarantine/bad_data.jsonl`

**Cause:** Auto-retry disabled; quarantine timeout not set.

**Solution:**
```yaml
ingestion:
  quarantine:
    enabled: true
    path: /var/lib/themisdb/quarantine/
    auto_retry: true
    retry_after_hours: 24         # retry after 24 hours
    max_retries: 3
    notify_on_permanent_failure: true
```
```bash
# Manually retry quarantined files
themisdb-admin ingestion quarantine list
themisdb-admin ingestion quarantine retry --all
```

---

### Issue 6: Duplicate Records Ingested

**Description:** Re-running an ingestion job creates duplicate records.

**Symptoms:**
- Collection row count doubles after re-ingestion
- No error in logs; just silent duplicates

**Cause:** Deduplication disabled; no checkpoint tracking.

**Solution:**
```yaml
ingestion:
  dedup:
    enabled: true
    key_fields: [id, source, created_at]
    strategy: upsert              # "upsert" | "skip" | "error"
  checkpoint:
    enabled: true
    path: /var/lib/themisdb/ingestion-checkpoints/
    resume_on_restart: true
```

---

### Issue 7: Ingestion Job Silently Fails

**Description:** Ingestion job reports success but no records are actually ingested.

**Symptoms:**
- Job status is `COMPLETED` but collection has 0 new records
- No error in logs

**Cause:** Error logging disabled; transform step silently drops all records.

**Solution:**
```yaml
ingestion:
  log_errors: true
  error_threshold_pct: 5.0        # fail job if > 5% of records error
  dry_run_first: false
  transform:
    on_error: log_and_skip         # "abort" | "log_and_skip" | "quarantine"
    log_transform_errors: true
```

---

### Issue 8: API Connector Pagination Stalls

**Description:** Paginated API ingestion stops after the first page.

**Symptoms:**
- Only the first 100 records are ingested
- Log: `ApiConnector: next_cursor is null; stopping`

**Cause:** API returns null cursor for empty intermediate pages.

**Solution:**
```yaml
ingestion:
  api:
    pagination:
      strategy: page
      page_param: page
      size_param: limit
      page_size: 100
      total_field: total_count
      stop_on_empty_page: true
      next_cursor_field: next_cursor
      cursor_null_action: stop    # "stop" | "continue_with_offset"
```

## Diagnostic Commands

```bash
# List ingestion jobs
themisdb-admin ingestion jobs list

# Job progress
themisdb-admin ingestion job status --job-id ing-abc123

# Quarantine contents
themisdb-admin ingestion quarantine list

# Test HF connectivity
themisdb-admin ingestion test-source \
  --type huggingface \
  --dataset squad

# Live ingestion metrics
curl -s http://localhost:9100/metrics | grep themisdb_ingestion

# Tail ingestion logs
journalctl -u themisdb -f | grep -E "ingestion|huggingface|filesystem.ingest|api.connector|quarantine"
```

## Configuration Reference

```yaml
ingestion:
  enabled: true
  batch_size: 1000
  streaming: true
  dedup:
    enabled: true
    strategy: upsert
  checkpoint:
    enabled: true
    path: /var/lib/themisdb/ingestion-checkpoints/
  quarantine:
    enabled: true
    auto_retry: true
    retry_after_hours: 24
  huggingface:
    token: ${HF_TOKEN}
  api:
    rate_limiter:
      enabled: true
      requests_per_second: 10
```

## Known Limitations

- HuggingFace streaming API does not support random access; ingestion must be sequential.
- Filesystem ingester processes files in alphabetical order; no priority scheduling.
- API connector pagination only supports cursor, page-number, and offset strategies.
- Checkpoint file stores only byte offsets; schema changes in the source require full re-ingestion.

## Related Documentation

- [Ingestion Module ROADMAP](../../src/ingestion/ROADMAP.md)
- [Ingestion Roadmap](../de/roadmap/ingestion_roadmap.md)
- [Exporters Troubleshooting](./exporters_troubleshooting.md)
- [Content Troubleshooting](./content_troubleshooting.md)
