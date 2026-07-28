# Chimera Troubleshooting Guide

The `chimera` module provides database adapter interoperability for ThemisDB, enabling transparent compatibility with MongoDB wire protocol and ThemisDB native protocol via an adapter factory pattern.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `MongoAdapter: command not supported` | MongoDB command not implemented | Check supported command list |
| `AdapterFactory: no adapter for protocol` | Protocol not configured | Set `chimera.protocol` |
| MongoDB client cannot connect | Wrong port or TLS setting | Check `chimera.mongodb.port` |
| Translation produces wrong query | Mongo query uses unsupported operator | Check operator mapping table |
| `ThemisdbAdapter: auth failure` | Native auth not propagated | Pass credentials through adapter |
| Chimera exporter connection refused | Exporter not started | Check chimera exporter service |
| BSON deserialization error | Binary BSON data not handled | Enable `chimera.bson.strict: false` |
| Slow MongoDB compatibility queries | N+1 query from translation | Batch lookups; use native API |

## Common Issues

### Issue 1: MongoDB Command Not Supported

**Description:** A MongoDB client sends a command that the Chimera adapter does not implement.

**Symptoms:**
- Error: `MongoAdapter: command 'mapReduce' not supported`
- Legacy MongoDB application fails

**Cause:** `mapReduce` and other legacy MongoDB commands are not implemented.

**Solution:**
```bash
# List supported MongoDB commands
themisdb-admin chimera supported-commands

# Check unsupported command usage in your application
themisdb-admin chimera command-stats --last 24h | grep UNSUPPORTED
```
```yaml
chimera:
  mongodb:
    unsupported_command_action: error   # "error" | "warn" | "ignore"
    warn_on_translation: true
```
*Migrate `mapReduce` to AQL aggregation queries for full support.*

---

### Issue 2: MongoDB Client Connection Refused

**Description:** MongoDB client cannot connect to the Chimera compatibility port.

**Symptoms:**
- `mongosh mongodb://localhost:27017` returns `connection refused`
- Chimera MongoDB endpoint not listening

**Cause:** Chimera MongoDB compatibility is disabled or wrong port.

**Solution:**
```yaml
chimera:
  enabled: true
  protocol: mongodb               # "mongodb" | "themisdb" | "both"
  mongodb:
    port: 27017
    bind_address: 0.0.0.0
    tls:
      enabled: false
```
```bash
# Check if port is listening
ss -tlnp | grep 27017

# Test connection
mongosh --host localhost --port 27017 --eval "db.runCommand({ping: 1})"
```

---

### Issue 3: Query Translation Produces Wrong Results

**Description:** MongoDB queries translated to AQL return different results than expected.

**Symptoms:**
- `db.users.find({age: {$gte: 18, $lte: 65}})` returns unexpected documents
- Range query boundary handling differs

**Cause:** Edge cases in MongoDB-to-AQL query translation.

**Solution:**
```yaml
chimera:
  translation:
    debug_mode: true              # log original and translated query
    strict_compatibility: true    # fail on ambiguous translations
    null_handling: mongodb        # "mongodb" | "sql" | "native"
```
```bash
# Debug translation for a specific query
themisdb-admin chimera translate \
  --protocol mongodb \
  --query '{"age": {"$gte": 18, "$lte": 65}}'
```

---

### Issue 4: BSON Deserialization Error

**Description:** Binary BSON documents from MongoDB clients cannot be parsed.

**Symptoms:**
- Log: `MongoAdapter: BSON parse error: unknown type 0x05 at offset 42`
- Documents with binary fields fail to insert

**Cause:** Binary BSON subtypes not handled.

**Solution:**
```yaml
chimera:
  bson:
    strict: false                 # lenient BSON parsing
    unsupported_type_action: null # replace unknown types with null
    max_document_size_mb: 16
```

---

### Issue 5: Authentication Not Propagated

**Description:** MongoDB clients are authenticated but the underlying ThemisDB query has no user context.

**Symptoms:**
- RLS policies not applied for MongoDB connections
- Audit log shows `user=anonymous` for authenticated MongoDB requests

**Cause:** Chimera adapter not propagating MongoDB auth credentials to ThemisDB auth.

**Solution:**
```yaml
chimera:
  auth:
    propagate_credentials: true
    mechanism: scram_sha256        # "scram_sha256" | "x509" | "plain"
    map_to_themisdb_user: true
```

## Diagnostic Commands

```bash
# Chimera status
themisdb-admin chimera status

# Supported MongoDB commands
themisdb-admin chimera supported-commands

# Query translation test
themisdb-admin chimera translate \
  --protocol mongodb \
  --query '{"status": "active"}'

# Connection stats
themisdb-admin chimera connection-stats

# Tail chimera logs
journalctl -u themisdb -f | grep -E "chimera|mongo|adapter|bson|translation"
```

## Configuration Reference

```yaml
chimera:
  enabled: false
  protocol: mongodb
  mongodb:
    port: 27017
    bind_address: 127.0.0.1
    tls:
      enabled: false
  translation:
    debug_mode: false
    strict_compatibility: false
  bson:
    strict: true
    max_document_size_mb: 16
```

## Known Limitations

- `mapReduce`, `$where`, and server-side JavaScript are not supported.
- MongoDB transactions with multi-document atomicity use ThemisDB's MVCC; exact MongoDB semantics may differ.
- BSON `Decimal128` type is mapped to ThemisDB `double`; precision loss may occur for very large decimals.
- MongoDB `changeStream` is mapped to ThemisDB CDC; resume tokens are not interchangeable.

## Related Documentation

- [Chimera Module ROADMAP](../../src/chimera/ROADMAP.md)
- [CDC Troubleshooting](./cdc_troubleshooting.md)
- [Wire Protocol](../wire-protocol.md)
- [API Versioning](../api/API_VERSIONING.md)
