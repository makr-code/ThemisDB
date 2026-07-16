# Temporal Troubleshooting Guide

The `temporal` module provides bi-temporal data management for ThemisDB, including system-versioned tables, business-time tracking, temporal queries, point-in-time snapshots, and conflict resolution for overlapping validity periods.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `TemporalIndex: version not found` | Retention window too short | Increase `temporal.retention_days` |
| Overlapping validity periods in insert | No overlap check enabled | Enable `temporal.overlap_prevention: true` |
| PITR query returns empty results | Snapshot not taken before requested time | Ensure snapshot interval covers query range |
| `SnapshotManager: snapshot limit exceeded` | Too many snapshots retained | Increase `temporal.max_snapshots` or run cleanup |
| Temporal query ignores system time | Missing `AS OF SYSTEM TIME` clause | Add explicit temporal clause |
| `TemporalAggregator: period boundary error` | Timezone mismatch in timestamp | Normalise all timestamps to UTC |
| Conflict resolution picks wrong version | Resolution strategy misconfigured | Review `temporal.conflict_resolution.strategy` |
| `BiTemporal: transaction_time not set` | Server clock not synchronised | Sync NTP; check `temporal.use_hlc: true` |
| Business time updates don't appear | Wrong time range in query | Use `FOR PERIOD_OF system_time` clause correctly |
| Historical query very slow | No temporal index on time field | Create temporal index on `valid_from`/`valid_to` |

## Common Issues

### Issue 1: Temporal Version Not Found for Historical Query

**Description:** Point-in-time query returns empty results because the version no longer exists.

**Symptoms:**
- Error: `TemporalQueryEngine: no version of document user-42 at 2024-01-01T00:00:00Z`
- Historical reports show gaps

**Cause:** Retention window is shorter than the query's historical range.

**Solution:**
```yaml
temporal:
  retention_days: 1825              # 5 years of history
  snapshot_interval_hours: 1
  compact_old_versions: true
  keep_every_nth_version_after_days: 90  # thin history after 90 days
```

---

### Issue 2: Overlapping Validity Periods on Insert

**Description:** Two versions of the same record have overlapping `valid_from`/`valid_to` periods.

**Symptoms:**
- Query at timestamp T returns two versions of the same document
- Log: `BiTemporal: overlap detected for key=product-99 period=[2025-01-01, 2025-06-30]`

**Cause:** Application is inserting overlapping periods without checking for conflicts.

**Solution:**
```yaml
temporal:
  overlap_prevention:
    enabled: true
    action: reject                  # "reject" | "close_previous" | "warn"
    key_fields: [entity_id, entity_type]
```
```sql
-- Close the previous period before inserting new
UPDATE products
  SET valid_to = @new_valid_from
  WHERE _key = @key AND valid_to = '9999-12-31'

INSERT INTO products { ..., valid_from: @new_valid_from, valid_to: '9999-12-31' }
```

---

### Issue 3: Point-in-Time Query Returns Wrong Data

**Description:** `AS OF` query returns the current state instead of the historical state.

**Symptoms:**
- `FOR p IN products AS OF SYSTEM TIME '2024-06-01T00:00:00Z' RETURN p` returns current data
- Log: `TemporalQueryEngine: snapshot not found for timestamp; using current`

**Cause:** Snapshot was not taken before the requested timestamp; no temporal index exists.

**Solution:**
```bash
# List available snapshots
themisdb-admin temporal snapshots list --collection products

# Take a snapshot now
themisdb-admin temporal snapshot create --collection products

# Set up scheduled snapshots
```
```yaml
temporal:
  snapshot_schedule: "0 * * * *"    # hourly snapshots
  snapshot_retention_count: 720     # keep 30 days of hourly snapshots
```

---

### Issue 4: Conflict Resolution Picks Old Version

**Description:** When two updates arrive for the same time period, the wrong version wins.

**Symptoms:**
- Older update overwrites a newer update
- Log: `TemporalConflictResolver: resolving conflict using strategy=last_write_wins`

**Cause:** Conflict resolution strategy does not use the business-time validity, only system time.

**Solution:**
```yaml
temporal:
  conflict_resolution:
    strategy: valid_time_priority    # "lww" | "valid_time_priority" | "manual" | "crdt"
    tiebreaker: system_time          # when valid_time equal, use system_time
    manual_review_queue: true        # queue conflicts for human review
```

---

### Issue 5: Temporal Aggregator Returns Wrong Subtotals

**Description:** Temporal aggregations produce subtotals that do not add up to the total.

**Symptoms:**
- Monthly totals summing to more than the yearly total
- Log: `TemporalAggregator: period boundary overlap detected`

**Cause:** Mixed timezone timestamps; `valid_from` in UTC+2 and `valid_to` in UTC.

**Solution:**
```yaml
temporal:
  timezone: UTC                      # normalise all timestamps to UTC
  strict_utc_enforcement: true       # reject non-UTC timestamps
```
```sql
-- Normalise timestamps on insert
INSERT INTO contracts {
  valid_from: DATE_ISO8601(DATE_TIMESTAMP(@valid_from_local, "UTC+2")),
  valid_to:   DATE_ISO8601(DATE_TIMESTAMP(@valid_to_local,   "UTC+2"))
}
```

---

### Issue 6: System Time Not Set Correctly (HLC)

**Description:** Bi-temporal records have incorrect `transaction_time` due to clock issues.

**Symptoms:**
- `transaction_time` for new records is in the past
- Log: `BiTemporal: HLC clock jumped backwards – using last observed time`

**Cause:** Server NTP not synced; multiple nodes have clock drift.

**Solution:**
```bash
# Sync NTP immediately
chronyc makestep
timedatectl set-ntp true
```
```yaml
temporal:
  use_hlc: true                      # Hybrid Logical Clock for distributed correctness
  hlc:
    max_drift_ms: 500                # reject HLC if drift > 500ms
    sync_interval_ms: 1000
```

---

### Issue 7: Temporal Index Not Used for Range Queries

**Description:** Historical queries scan all versions instead of using the temporal index.

**Symptoms:**
- EXPLAIN shows `CollectionScan` on temporal queries
- Historical queries take minutes on large collections

**Cause:** No temporal index exists on `valid_from`/`valid_to` fields.

**Solution:**
```bash
# Create temporal index
themisdb-admin index create \
  --collection contracts \
  --type range \
  --fields valid_from,valid_to \
  --sparse true

# Or compound temporal + entity key
themisdb-admin index create \
  --collection contracts \
  --type range \
  --fields entity_id,valid_from,valid_to
```

---

### Issue 8: Retention Manager Does Not Delete Old Versions

**Description:** Old temporal versions accumulate and disk usage grows unbounded.

**Symptoms:**
- Disk usage for `contracts` collection keeps growing
- Log: `RetentionManager: no retention policy for collection=contracts`

**Cause:** Temporal retention not configured for the collection.

**Solution:**
```yaml
temporal:
  retention:
    enabled: true
    default_retention_days: 1825    # 5 years
    collections:
      contracts:
        retention_days: 3650        # 10 years for contracts
      audit_logs:
        retention_days: 2555        # 7 years for compliance
    run_interval_hours: 24
    dry_run: false
```
```bash
# Run retention now
themisdb-admin temporal retention run --collection contracts --dry-run
themisdb-admin temporal retention run --collection contracts
```

## Diagnostic Commands

```bash
# Temporal collection status
themisdb-admin temporal status --collection contracts

# List snapshots
themisdb-admin temporal snapshots list --collection contracts

# Check HLC drift
themisdb-admin temporal hlc-stats

# Retention stats
themisdb-admin temporal retention status

# Live temporal metrics
curl -s http://localhost:9100/metrics | grep themisdb_temporal

# Tail temporal logs
journalctl -u themisdb -f | grep -E "temporal|bi.temporal|snapshot|retention|hlc|valid_time"
```

## Configuration Reference

```yaml
temporal:
  enabled: true
  use_hlc: true
  timezone: UTC
  retention_days: 1825
  snapshot_interval_hours: 24
  overlap_prevention:
    enabled: true
    action: reject
  conflict_resolution:
    strategy: valid_time_priority
    tiebreaker: system_time
  retention:
    enabled: true
    run_interval_hours: 24
```

## Known Limitations

- System-versioned tables only track `transaction_time` automatically; `valid_time` must be managed by the application.
- Temporal queries using `AS OF SYSTEM TIME` require a snapshot to exist; on-demand reconstruction is not yet supported.
- HLC clock guarantees monotonicity per node but not global ordering across nodes without additional synchronisation.
- Retention does not respect open transactions; records referenced by active transactions may not be deleted immediately.

## Related Documentation

- [Temporal Module ROADMAP](../../src/temporal/ROADMAP.md)
- [PITR Implementation Complete](../ARCHIVED/implementation-summaries/PITR_IMPLEMENTATION_COMPLETE.md)
- [Branching Docs Index](../BRANCHING_DOCS_INDEX.md)
- [Distributed Transactions](../DISTRIBUTED_TRANSACTIONS.md)
