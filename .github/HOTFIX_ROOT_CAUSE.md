# HOTFIX Discovery Updated: RocksDB + Sharding Initialization Order

**Status:** 🔴 ROOT CAUSE REFINED  
**Date:** 2026-01-04  
**Root Cause:** `AdaptiveIndexManager` tries to use Sharding features before Sharding is initialized

---

## Detailed Root Cause

### The Problem Sequence

```
1. ✅ RocksDB opens with 2 CFs (one for Sharding)
2. ✅ All basic managers initialized (Spatial, PII, Graph, Vector, Secondary Index)
3. ✅ HTTP Server created
4. ✅ Spatial Index Manager initialized
5. 🔴 Adaptive Index Manager initialized → Creates snapshot/transaction
   └─ Tries to use RocksDB MVCC with Sharding support
   └─ Sharding Manager NOT YET INITIALIZED
   └─ Hangs waiting for Shard Discovery / Cluster Coordination
   └─ Blocks rest of initialization
6. ❌ (never reached) getOrCreateColumnFamily("prompt_templates")
7. ❌ (never reached) PKIKeyProvider initialization
8. ❌ (never reached) Auth, Security components
9. ❌ (never reached) "READY FOR OPERATIONS"
```

### Key Insight

**RocksDB TransactionDB with 2 Column Families AND Sharding enabled:**
- The second column family is used for **cross-shard coordination data**
- `AdaptiveIndexManager` tries to create snapshots across all CFs
- But Sharding Manager (which handles inter-shard communication) is NOT initialized yet
- → Deadlock waiting for cluster bootstrap that hasn't happened

### Location: src/server/http_server.cpp

```cpp
Line 275: prompt_cf_handle_ = storage_->getOrCreateColumnFamily("prompt_templates");
Line 303: adaptive_index_ = std::make_unique<AdaptiveIndexManager>(storage_->getRawDB());
Line 304: THEMIS_INFO("Adaptive Index Manager initialized");
          ^ THIS LINE PRINTS, but next line hangs
```

The `AdaptiveIndexManager` constructor calls `storage_->getRawDB()` which is the raw TransactionDB pointer. When sharding is enabled, this tries to establish cluster snapshot consistency, which fails because:

1. Sharding Manager not initialized
2. No response from other shards (network calls pending)
3. MVCC transaction blocks waiting for inter-shard coordination

---

## Solution Approaches

### Option 1: Lazy Initialize Sharding Manager EARLIER

Move Sharding Manager initialization **before** `AdaptiveIndexManager`:

```cpp
// In http_server.cpp constructor, reorder:

// 1. Initialize Sharding Manager FIRST (if sharding enabled)
if (/* sharding enabled */) {
    sharding_manager_ = std::make_unique<ShardingManager>(...);
    THEMIS_INFO("Sharding Manager initialized");
}

// 2. THEN initialize AdaptiveIndexManager (which may use sharding)
adaptive_index_ = std::make_unique<AdaptiveIndexManager>(storage_->getRawDB());
THEMIS_INFO("Adaptive Index Manager initialized");
```

### Option 2: Pass Sharding Context to AdaptiveIndexManager

Instead of `storage_->getRawDB()`, pass a sharding-aware context:

```cpp
// Create sharding context first
auto shard_context = std::make_unique<ShardingContext>(...);

// Then pass to AdaptiveIndexManager so it knows not to wait for other shards
adaptive_index_ = std::make_unique<AdaptiveIndexManager>(
    storage_->getRawDB(), 
    shard_context
);
```

### Option 3: Disable Sharding-Aware MVCC for Single-Node Shards

Detect if this is a single shard (bootstrap node with THEMIS_BOOTSTRAP_SHARD == self) and disable cross-shard MVCC coordination:

```cpp
bool is_cluster_bootstrap = /* check THEMIS_SHARDS contains all shards */;
bool needs_cluster_coordination = is_cluster_bootstrap && (num_shards > 1);

// Only enable cluster MVCC if we have multiple shards to coordinate
adaptive_index_ = std::make_unique<AdaptiveIndexManager>(
    storage_->getRawDB(),
    needs_cluster_coordination  // false = no cross-shard waits
);
```

---

## Implementation Priority

1. **Option 3 (QUICKEST):** Just check shard count and disable cross-shard MVCC for single shards
2. **Option 1 (SAFE):** Reorder initialization to initialize Sharding Manager earlier
3. **Option 2 (BEST):** Refactor to pass explicit Sharding context

---

## Files to Modify

- **src/server/http_server.cpp** - Constructor/initialization order
- **include/indexes/adaptive_index_manager.h** - Add sharding context parameter (optional)
- **src/indexes/adaptive_index_manager.cpp** - Check sharding context before cluster ops

---

## Testing

```bash
# After fix, should see:
docker logs themis-raid0-shard1 | grep -E "Adaptive Index Manager|PKIKeyProvider|NOW READY"

# Expected:
# Adaptive Index Manager initialized
# PKIKeyProvider initialized with persistent KEK/DEK  
# 🎉 ThemisDB 1.3.4 is now READY FOR OPERATIONS
```

---

**Priority:** 🔴 CRITICAL  
**Complexity:** Low (reordering) to Medium (refactoring)  
**Time to Fix:** 30 mins to 2 hours depending on approach chosen
