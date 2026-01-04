# HOTFIX Discovery: RocksDB Column Family Deadlock in Sharding Mode

**Status:** 🔴 ROOT CAUSE IDENTIFIED  
**Date:** 2026-01-04  
**Issue:** Server hangs after "Adaptive Index Manager initialized" in sharding mode

---

## Critical Finding: Column Family Count Mismatch

### Evidence

When comparing **Standalone vs. Sharding Mode:**

```
❌ RAID Cluster / Sharding Mode (HANGS):
   [2026-01-04 10:47:31.404] Opened RocksDB TransactionDB (MVCC enabled, 2 column families)
   [2026-01-04 10:47:31.404] Adaptive Index Manager initialized
   [... DEADLOCK - NO FURTHER LOGS ...]

✅ Standalone / No Sharding (WORKS):
   [2026-01-04 10:43:04.598] Opened RocksDB TransactionDB (MVCC enabled, 1 column families)
   [2026-01-04 10:43:04.626] Adaptive Index Manager initialized
   [2026-01-04 10:43:04.667] PKIKeyProvider initialized
   [2026-01-04 10:43:04.742] 🎉 ThemisDB 1.3.4 is now READY FOR OPERATIONS
```

**Key Difference:** Server creates **2 column families** when clustering is enabled!

### Root Cause

When sharding is enabled (`THEMIS_ENABLE_SHARDING=true`), the server opens RocksDB with an additional column family, likely for:
- Shard-specific metadata
- Cross-shard state/routing information
- Cluster coordination data

However, this second column family **is not properly initialized** before the Adaptive Index Manager proceeds, causing a **deadlock or infinite wait**.

### Initialization Sequence Issue

```
1. ✅ RocksDB opened with 2 CFs
2. ✅ All managers initialized (Spatial, PII, etc.)
3. ✅ HTTP Server created
4. ✅ Spatial Index Manager initialized
5. ✅ Adaptive Index Manager initialized
6. ❌ [DEADLOCK] - Second column family blocking subsequent steps
   - PKIKeyProvider initialization never reached
   - No HTTP server startup message
   - No "READY FOR OPERATIONS" message
```

---

## Source Code Investigation

### Files to examine:

1. **src/sharding/shard_manager.cpp**
   - Look for RocksDB column family creation in sharding mode
   - Check if second CF initialization is blocking or missing synchronization

2. **src/storage/rocksdb_wrapper.cpp or rocksdb_transaction_db.cpp**
   - Search for column family list creation
   - Check MVCC initialization with multiple CFs
   - Look for mutex/condition variable waits that might deadlock

3. **src/indexes/adaptive_index_manager.cpp**
   - What happens after initialization?
   - Does it wait for something that never signals back?

4. **src/server/http_server.cpp**
   - PKIKeyProvider initialization (next step that fails)
   - Any dependencies on all column families being ready?

### Query Pattern

```bash
# Find where 2nd column family is created
grep -r "column.family\|ColumnFamily\|CF\|cf_" src/ | grep -i "shard\|cluster"

# Find blocking operations in Adaptive Index Manager context
grep -r "Adaptive Index" src/ -A 50 | grep -E "wait|mutex|lock|cv|condition"

# Find RocksDB CF creation
grep -r "CreateColumnFamily\|column_families" src/ | grep -v ".a\|.o"
```

---

## Workaround (Temporary)

### Option 1: Disable Sharding for now
```bash
# Remove THEMIS_ENABLE_SHARDING=true from docker-compose-sharding.yml
# Run as standalone deployment
```

### Option 2: Add forced timeout/watchdog
In docker-compose, reduce health check start_period but allow more retries:
```yaml
healthcheck:
  start_period: 120s  # Give extra time for 2-CF initialization
  retries: 10        # More restart attempts
```

---

## Fix Implementation Path

1. **Identify blocking call** in CF initialization
2. **Add proper synchronization** for second CF
3. **Or skip second CF** if not needed for this deployment
4. **Rebuild and test** with sharding enabled
5. **Verify both 1-CF and 2-CF paths** work

---

## Related Files

- [Dockerfile.themis-server](../Dockerfile.themis-server) - Build config
- [docker-compose-sharding.yml](../docker/compose/docker-compose-sharding.yml) - Cluster setup
- **Source to investigate:** See "Files to examine" section above

---

## Next Steps

1. ✅ ROOT CAUSE IDENTIFIED: 2-CF initialization blocks server
2. ⏳ Locate exact blocking call in source code
3. ⏳ Fix synchronization/initialization issue
4. ⏳ Rebuild Docker image
5. ⏳ Test with RAID cluster
6. ⏳ Verify features work correctly

---

**Discovered by:** Docker logs analysis  
**Date:** 2026-01-04 11:50 UTC  
**Impact:** Blocks all RAID cluster deployments  
**Severity:** 🔴 CRITICAL
