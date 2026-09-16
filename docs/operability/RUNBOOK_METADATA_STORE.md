# Runbook: Metadata Store

> **Module**: `src/metadata`
> **Wave**: D
> **Version**: 1.0.0
> **Owner**: ThemisDB Platform Engineering
> **See also**: [`src/metadata/ROADMAP.md`](../../src/metadata/ROADMAP.md),
> [`docs/operability/WAVE_D_ROADMAP.md`](WAVE_D_ROADMAP.md)

---

## Purpose

This runbook provides operator triage and remediation procedures for the
ThemisDB Metadata Store. It covers the five operator-critical failure
scenarios identified during Wave D.

---

## Scenario 1 — Index Corruption

### Symptom
Log pattern: `[METADATA:IndexCorruption]`

Metadata lookups return empty results for keys that are known to exist.
Index inconsistency may propagate to query planning and schema validation.

### Diagnosis
1. Search logs for `[METADATA:IndexCorruption]` entries; note the affected
   key prefixes and timestamps.
2. Run the metadata index integrity check:
   ```
   themis-ctl metadata index-verify
   ```
3. Identify whether the corruption is isolated to one partition or systemic.

### Remediation
1. Rebuild the affected index partition:
   ```
   themis-ctl metadata index-rebuild --partition <PARTITION_ID>
   ```
2. If the corruption is systemic, initiate a full index rebuild:
   ```
   themis-ctl metadata index-rebuild --full
   ```
3. Monitor index rebuild progress via the `metadata_index_rebuild_progress`
   metric.
4. Confirm `[METADATA:IndexCorruption]` clears in logs after rebuild.

---

## Scenario 2 — Schema Conflict

### Symptom
Log pattern: `[METADATA:SchemaConflict]`

Concurrent schema migrations produce version conflicts. One or more
migrations may have been silently rejected.

### Diagnosis
1. Check the schema version history for gaps or duplicate versions:
   ```
   themis-ctl metadata schema-history
   ```
2. Look for `[METADATA:SchemaConflict]` entries in logs; note the expected
   vs. actual version at conflict time.
3. Identify whether multiple schema mutators are running concurrently.

### Remediation
1. Serialize schema migrations by ensuring only one migration agent runs
   at a time:
   ```
   themis-ctl config set metadata.schema.concurrent_migrations=1
   ```
2. Re-apply any rejected migrations in order:
   ```
   themis-ctl metadata schema-apply --migration <MIGRATION_ID>
   ```
3. Validate the final schema version matches expectations:
   ```
   themis-ctl metadata schema-verify
   ```
4. Monitor for `[METADATA:SchemaConflict]` clearance.

---

## Scenario 3 — Partition Inconsistency

### Symptom
Log pattern: `[METADATA:PartitionInconsistency]`

Metadata records are missing from one or more partitions. Cross-partition
queries may return incomplete result sets.

### Diagnosis
1. Run the partition consistency check:
   ```
   themis-ctl metadata partition-verify --full
   ```
2. Look for `[METADATA:PartitionInconsistency]` log entries and identify
   the affected partition IDs.
3. Check replication lag for the affected partitions:
   ```
   themis-ctl metadata replication-status
   ```

### Remediation
1. If the inconsistency is due to replication lag, wait for replication to
   catch up (monitor `metadata_replication_lag_seconds`).
2. If a partition is permanently lost, restore from the last snapshot:
   ```
   themis-ctl metadata partition-restore --partition <PARTITION_ID> \
       --snapshot <SNAPSHOT_ID>
   ```
3. After restoration, run a full consistency check to confirm resolution:
   ```
   themis-ctl metadata partition-verify --full
   ```
4. Monitor for `[METADATA:PartitionInconsistency]` clearance.

---

## Scenario 4 — Exporter Lag

### Symptom
Log pattern: `[METADATA:ExporterLag]`

The metadata metrics/event exporter is lagging behind the store operations.
Schema change events may be delayed or dropped.

### Diagnosis
1. Check the exporter queue depth:
   ```
   themis-ctl exporter status --module metadata
   ```
2. Look for `[METADATA:ExporterLag]` entries and note the lag duration.
3. Verify that the export destination is reachable.

### Remediation
1. If the destination is unreachable, restore connectivity; the exporter
   will drain automatically.
2. If the queue is persistently full, increase the buffer:
   ```
   themis-ctl config set metadata.exporter.queue_depth=<N>
   ```
3. Flush the exporter queue if data loss for the window is acceptable:
   ```
   themis-ctl exporter flush --module metadata
   ```
4. Monitor the `metadata_exporter_lag_seconds` metric for clearance.

---

## Scenario 5 — Deep Concurrent Access Regression

### Symptom
Performance regression under deep concurrent access (16+ threads). Latency
spikes or errors appear under sustained concurrent metadata operations.

### Diagnosis
1. Run the deep concurrent access stress test to reproduce:
   ```
   ctest -R DeepConcurrentAccessStress -V
   ```
2. Profile lock contention on `metadata_store_mutex` using `perf lock`.
3. Review recent changes to schema_manager or statistics_collector for
   accidental lock re-acquisition.

### Remediation
1. If lock contention is the root cause, migrate the hot path to a
   shared-lock (read-write mutex):
   ```
   # Review src/metadata/src/schema_manager.cpp for lock upgrade opportunities
   ```
2. If a recent change introduced a regression, revert and raise a fix PR.
3. Re-run the stress test after remediation to confirm resolution.
4. Update `docs/operability/WAVE_D_ACCEPTANCE_CHECKLIST.md` with results.

---

## Log Pattern Reference

| Pattern                              | Scenario                              |
|--------------------------------------|---------------------------------------|
| `[METADATA:IndexCorruption]`         | Index inconsistency detected          |
| `[METADATA:SchemaConflict]`          | Concurrent schema migration conflict  |
| `[METADATA:PartitionInconsistency]`  | Partition data missing or stale       |
| `[METADATA:ExporterLag]`             | Metrics exporter queue lag            |

---

## Escalation

If remediation steps do not resolve the incident within 30 minutes, escalate
to the ThemisDB on-call engineer with:
- Relevant log excerpts (tagged with the log patterns above)
- Output of `themis-ctl metadata index-verify`
- Output of `themis-ctl metadata partition-verify --full`
- Output of `themis-ctl metadata schema-history`
