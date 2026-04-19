# CDC Troubleshooting Guide

The `cdc` module provides Change Data Capture for ThemisDB, streaming database changes to consumers via changefeeds, managing tenant-specific buffer allocations, and offering an administrative API for managing CDC pipelines.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `ChangefeedBuffer: overflow` | Consumer too slow | Increase buffer or consumer throughput |
| CDC consumer lag > 60s | Slow downstream or network | Check consumer latency; increase `batch_size` |
| `TenantBufferManager: quota exceeded` | Tenant CDC quota too low | Increase `cdc.tenant.max_buffer_mb` |
| Events missing for some collections | Collection not in CDC scope | Add collection to `cdc.collections` |
| Changefeed not starting | Missing initial sequence | Run `themisdb-admin cdc reset --collection orders` |
| Duplicate events on reconnect | `resume_token` not persisted | Persist resume token in consumer |
| CDC auth failure | CDC endpoint needs auth token | Include `Authorization: Bearer` header |
| Rate limiter blocks CDC consumer | Consumer request rate too high | Add consumer to CDC whitelist |
| Events delivered out of order | Multi-shard without ordering enabled | Enable `cdc.ordered: true` |
| `cdc_admin: collection not enabled` | CDC not enabled for this collection | Enable via admin API |

## Common Issues

### Issue 1: Changefeed Buffer Overflow

**Description:** The CDC buffer overflows because the consumer cannot process events fast enough.

**Symptoms:**
- Log: `ChangefeedBuffer: buffer full for changefeed=orders-changes; dropping events`
- Consumer reports missing events

**Cause:** Consumer processing is slower than the write rate; buffer too small.

**Solution:**
```yaml
cdc:
  buffer:
    max_size_mb: 512               # increase from default 64
    overflow_strategy: block       # "block" | "drop" | "error"
    flush_interval_ms: 100
```
```bash
# Monitor buffer usage
themisdb-admin cdc buffer-stats --changefeed orders-changes

# Check consumer lag
themisdb-admin cdc lag --changefeed orders-changes
```

---

### Issue 2: Events Missing After Consumer Reconnect

**Description:** After a consumer disconnects and reconnects, some events are missing.

**Symptoms:**
- Consumer reports gap in sequence numbers after reconnect
- Log: `Changefeed: resume token not found – starting from current`

**Cause:** Consumer did not persist the resume token before disconnecting.

**Solution:**
```python
# Always persist resume token
def on_event(event, resume_token):
    process_event(event)
    save_to_persistent_store(resume_token)  # save AFTER processing

# Reconnect with saved token
client.watch("orders", resume_after=load_resume_token())
```
```yaml
cdc:
  resume_token:
    ttl_seconds: 86400             # keep resume tokens for 24 hours
    storage: rocksdb               # persist tokens in RocksDB
```

---

### Issue 3: Duplicate Events on Restart

**Description:** After a server restart, events are delivered more than once.

**Symptoms:**
- Consumer processes the same event twice
- Downstream database has duplicate rows

**Cause:** At-least-once delivery semantics; consumer must implement idempotency.

**Solution:**
```yaml
cdc:
  delivery_semantics: exactly_once   # requires compatible consumer
  dedup_window_ms: 300000
```
```python
# Implement idempotent consumer
def handle_event(event):
    # Use event._id as idempotency key
    if not already_processed(event["_id"]):
        apply_to_sink(event)
        mark_processed(event["_id"])
```

---

### Issue 4: High Consumer Lag

**Description:** CDC consumer is far behind real-time events.

**Symptoms:**
- `themisdb_cdc_consumer_lag_ms > 60000`
- Log: `Changefeed: consumer orders-sink is 5000 events behind`

**Cause:** Downstream system is slow; batch size too small.

**Solution:**
```yaml
cdc:
  consumer:
    batch_size: 1000               # process in larger batches
    batch_timeout_ms: 100
    parallel_consumers: 4          # parallelise by shard
    backpressure: true
```

---

### Issue 5: CDC Not Enabled for Collection

**Description:** Attempting to create a changefeed fails because CDC is not enabled.

**Symptoms:**
- Error: `CdcAdmin: CDC not enabled for collection 'products'`

**Cause:** Collection was not added to the CDC scope.

**Solution:**
```bash
# Enable CDC for a collection via admin API
curl -X POST http://localhost:9090/admin/cdc/enable \
     -H "Authorization: Bearer $ADMIN_TOKEN" \
     -d '{"collection": "products", "include_deletes": true}'

# Or via config
```
```yaml
cdc:
  collections:
    - name: orders
      include_inserts: true
      include_updates: true
      include_deletes: true
    - name: products
      include_updates: true
```

---

### Issue 6: Events Delivered Out of Order on Multi-Shard Collection

**Description:** Events for the same document arrive out of order.

**Symptoms:**
- Update event arrives before insert event for the same `_key`
- Consumer state is inconsistent

**Cause:** Events from different shards are merged without global ordering.

**Solution:**
```yaml
cdc:
  ordered: true                    # enforce global ordering (adds latency)
  ordering:
    strategy: timestamp            # "timestamp" | "sequence"
    max_reorder_buffer_ms: 500
```

---

### Issue 7: Tenant Buffer Quota Exceeded

**Description:** A tenant's CDC buffer is full and events are being dropped.

**Symptoms:**
- Log: `TenantBufferManager: tenant=tenant-abc exceeded CDC quota (512MB)`
- Tenant CDC consumer receives errors

**Cause:** Tenant's write rate produces more CDC events than its quota allows.

**Solution:**
```yaml
cdc:
  tenant:
    default_max_buffer_mb: 128
    max_buffer_mb: 1024            # per-tenant cap
    quota_strategy: block          # "block" | "drop" highest-priority-last

# Per-tenant override via admin API
```
```bash
themisdb-admin cdc set-tenant-quota \
  --tenant tenant-abc \
  --max-buffer-mb 512
```

---

### Issue 8: Rate Limiter Blocks CDC Consumer

**Description:** The CDC consumer is being rate-limited by the API.

**Symptoms:**
- Consumer receives HTTP 429
- Log: `AuthRateLimiter: CDC consumer 10.0.0.50 exceeded 100 req/min`

**Cause:** CDC consumer uses the same rate limit as regular API clients.

**Solution:**
```yaml
api:
  rate_limiter:
    cdc_consumer_exemption: true   # exempt CDC consumers from rate limiting
    whitelist_cidrs:
      - 10.0.0.0/8
```

## Diagnostic Commands

```bash
# List all changefeeds
themisdb-admin cdc list

# Consumer lag
themisdb-admin cdc lag

# Buffer statistics
themisdb-admin cdc buffer-stats

# Replay events from a specific sequence
themisdb-admin cdc replay \
  --changefeed orders-changes \
  --from-sequence 12345 \
  --to-sequence 12500

# Live CDC metrics
curl -s http://localhost:9100/metrics | grep themisdb_cdc

# Tail CDC logs
journalctl -u themisdb -f | grep -E "cdc|changefeed|buffer|tenant.buffer"
```

## Configuration Reference

```yaml
cdc:
  enabled: true
  delivery_semantics: at_least_once
  buffer:
    max_size_mb: 256
    overflow_strategy: block
  collections:
    - name: orders
      include_inserts: true
      include_updates: true
      include_deletes: true
  ordered: false
  tenant:
    default_max_buffer_mb: 64
  resume_token:
    ttl_seconds: 86400
```

## Known Limitations

- Exactly-once delivery requires a compatible consumer that supports transactional acknowledgement.
- Global ordering across shards adds latency proportional to the maximum shard lag.
- CDC does not capture DDL changes (schema migrations); use the `updates` module's change log.
- Resume tokens expire after `resume_token.ttl_seconds`; long consumer outages may require a full resync.

## Related Documentation

- [CDC Module ROADMAP](../../src/cdc/ROADMAP.md)
- [CDC Roadmap](../de/roadmap/cdc_roadmap.md)
- [CDC Operations Runbook](../CDC_OPERATIONS_RUNBOOK.md)
- [CDC Implementation Summary](../ARCHIVED/implementation-summaries/CDC_IMPLEMENTATION_SUMMARY.md)
- [Changefeed Authentication](../security/changefeed_authentication.md)
- [Task Scheduler CDC](../TASK_SCHEDULER_CRON_CDC.md)
