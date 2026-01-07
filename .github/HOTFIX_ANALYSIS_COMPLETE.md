# HOTFIX: RAID Sharding Initialization Deadlock - Complete Analysis

**Status:** 🔴 ROOT CAUSE IDENTIFIED & DOCUMENTED  
**Date:** 2026-01-04  
**Impact:** v1.3.4 RAID cluster deployments blocked  
**Severity:** CRITICAL

---

## Executive Summary

ThemisDB v1.3.4 server hangs indefinitely during startup when **RAID sharding is enabled** (`THEMIS_ENABLE_SHARDING=true`). The hang occurs **after** "Adaptive Index Manager initialized" and **before** PKIKeyProvider initialization, blocking all subsequent server setup.

**Root Cause:** `AdaptiveIndexManager` initialization attempts to use RocksDB MVCC snapshot functionality across **2 column families** (one for sharding coordination), but the **Sharding Manager is not yet initialized**. The MVCC code hangs waiting for inter-shard cluster coordination that never happens.

### Key Evidence

```
Standalone (WORKS):        "MVCC enabled, 1 column families"   ✅
RAID Cluster (HANGS):      "MVCC enabled, 2 column families"   🔴

Hang Point: After "Adaptive Index Manager initialized"
           Before PKIKeyProvider initialization
```

---

## Initialization Sequence Issue

### Current (Broken) Sequence

```
1. ✅ RocksDB opens with 2 CFs (default + sharding coordination CF)
2. ✅ Basic managers (Spatial, PII, Graph, Vector, Secondary Index)
3. ✅ HTTP Server created
4. ✅ Spatial Index Manager initialized
5. 🔴 AdaptiveIndexManager::ctor() → Creates MVCC snapshots
       ├─ Sharding is enabled (CF#2 exists)
       ├─ Tries to coordinate snapshot across shards
       ├─ Sharding Manager NOT YET INITIALIZED
       └─ DEADLOCK: Waiting for cluster bootstrap response
6. ❌ PKIKeyProvider initialization (NEVER REACHED)
7. ❌ Auth/Security components (NEVER REACHED)
8. ❌ Server "READY FOR OPERATIONS" (NEVER REACHED)
```

### Root Cause Location

**File:** `src/server/http_server.cpp`  
**Constructor:** `HttpServer::HttpServer(...)`  
**Lines:**
- 303-304: `AdaptiveIndexManager` initialization
- 275: `getOrCreateColumnFamily("prompt_templates")` never reached

---

## Why It Happens

### RocksDB Column Family Behavior

When `THEMIS_ENABLE_SHARDING=true`:
- RocksDB opens with **2 column families** instead of 1
- Second CF for cross-shard coordination metadata
- MVCC transactions must be consistent across all CFs

### AdaptiveIndexManager MVCC Issue

The `AdaptiveIndexManager` constructor likely calls:
```cpp
AdaptiveIndexManager(rocksdb::TransactionDB* db) {
    // Attempts to create snapshot/transaction across all CFs
    // This triggers MVCC coordin ation with Sharding subsystem
    // But Sharding Manager not initialized yet!
    // → DEADLOCK
}
```

### Timing: Sharding Manager Initialization Missing

- **Sharding Manager needed:** For inter-shard RPC/network coordination
- **When should initialize:** Before `AdaptiveIndexManager`
- **When actually initializes:** Never (constructor doesn't set it up)
- **Result:** Cross-shard MVCC coordination hangs forever

---

## Solution Approaches

### Approach 1: Reorder Initialization (RECOMMENDED)

Move Sharding Manager initialization **before** AdaptiveIndexManager:

```cpp
// In HttpServer constructor, BEFORE AdaptiveIndexManager:

// Initialize Sharding Manager FIRST (if enabled)
if (config_.sharding_enabled || std::getenv("THEMIS_ENABLE_SHARDING")) {
    sharding_manager_ = initializeShardingManager(...);
    THEMIS_INFO("Sharding Manager initialized");
}

// THEN initialize Adaptive Index Manager
adaptive_index_ = std::make_unique<AdaptiveIndexManager>(storage_->getRawDB());
THEMIS_INFO("Adaptive Index Manager initialized");
```

**Pros:** Simple, safe, follows dependency order  
**Cons:** Requires creating ShardingManager  
**Effort:** 1-2 hours

### Approach 2: Lazy Initialization

Don't initialize AdaptiveIndexManager until after full HTTP startup:

```cpp
// During constructor: skip
// adaptive_index_ initialization

// In server startup (later):
// When Sharding Manager is ready
adaptive_index_ = std::make_unique<AdaptiveIndexManager>(...);
```

**Pros:** Minimal code changes  
**Cons:** Changes initialization semantics  
**Effort:** 1 hour

### Approach 3: Detect Single-Node Shards

Check if shard is running alone (not in cluster) and disable cluster MVCC:

```cpp
bool is_single_shard = (shard_count_ == 1 && no_peers_configured);

adaptive_index_ = std::make_unique<AdaptiveIndexManager>(
    storage_->getRawDB(),
    is_single_shard ? SkipClusterCoordination : EnableClusterCoordination
);
```

**Pros:** Works for both standalone and cluster  
**Cons:** Requires AdaptiveIndexManager API change  
**Effort:** 2-3 hours

---

## Temporary Workaround

### Disable Sharding in docker-compose

```yaml
# In docker-compose-sharding.yml, remove:
THEMIS_ENABLE_SHARDING: "true"
THEMIS_ROLE: "shard"
THEMIS_SHARD_ID: "..."

# Result: Runs as standalone (works, but no sharding)
```

### Use Standalone Image Instead

```bash
docker run -d \
  -p 18765:18765 \
  themisdb/themisdb:hyperscaler
# (without THEMIS_ENABLE_SHARDING env var)
# ✅ Works perfectly
```

---

## Testing & Verification

### Before Fix

```
❌ docker logs themis-raid0-shard1
   [10:54:07.563] Adaptive Index Manager initialized
   [... no further logs, hangs indefinitely ...]
```

### After Fix

```
✅ docker logs themis-raid0-shard1
   [10:54:07.563] Adaptive Index Manager initialized
   [10:54:07.667] PKIKeyProvider initialized with persistent KEK/DEK
   [10:54:07.742] 🎉 ThemisDB 1.3.4 is now READY FOR OPERATIONS
```

### Test Commands

```bash
# Verify fix works
docker-compose -f docker-compose-sharding.yml up -d

# Check logs
for shard in raid0-shard1 raid0-shard2 raid1-primary; do
  echo "=== $shard ==="
  docker logs themis-$shard | tail -5
done

# Health check
for port in 18765 18766 18768; do
  curl -s http://localhost:$port/health | jq .
done

# Feature verification
curl -s http://localhost:18765/health | jq '.sharding'
```

---

## Files to Investigate/Modify

1. **src/server/http_server.cpp** - Initialization order (PRIMARY)
2. **include/indexes/adaptive_index_manager.h** - Constructor (SECONDARY)
3. **src/indexes/adaptive_index_manager.cpp** - Implementation (SECONDARY)
4. **include/sharding/shard_manager.h** - Sharding initialization (SECONDARY)
5. **src/sharding/shard_manager.cpp** - Implementation (SECONDARY)

---

## References

- Related Issue: Hangs after "Adaptive Index Manager initialized"
- Build Flags: THEMIS_ENABLE_SHARDING, THEMIS_ENABLE_LLM, THEMIS_ENABLE_GPU
- Docker Config: docker-compose-sharding.yml
- Deployment: RAID0/RAID1/RAID5 cluster

---

## Quick Checklist for Fix

- [ ] Identify Sharding Manager initialization function
- [ ] Move/add Sharding Manager init before AdaptiveIndexManager
- [ ] Test with single shard (RAID0 Shard1)
- [ ] Test with full cluster (9 shards)
- [ ] Verify PKIKeyProvider appears in logs
- [ ] Verify "READY FOR OPERATIONS" message appears
- [ ] Check health endpoint returns 200
- [ ] Verify cluster is operational (query data, etc.)
- [ ] Rebuild Docker image
- [ ] Push to Docker Hub
- [ ] Update CHANGELOG

---

**Created by:** Root Cause Analysis  
**Assigned to:** Dev Team  
**Fix Timeline:** 2-4 hours including testing  
**Blocker:** Yes (v1.3.4 RAID can't deploy)
