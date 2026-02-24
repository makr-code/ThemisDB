# Importers Troubleshooting Guide

The `importers` module enables migration of data into ThemisDB from external database systems, currently supporting MongoDB, MySQL, and PostgreSQL as source databases.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `MongoImporter: connection refused` | MongoDB source not reachable | Check `importers.mongodb.host` and network |
| `MysqlImporter: access denied` | Wrong credentials | Verify `importers.mysql.user` and password |
| `PostgresImporter: SSL required` | Source requires TLS | Set `importers.postgres.ssl_mode: require` |
| Import produces duplicate keys | Target collection already has data | Truncate target or use `on_conflict: update` |
| `MongoImporter: BSON decode error` | Unsupported BSON type | Enable `importers.mongodb.bson_lenient: true` |
| Import is very slow | No batching or single thread | Increase `importers.batch_size` |
| Type mismatch in imported data | Source has inconsistent types | Enable `importers.type_coercion: true` |
| Import stops at 50% | Memory pressure | Reduce batch size; enable streaming |
| `MysqlImporter: max packet exceeded` | Large row in MySQL | Increase `max_allowed_packet` in MySQL |
| NULL values not imported | NULL handling mismatch | Set `importers.null_handling: preserve` |

## Common Issues

### Issue 1: MongoDB Source Connection Failure

**Description:** MongoDB importer cannot connect to the source database.

**Symptoms:**
- Log: `MongoImporter: connection failed: connect ECONNREFUSED mongodb:27017`
- Import does not start

**Cause:** Source MongoDB unreachable; wrong host/port; authentication required.

**Solution:**
```yaml
importers:
  mongodb:
    host: mongodb.prod.internal
    port: 27017
    database: mydb
    username: ${MONGO_USER}
    password: ${MONGO_PASS}
    auth_mechanism: SCRAM-SHA-256
    tls:
      enabled: true
      ca_file: /etc/themisdb/importers/mongo-ca.crt
    connection_timeout_ms: 10000
```
```bash
# Test connectivity
mongosh "mongodb://user:pass@mongodb:27017/mydb" --eval "db.runCommand({ping:1})"
```

---

### Issue 2: PostgreSQL Import Slow Due to No Batching

**Description:** PostgreSQL import inserts rows one by one, making it extremely slow.

**Symptoms:**
- Import rate: 100 rows/sec for a 10M row table
- Log: `PostgresImporter: batch_size=1`

**Cause:** Batch size not configured; default is 1.

**Solution:**
```yaml
importers:
  batch_size: 5000               # insert 5000 rows per batch
  postgres:
    host: postgres.prod.internal
    port: 5432
    database: mydb
    username: ${PG_USER}
    password: ${PG_PASS}
    ssl_mode: require
    fetch_size: 10000            # fetch 10000 rows per round-trip
    parallel_tables: 4           # import 4 tables in parallel
```

---

### Issue 3: Duplicate Key Conflicts on Import

**Description:** Import fails because target collection already contains documents with the same keys.

**Symptoms:**
- Log: `PostgresImporter: duplicate key violation for _key=user-42`
- Import aborts on first conflict

**Cause:** Target collection already has data from a previous partial import.

**Solution:**
```yaml
importers:
  on_conflict: update             # "error" | "skip" | "update" | "replace"
  conflict_key: _key              # field to use for conflict detection
  truncate_before_import: false   # set true to clear target before import
```
```bash
# Truncate target collection before re-import
themisdb-admin collection truncate --collection users

# Or use upsert mode
themisdb-admin importers run \
  --source postgres \
  --table users \
  --target users \
  --on-conflict update
```

---

### Issue 4: MySQL Large Row Packet Error

**Description:** MySQL import fails for tables with large TEXT or BLOB fields.

**Symptoms:**
- Log: `MysqlImporter: packet too large: max_allowed_packet exceeded`
- Import fails on rows with large fields

**Cause:** MySQL `max_allowed_packet` too small for large columns.

**Solution:**
```bash
# Increase max_allowed_packet in MySQL
mysql -u root -p -e "SET GLOBAL max_allowed_packet=67108864;"  # 64MB

# Or set in my.cnf
echo "max_allowed_packet = 64M" >> /etc/mysql/conf.d/themisdb.cnf
systemctl restart mysql
```
```yaml
importers:
  mysql:
    max_allowed_packet_mb: 64
    fetch_size: 1000             # reduce if rows are large
```

---

### Issue 5: Type Coercion Failures

**Description:** Import fails because source data types don't match ThemisDB schema.

**Symptoms:**
- Log: `MongoImporter: type mismatch: field 'age' is string in source, expected number`
- Import aborts on type error

**Cause:** Inconsistent types in source database (common in MongoDB).

**Solution:**
```yaml
importers:
  type_coercion: true             # automatically coerce types
  coercion_rules:
    string_to_number: true
    null_to_zero: false
    boolean_from_int: true
  on_type_error: skip             # "abort" | "skip" | "coerce"
```

---

### Issue 6: Import Memory Exhaustion

**Description:** Import process runs out of memory for very large tables.

**Symptoms:**
- Process OOM-killed at 50% completion
- Log: `PostgresImporter: fetching all rows into memory`

**Cause:** Entire result set loaded into memory; streaming not enabled.

**Solution:**
```yaml
importers:
  streaming: true                 # stream rows instead of loading all
  batch_size: 1000                # process 1000 rows at a time
  max_memory_mb: 512              # cap memory usage
  checkpoint:
    enabled: true
    interval: 10000               # checkpoint every 10000 rows
    path: /var/lib/themisdb/import-checkpoints/
```

## Diagnostic Commands

```bash
# List import jobs
themisdb-admin importers jobs list

# Start an import
themisdb-admin importers run \
  --source postgres \
  --host localhost \
  --database mydb \
  --table users \
  --target users

# Check import progress
themisdb-admin importers status --job-id imp-abc123

# Test source connectivity
themisdb-admin importers test-connection \
  --source postgres \
  --host localhost \
  --database mydb

# Tail importer logs
journalctl -u themisdb -f | grep -E "importer|mongo.import|mysql.import|postgres.import"
```

## Configuration Reference

```yaml
importers:
  enabled: true
  batch_size: 1000
  on_conflict: error
  type_coercion: false
  streaming: true
  mongodb:
    host: localhost
    port: 27017
  mysql:
    host: localhost
    port: 3306
  postgres:
    host: localhost
    port: 5432
    ssl_mode: require
  checkpoint:
    enabled: true
    interval: 10000
```

## Known Limitations

- MongoDB importer does not support change stream-based continuous sync; it is a one-time snapshot import.
- MySQL importer does not support stored procedure results as import source.
- PostgreSQL importer cannot import partitioned tables in a single job; each partition must be imported separately.
- Import performance is limited by the source database's read throughput; use read replicas for large imports.

## Related Documentation

- [Importers Module ROADMAP](../../src/importers/ROADMAP.md)
- [Exporters Troubleshooting](./exporters_troubleshooting.md)
- [CDC Troubleshooting](./cdc_troubleshooting.md)
- [Storage Troubleshooting](./storage_troubleshooting.md)
