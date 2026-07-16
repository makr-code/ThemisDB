# Maintenance Schedule Runbook

## Overview

This document describes the operational maintenance schedules shipped with
ThemisDB.  All schedules are **disabled by default**.  Operators enable and
tune them via the CRUD API.

## Default Schedules

Four default schedules are registered automatically by `registerDefaultMaintenanceSetup()`.
They are inserted in disabled state so no maintenance runs without explicit operator approval.

### Daily Maintenance

**Cron:** `0 2 * * *` (02:00 UTC every day)

| Task | Purpose |
|------|---------|
| `metrics_collection` | Snapshot Prometheus counters, reset delta accumulators |
| `fragmentation_monitoring` | Scan all indexes for fragmentation %; emit alert if HIGH |
| `quota_check` | Validate storage quota thresholds; alert on > 80 % |

**Enable:**
```bash
# 1. Find the schedule id
curl -s -H "Authorization: Bearer $TOKEN" \
     https://themisdb/api/v1/maintenance/schedules \
  | jq '.schedules[] | select(.name=="Daily Maintenance") | .id'

# 2. Enable it
curl -X PATCH -H "Authorization: Bearer $TOKEN" \
     -H "Content-Type: application/json" \
     -d '{"enabled": true}' \
     https://themisdb/api/v1/maintenance/schedules/<id>
```

---

### Weekly Maintenance

**Cron:** `0 2 * * 0` (02:00 UTC every Sunday)

| Task | Purpose |
|------|---------|
| `consistency_check` | Validate index integrity; report inconsistencies |
| `replica_validation` | Check replica lag < threshold |
| `performance_analysis` | Identify slow queries and lock contention |
| `mvcc_cleanup` | GC stale MVCC versions older than retention window |

---

### Monthly Maintenance

**Cron:** `0 2 1 * *` (02:00 UTC on the 1st of each month)

| Task | Purpose |
|------|---------|
| `full_checkdb` | Full integrity scan (CHECKDB equivalent) |
| `backup_verification` | Restore verification of most-recent backup |
| `capacity_trend_analysis` | Project storage growth; raise alert if runway < 30 d |
| `index_fragmentation_report` | Produce full fragmentation report for all indexes |

---

### Quarterly Maintenance

**Cron:** `0 0 1 1,4,7,10 *` (00:00 UTC on Jan/Apr/Jul/Oct 1st)

| Task | Purpose |
|------|---------|
| `disaster_recovery_drill` | Simulate DR restore procedure; verify RTO/RPO |
| `baseline_update` | Refresh performance baselines for anomaly detection |

`halt_on_task_failure: true` – if the DR drill fails, baseline update is skipped
until the drill is remediated.

---

## Creating a Custom Schedule

```bash
curl -X POST -H "Authorization: Bearer $TOKEN" \
     -H "Content-Type: application/json" \
     -d '{
       "name": "Nightly Index Rebuild",
       "description": "Rebuild fragmented indexes every night",
       "frequency": "daily",
       "tasks": ["fragmentation_monitoring", "index_rebuild"],
       "window_start_hour": 1,
       "window_end_hour": 5,
       "halt_on_task_failure": false
     }' \
     https://themisdb/api/v1/maintenance/schedules
```

## Triggering a Schedule Immediately

```bash
curl -X POST -H "Authorization: Bearer $TOKEN" \
     https://themisdb/api/v1/maintenance/schedules/<id>/run
```

The response contains a job object with `"state": "running"`.
Poll `GET /api/v1/maintenance/jobs/<job_id>` until `state` is `succeeded` or `failed`.

## Monitoring

```bash
# Overall health
curl -H "Authorization: Bearer $TOKEN" https://themisdb/api/v1/maintenance/health

# Active jobs
curl -H "Authorization: Bearer $TOKEN" \
     "https://themisdb/api/v1/maintenance/jobs?active_only=true"
```

## Maintenance Windows

Each schedule has `window_start_hour` and `window_end_hour` (UTC).
When `enforce_window: true`, the orchestrator will skip (mark `SKIPPED`) any
job triggered outside the window.  This prevents maintenance from running
during peak traffic.

Recommended window: **01:00–06:00 UTC** for EMEA / APAC off-peak.

## Available Task Types

| Task Type | Description |
|-----------|-------------|
| `metrics_collection` | Prometheus metrics snapshot |
| `fragmentation_monitoring` | Index fragmentation scan |
| `quota_check` | Storage quota validation |
| `consistency_check` | Index / data integrity validation |
| `replica_validation` | Replica lag & consistency check |
| `performance_analysis` | Slow-query & lock-contention report |
| `mvcc_cleanup` | Stale MVCC version GC |
| `full_checkdb` | Full database integrity scan |
| `backup_verification` | Backup restore verification |
| `capacity_trend_analysis` | Storage growth projection |
| `index_fragmentation_report` | Full index fragmentation report |
| `disaster_recovery_drill` | DR drill simulation |
| `baseline_update` | Performance baseline refresh |
| `index_rebuild` | Full index rebuild |
| `index_reorganize` | In-place index defragmentation |
| `statistics_update` | Query-planner statistics refresh |
| `storage_compaction` | RocksDB compaction trigger |
| `orphan_cleanup` | Orphaned index entry removal |
| `vector_reindex` | Incremental HNSW re-index |
