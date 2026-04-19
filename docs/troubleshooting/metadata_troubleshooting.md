# Metadata Troubleshooting Guide

The `metadata` module manages ThemisDB's schema layer, including schema definition, constraints, versioning, consistency checking, auditing of schema changes, index recommendations, and information schema queries.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `SchemaManager: collection not found` | Collection does not exist | Create collection first |
| `SchemaConstraints: violation` | Data breaks defined constraint | Fix data or relax constraint |
| Schema version mismatch | Rolling upgrade in progress | Wait for upgrade to complete |
| `SchemaConsistencyChecker: inconsistency detected` | Schema replicas differ | Run `themisdb-admin metadata repair` |
| `InformationSchema: query timeout` | Large number of collections | Increase `metadata.info_schema.timeout_ms` |
| Index recommendation never fires | Not enough query statistics | Wait for more queries; lower threshold |
| `SchemaAuditLog: write failed` | Audit log storage full | Rotate audit log |
| Statistics collector stale | Auto-analyze disabled | Enable `metadata.auto_analyze: true` |
| Schema migration blocks writes | Migration holds schema lock | Check migration status; reduce lock time |
| `SchemaVersionManager: version too old` | Client sends old schema version | Update client library |

## Common Issues

### Issue 1: Schema Constraint Violation

**Description:** Insert fails because data violates a defined schema constraint.

**Symptoms:**
- Error: `SchemaConstraints: field 'email' must match pattern '^[^@]+@[^@]+\.[^@]+$'`
- Insert API returns `422 Unprocessable Entity`

**Cause:** Application is inserting data that violates the email format constraint.

**Solution:**
```bash
# List constraints on collection
themisdb-admin metadata constraints list --collection users

# Show constraint details
themisdb-admin metadata constraints show --collection users --constraint email_format

# Relax constraint if needed
themisdb-admin metadata constraints drop --collection users --constraint email_format
```
```yaml
metadata:
  constraints:
    users:
      email:
        type: pattern
        pattern: "^[^@]+@[^@]+\\.[^@]+$"
        required: false           # set true to enforce
```

---

### Issue 2: Schema Inconsistency Across Replicas

**Description:** Schema consistency checker reports that replicas have different schema versions.

**Symptoms:**
- Log: `SchemaConsistencyChecker: replica-02 has schema_version=5, primary has version=6`
- Queries on replica return unexpected results

**Cause:** Schema change applied on primary but not yet propagated to replica.

**Solution:**
```bash
# Check schema consistency
themisdb-admin metadata consistency-check

# Force schema sync
themisdb-admin metadata repair --replica replica-02

# Check schema version on all nodes
themisdb-admin metadata schema-version --all-nodes
```

---

### Issue 3: Information Schema Query Times Out

**Description:** Querying the information schema is slow or times out.

**Symptoms:**
- `SELECT * FROM information_schema.collections` takes > 30s
- Log: `InformationSchema: query timeout`

**Cause:** Large number of collections; statistics not cached.

**Solution:**
```yaml
metadata:
  info_schema:
    timeout_ms: 120000            # increase from 30000
    cache_ttl_ms: 60000           # cache information schema results
    async_stats: true             # collect stats asynchronously
```

---

### Issue 4: Statistics Collector Returns Stale Stats

**Description:** Query optimizer uses stale statistics and produces poor plans.

**Symptoms:**
- EXPLAIN shows wrong row count estimates
- Log: `StatisticsCollector: stats for 'orders' are 7 days old`

**Cause:** Auto-analyze disabled; statistics not updated after bulk inserts.

**Solution:**
```yaml
metadata:
  auto_analyze: true
  analyze:
    trigger_rows_changed_pct: 10  # re-analyze when 10% of rows change
    schedule: "0 3 * * *"         # nightly at 3am
    sample_rate: 0.05             # 5% sample for large collections
    max_analyze_time_ms: 300000
```
```bash
# Manually run ANALYZE
themisdb-admin metadata analyze --collection orders
```

---

### Issue 5: Index Recommendation Not Triggering

**Description:** The index recommender is configured but never suggests new indices.

**Symptoms:**
- No index recommendations after many slow queries
- Log: `IndexRecommender: insufficient query stats (n=5 < min=100)`

**Cause:** Not enough query statistics accumulated; threshold too high.

**Solution:**
```yaml
metadata:
  index_recommender:
    enabled: true
    min_query_samples: 20         # reduce from 100
    recommendation_threshold_ms: 100   # recommend index for queries > 100ms
    max_recommendations: 5
    check_interval_ms: 3600000    # check hourly
```
```bash
# Check current recommendations
themisdb-admin metadata index-recommendations

# Apply a recommendation
themisdb-admin metadata apply-recommendation --id rec-001
```

---

### Issue 6: Schema Audit Log Full

**Description:** Schema change audit log disk usage is excessive.

**Symptoms:**
- Log: `SchemaAuditLog: write failed: ENOSPC`
- Schema changes not audited

**Cause:** No rotation configured for schema audit log.

**Solution:**
```yaml
metadata:
  audit:
    enabled: true
    path: /var/lib/themisdb/audit/schema/
    rotation:
      enabled: true
      max_size_mb: 200
      retention_days: 365
      compress: true
```

---

### Issue 7: Schema Version Manager Rejects Old Client

**Description:** A client with an older schema version is rejected.

**Symptoms:**
- Error: `SchemaVersionManager: client schema_version=3 is too old (min=5)`
- Specific client cannot connect

**Cause:** Minimum supported schema version raised after migration.

**Solution:**
```yaml
metadata:
  schema_versioning:
    min_client_version: 3         # lower min version to support older clients
    current_version: 6
    backward_compat_versions: 2   # support 2 versions back
```

## Diagnostic Commands

```bash
# Schema for collection
themisdb-admin metadata schema show --collection users

# Constraints
themisdb-admin metadata constraints list --collection users

# Schema version
themisdb-admin metadata schema-version

# Statistics
themisdb-admin metadata stats show --collection users

# Index recommendations
themisdb-admin metadata index-recommendations

# Live metadata metrics
curl -s http://localhost:9100/metrics | grep themisdb_metadata

# Tail metadata logs
journalctl -u themisdb -f | grep -E "metadata|schema|constraint|statistics|index.recommend"
```

## Configuration Reference

```yaml
metadata:
  auto_analyze: true
  analyze:
    trigger_rows_changed_pct: 10
    schedule: "0 3 * * *"
  index_recommender:
    enabled: true
    min_query_samples: 50
  schema_versioning:
    min_client_version: 1
    backward_compat_versions: 3
  audit:
    enabled: true
    rotation:
      enabled: true
      retention_days: 365
  info_schema:
    cache_ttl_ms: 60000
    timeout_ms: 60000
```

## Known Limitations

- Schema consistency checker requires network access to all replica nodes; partitioned clusters may show false inconsistencies.
- Index recommendations are based on query statistics only; they do not consider write amplification costs.
- Statistics sampling (`sample_rate < 1.0`) may produce inaccurate estimates for skewed distributions.
- Schema audit log does not record DDL changes made via direct RocksDB manipulation (only API-level changes).

## Related Documentation

- [Metadata Module ROADMAP](../../src/metadata/ROADMAP.md)
- [Metadata Roadmap](../metadata_roadmap.md)
- [Automatic Full-Text Index](../de/aql/AUTOMATIC_FULLTEXT_INDEX.md)
- [Schema Validation Complete](../SCHEMA_VALIDATION_COMPLETE.md)
- [Information Schema Roadmap](../INFORMATION_SCHEMA_ROADMAP.md)
