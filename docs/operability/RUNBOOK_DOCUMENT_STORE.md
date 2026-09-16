# RUNBOOK: Document Store — Triage & Recovery

**Author:** ThemisDB Contributors
**Created:** 2026-09-16
**Last Updated:** 2026-09-16
**Status:** active

**Audience:** Database Operators, SREs, Document Module Team Lead
**Purpose:** Triage and recover from document store failures, merge conflict storms, schema migration failures, round-trip persistence loss, and XDOMEA exchange failures
**Severity:** High (document store is on the critical data path; write/read failures directly impact stored records)
**Estimated Duration:** 5 min - 2 hours (depending on failure class)

---

## Overview

This runbook guides operators through diagnosing and recovering from failures in the ThemisDB document store pipeline (`src/document/`). The module has five primary surfaces:

1. **Document store** — write/read/delete operations (`DocumentStore`, `DocumentManager`)
2. **Diff/merge engine** — three-way merge and conflict resolution (`DocumentDiff`, `DocumentMerge`)
3. **Schema evolution** — version transition and validation (`DocumentSchema`, `SchemaRegistry`)
4. **Round-trip persistence** — snapshot/restore editing (`RoundTripEditor`)
5. **XDOMEA connector** — structured records exchange (`XdomeaConnector`, exchange interfaces)

**Key Principles:**
- All store operations are fail-closed: malformed descriptors and schema violations are rejected, never silently accepted
- Merge conflict detection is mandatory; conflicts must be surfaced to the caller, not silently discarded
- Schema transitions are strictly versioned; downgrade paths must be declared in migration notes
- XDOMEA exchange failures must be logged with structured tags and never swallowed silently

---

## Prerequisites Checklist

Before beginning any intervention, verify:

- [ ] Direct document read/write access is being tested (not just XDOMEA or round-trip paths)
- [ ] Access to server logs with `[DOCUMENT:*]`, `[MERGE:*]`, `[SCHEMA:*]`, `[XDOMEA:*]` tag filtering
- [ ] Prometheus/Grafana dashboard showing document store metrics (`document_write_*`, `document_read_*`, `merge_conflict_*`)
- [ ] Knowledge of the active schema version for the affected document collection
- [ ] Wave D D1 trace spans are available (see §Distributed Tracing below) if Phase 2A is deployed

---

## Failure Scenarios

---

### Scenario 1: Document Store Unavailability

**Symptoms:**
- Logs contain `[DOCUMENT:StoreUnavailable]` or `[DOCUMENT:WriteFailure]` tags
- Write/read operations return error status or throw with store-unavailable messages
- Dashboard: `document_write_error_total` rising sharply

**Log patterns:**
```
[DOCUMENT:StoreUnavailable] DocumentStore::write: store backend unavailable for key <key>
[DOCUMENT:WriteFailure] DocumentManager::write: persistence layer error: <detail>
[DOCUMENT:ReadFailure] DocumentStore::read: key <key> not found or store unreachable
```

#### Step 1: Confirm Store Availability

```bash
# Search recent logs for store unavailability events
grep '\[DOCUMENT:StoreUnavailable\]\|\[DOCUMENT:WriteFailure\]' \
  /var/log/themisdb/themisdb.log | tail -20

# Check total write error rate
grep '\[DOCUMENT:WriteFailure\]' /var/log/themisdb/themisdb.log | \
  awk '{print $1, $2}' | cut -d: -f1,2 | sort | uniq -c | tail -10
```

#### Step 2: Diagnose Root Cause

| Symptom | Likely Cause | Action |
|---------|-------------|--------|
| All writes fail with `StoreUnavailable` | Storage backend down or disk full | Check disk usage and storage backend health |
| Intermittent write failures | I/O contention or transaction lock contention | Check I/O metrics; reduce write concurrency if needed |
| Reads fail for recently-written documents | WAL sync lag or cache invalidation | Verify WAL flush interval; check `document_cache_miss_total` |
| Only specific key ranges fail | Partition or shard-specific failure | Check shard health for the affected key range |

#### Step 3: Restore Store Access

```bash
# Check disk usage on the document store volume
df -h /var/lib/themisdb/documents

# If disk full: identify large document collections
du -sh /var/lib/themisdb/documents/* | sort -rh | head -10

# Restart the document store service component (soft reset, no data loss)
themisdb-admin document-store restart --safe

# Verify the store is accepting writes
themisdb-admin document-store test-write \
  --key "ops-probe-$(date +%s)" \
  --content '{"probe": true}' \
  --schema-version 1
```

#### Step 4: Validate Recovery

```bash
# Confirm write/read cycle succeeds
themisdb-admin document-store test-round-trip \
  --key "ops-validate-$(date +%s)" \
  --content '{"recovery_validated": true}'

# Confirm [DOCUMENT:StoreUnavailable] messages stop
grep '\[DOCUMENT:StoreUnavailable\]' /var/log/themisdb/themisdb.log | \
  awk '{print $1, $2}' | tail -5
```

**Decision Point:**
- ✅ **Recovery confirmed:** Write/read cycle succeeds, error rate drops to baseline → incident closed
- ❌ **Store still unavailable:** Escalate to storage infrastructure team; consider read-only mode

---

### Scenario 2: Merge Conflict Storm

**Symptoms:**
- Logs contain `[MERGE:ConflictDetected]` tags at high frequency
- Merge operations return conflict status for documents that should merge cleanly
- Dashboard: `merge_conflict_total` rising above baseline

**Log patterns:**
```
[MERGE:ConflictDetected] DocumentMerge::merge: conflict detected for key <key> at field <field>
[MERGE:ConflictStorm] DocumentMerge::merge: conflict rate exceeds threshold: <rate>/min
[MERGE:ResolutionFailed] DocumentMerge::resolve: auto-resolution not available for field <field>
```

#### Step 1: Identify the Conflict Pattern

```bash
# Count conflicts per minute (rate spike indicates a systematic issue)
grep '\[MERGE:ConflictDetected\]' /var/log/themisdb/themisdb.log | \
  awk '{print $1, $2}' | cut -d: -f1,2 | sort | uniq -c | tail -20

# Extract the conflicting field names
grep '\[MERGE:ConflictDetected\]' /var/log/themisdb/themisdb.log | \
  sed 's/.*field //' | sort | uniq -c | sort -rn | head -10

# Identify which document keys are generating the most conflicts
grep '\[MERGE:ConflictDetected\]' /var/log/themisdb/themisdb.log | \
  sed 's/.*key //' | awk '{print $1}' | sort | uniq -c | sort -rn | head -10
```

#### Step 2: Diagnose by Conflict Type

| Conflict Pattern | Likely Cause | Resolution |
|---|---|---|
| Single field conflicting across all documents | Concurrent writers updating same field without lock coordination | Identify and serialize the conflicting writers |
| Conflict burst after deployment | New writer introduced conflicting schema semantics | Roll back the deployment; review merge semantics |
| Conflicts on `_version` or `_timestamp` fields | Clock skew between nodes | Resync NTP; use logical clock for version fields |
| Random fields, low rate | Normal concurrent edit conflict | Surface conflict to user; request manual resolution |

#### Step 3: Drain the Conflict Queue

```bash
# View pending unresolved conflicts
themisdb-admin document-merge list-conflicts --limit 20

# Force-resolve using the "latest-writer-wins" policy (use with caution)
themisdb-admin document-merge resolve-conflicts \
  --policy latest_writer_wins \
  --key-pattern "conflicted-prefix-*" \
  --dry-run

# Apply if dry-run output looks correct
themisdb-admin document-merge resolve-conflicts \
  --policy latest_writer_wins \
  --key-pattern "conflicted-prefix-*"
```

#### Step 4: Validate Recovery

```bash
# Confirm conflict rate drops to baseline
query-metrics --metric merge_conflict_total_rate --range 30m --window 1m
# Expected: ≤ baseline conflict rate (typically < 5/min in healthy operation)

# Verify merge succeeds for a representative document
themisdb-admin document-merge test-three-way \
  --base-key "sample-doc-001" \
  --expect-no-conflict
```

**Decision Point:**
- ✅ **Conflict rate at baseline:** Normal operation restored → close incident
- ⚠ **Isolated conflicts persist:** Surface to application layer; not a store-level incident
- ❌ **Storm continues:** Identify and stop the conflicting writer process; escalate to document module team

---

### Scenario 3: Schema Migration Failure

**Symptoms:**
- Logs contain `[SCHEMA:MigrationFailed]` or `[SCHEMA:VersionViolation]` tags
- Documents written with new schema version fail validation on read by older consumers
- Dashboard: `schema_migration_error_total` rising

**Log patterns:**
```
[SCHEMA:MigrationFailed] SchemaRegistry::migrate: migration from v<N> to v<M> failed: <detail>
[SCHEMA:VersionViolation] DocumentStore::write: schema version <N> not registered: <key>
[SCHEMA:SealViolation] DocumentSchema::seal: sealed schema v<N> cannot be modified
[SCHEMA:ValidationError] DocumentSchema::validate: field <field> failed constraint: <constraint>
```

#### Step 1: Identify the Schema Migration State

```bash
# Check currently registered schema versions
themisdb-admin document-schema list-versions

# Check migration errors
grep '\[SCHEMA:MigrationFailed\]\|\[SCHEMA:VersionViolation\]' \
  /var/log/themisdb/themisdb.log | tail -30

# Identify which schema version is causing failures
grep '\[SCHEMA:VersionViolation\]' /var/log/themisdb/themisdb.log | \
  sed 's/.*version //' | awk '{print $1}' | sort | uniq -c | sort -rn
```

#### Step 2: Action by Schema Failure Type

**MigrationFailed:**
1. Identify the failed migration script via log detail
2. Roll back to the previous schema version if migration is not idempotent
3. Re-run migration with `--dry-run` flag to validate before applying

**VersionViolation:**
1. The writer is using an unregistered schema version
2. Register the missing schema version: `themisdb-admin document-schema register --version <N> --definition /path/to/schema.json`
3. Or reject the writer if the version is invalid

**SealViolation:**
1. A sealed schema is being modified — this is a programming error
2. Create a new schema version for the modifications; do not unseal

#### Step 3: Complete the Migration (if safe)

```bash
# Validate migration script against test documents
themisdb-admin document-schema migrate \
  --from-version 2 --to-version 3 \
  --dry-run --sample-keys 20

# Apply migration
themisdb-admin document-schema migrate \
  --from-version 2 --to-version 3 \
  --batch-size 500

# Verify post-migration document integrity
themisdb-admin document-store validate-schema \
  --schema-version 3 --sample-keys 50
```

**Decision Point:**
- ✅ **Migration complete, no violations:** Schema at new version; writers using correct version → close
- ❌ **Migration failing repeatedly:** Roll back to previous version; file issue with document module team

---

### Scenario 4: Round-Trip Persistence Loss

**Symptoms:**
- Logs contain `[DOCUMENT:RoundTripLoss]` or `[DOCUMENT:SnapshotMismatch]` tags
- Read after write returns stale or empty content for recently written documents
- Dashboard: `document_roundtrip_mismatch_total` rising

**Log patterns:**
```
[DOCUMENT:RoundTripLoss] RoundTripEditor::read: snapshot mismatch for key <key> (expected v<N>, got v<M>)
[DOCUMENT:SnapshotMismatch] RoundTripEditor::restore: snapshot content digest mismatch
[DOCUMENT:PersistenceError] DocumentStore::flush: WAL flush failed: <detail>
```

#### Step 1: Identify Affected Documents

```bash
# Find all round-trip loss events in the last hour
grep '\[DOCUMENT:RoundTripLoss\]\|\[DOCUMENT:SnapshotMismatch\]' \
  /var/log/themisdb/themisdb.log | \
  awk -F'key ' '{print $2}' | awk '{print $1}' | sort | uniq > affected_keys.txt

wc -l affected_keys.txt
# If > 10 keys affected, likely a systemic WAL or cache flush issue
```

#### Step 2: Check WAL Flush Health

```bash
# Check for WAL flush errors
grep '\[DOCUMENT:PersistenceError\]' /var/log/themisdb/themisdb.log | tail -20

# Verify WAL flush interval and last flush time
themisdb-admin document-store wal-status

# Force an immediate WAL flush
themisdb-admin document-store wal-flush --force
```

#### Step 3: Restore from Snapshot (if WAL flush irreversible)

```bash
# List available snapshots for affected document collection
themisdb-admin document-store list-snapshots --collection <collection_name>

# Restore from the most recent clean snapshot
themisdb-admin document-store restore-snapshot \
  --snapshot-id <snapshot_id> \
  --dry-run

# Apply if dry-run shows correct state
themisdb-admin document-store restore-snapshot \
  --snapshot-id <snapshot_id>
```

**Decision Point:**
- ✅ **Round-trip fidelity restored:** New writes round-trip correctly → close incident
- ❌ **Persistent WAL loss:** Escalate to storage infrastructure; recovery from snapshot required

---

### Scenario 5: XDOMEA Exchange Failure

**Symptoms:**
- Logs contain `[XDOMEA:ExchangeFailure]` or `[XDOMEA:ValidationError]` tags
- Structured record exchange with XDOMEA consumers fails
- Dashboard: `xdomea_exchange_error_total` rising

**Log patterns:**
```
[XDOMEA:ExchangeFailure] XdomeaConnector::exchange: record transfer failed for record_id <id>: <detail>
[XDOMEA:ValidationError] XdomeaConnector::validate: XDOMEA schema validation failed: <field>
[XDOMEA:ProtocolError] XdomeaConnector::handshake: protocol version mismatch: expected <N>, got <M>
[XDOMEA:TimeoutError] XdomeaConnector::exchange: exchange timeout after <N>ms for record_id <id>
```

#### Step 1: Identify Exchange Failure Pattern

```bash
# Check XDOMEA exchange errors
grep '\[XDOMEA:ExchangeFailure\]\|\[XDOMEA:ValidationError\]' \
  /var/log/themisdb/themisdb.log | tail -30

# Check protocol version mismatches
grep '\[XDOMEA:ProtocolError\]' /var/log/themisdb/themisdb.log | tail -10

# Identify timeout rate
grep '\[XDOMEA:TimeoutError\]' /var/log/themisdb/themisdb.log | \
  awk '{print $1, $2}' | cut -d: -f1,2 | sort | uniq -c | tail -10
```

#### Step 2: Diagnose by Failure Type

| Error Tag | Likely Cause | Resolution |
|---|---|---|
| `[XDOMEA:ExchangeFailure]` | Record content fails XDOMEA schema constraints | Validate document against XDOMEA schema before exchange |
| `[XDOMEA:ValidationError]` | Missing required field or wrong field type | Check schema field definitions; update document before re-exchange |
| `[XDOMEA:ProtocolError]` | XDOMEA protocol version mismatch with peer | Align connector protocol version with peer; check peer upgrade status |
| `[XDOMEA:TimeoutError]` | Network latency or peer not responding | Check peer health; increase `exchange_timeout_ms` if transient |

#### Step 3: Retry Failed Exchange

```bash
# List failed XDOMEA exchanges pending retry
themisdb-admin xdomea list-failed --since 1h

# Retry a specific failed record exchange
themisdb-admin xdomea retry-exchange --record-id <id>

# Bulk retry for a time window
themisdb-admin xdomea retry-exchange --since 2h --dry-run
themisdb-admin xdomea retry-exchange --since 2h
```

**Decision Point:**
- ✅ **Exchange succeeds on retry:** Transient failure; monitor for recurrence → close
- ❌ **Persistent validation failures:** Content or schema issue requires document team involvement
- ❌ **Protocol mismatch:** Coordinate peer upgrade; block exchange until versions aligned

---

## Distributed Tracing — Wave D D1 Span Cross-Links

> **Note:** Distributed tracing spans are planned in the Wave D Phase 2A tracing framework
> planned in `docs/operability/WAVE_D_ROADMAP.md` §2A. Until Phase 2A implementation completes
> (Target: Q1 2027), the listed span names are reference identifiers for future instrumentation.

### Document Store D1 Trace Spans

When the Phase 2A tracing SDK is available, the following operator actions map to trace spans:

| Module Surface | D1 Span Name | Baggage Keys | Notes |
|---|---|---|---|
| Document write | `document.store.write` | `doc_key`, `schema_version`, `content_size_bytes` | Status ERROR on failure |
| Document read | `document.store.read` | `doc_key`, `schema_version`, `cache_hit` | Includes miss path |
| Diff operation | `document.diff.compute` | `from_key`, `to_key`, `patch_size_bytes` | |
| Merge operation | `document.merge.three_way` | `base_key`, `conflict_detected`, `resolution_policy` | Status ERROR on conflict |
| Schema migration | `document.schema.migrate` | `from_version`, `to_version`, `affected_docs` | Child spans per doc |
| Round-trip restore | `document.roundtrip.restore` | `key`, `snapshot_id`, `digest_match` | |
| XDOMEA exchange | `document.xdomea.exchange` | `record_id`, `protocol_version`, `exchange_result` | Status ERROR on failure |

### Querying Document Trace Spans (Phase 2A onwards)

```bash
# Find all failed document writes in the last hour
otel-query --service document_store --operation document.store.write \
  --status ERROR --range 1h

# Find all merge operations with conflicts
otel-query --service document_store --operation document.merge.three_way \
  --baggage "conflict_detected=true" --range 24h

# Find slow schema migrations (> 5 s)
otel-query --service document_store --operation document.schema.migrate \
  --min-duration 5s --range 7d

# Cross-reference round-trip restore failures with WAL flush events
otel-query --service document_store --operation document.roundtrip.restore \
  --status ERROR --range 2h --include-baggage
```

---

## Alert Reference

| Alert Name | Threshold | Runbook Step |
|---|---|---|
| `DocumentWriteErrorRate` | > 1% of writes failing over 5 min | Scenario 1 |
| `MergeConflictStorm` | > 50 conflicts/min for 2 min | Scenario 2 |
| `SchemaMigrationFailed` | Any `[SCHEMA:MigrationFailed]` event | Scenario 3 |
| `RoundTripPersistenceLoss` | Any `[DOCUMENT:RoundTripLoss]` event | Scenario 4 |
| `XdomeaExchangeFailureRate` | > 5% of exchanges failing over 10 min | Scenario 5 |

---

## Escalation Path

1. **First responder (operator):** Follow the applicable Scenario steps above
2. **Document module team:** Escalate for schema migration failures, merge semantics bugs, or XDOMEA protocol mismatches
3. **Storage infrastructure team:** Escalate for WAL flush failures, disk full, or backend unavailability
4. **Security team:** Escalate immediately if any injection or schema-tampering pattern detected

---

**Runbook Version:** 1.0
**Last Updated:** 2026-09-16
**Owner:** Document Module Team, Operations Team
**Next Review:** 2027-03-01 (post-Wave D Phase 2A delivery)
