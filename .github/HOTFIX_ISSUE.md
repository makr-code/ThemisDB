# HOTFIX: Server Startup Deadlock - Adaptive Index Manager

**Status:** 🔴 CRITICAL - Server hangs at initialization  
**Date:** 2026-01-04  
**Version:** v1.3.4  
**Environment:** Docker (Ubuntu 24.04), RAID Clustering configuration

---

## Problem Description

ThemisDB server hangs indefinitely after "Adaptive Index Manager initialized" during startup when RAID clustering with multiple shards is configured. The startup process never proceeds to REST API initialization or health check response.

**NOTE:** This is specifically when RAID clustering is enabled. Standalone servers (no sharding) work correctly.

### Symptoms

```
[2026-01-04 10:39:37.561] [themis] [info] [thread 1] Adaptive Index Manager initialized
[... NO MORE LOGS - DEADLOCK or HANG ...]
```

Server may restart every ~20 seconds (Docker health check restart policy), but always hangs at the same point during sharding initialization.

### Affected Configurations

- **Docker Image:** themisdb/themisdb:hyperscaler (v1.3.4)
- **Configuration:** RAID Clustering with multiple shards (RAID0, RAID1, RAID5)
- **Environment Variables:**
  - THEMIS_ROLE=shard
  - THEMIS_SHARD_ID=*
  - THEMIS_RAID_MODE=stripe|mirror|parity
  - THEMIS_RAID_GROUP=*
  - THEMIS_SHARDS=* or THEMIS_BOOTSTRAP_SHARD=*
  - THEMIS_ENABLE_SHARDING=true

### Environment Details

- Base Image: Ubuntu 24.04
- CMake Build Flags:
  - THEMIS_ENABLE_LLM=ON
  - THEMIS_ENABLE_GPU=ON
  - THEMIS_ENABLE_CONTENT_PROCESSORS=ON
  - THEMIS_ENABLE_TRACING=ON
  - THEMIS_ENABLE_AVX2=ON
  - THEMIS_BUILD_TESTS=OFF
  - THEMIS_BUILD_BENCHMARKS=OFF

---

## Root Cause Analysis (In Progress)

### Suspected Root Causes

1. **Cluster Bootstrap/Discovery Timeout**
   - Shards 2+ may be attempting cluster discovery before Shard 1 is fully initialized
   - Connection attempt to bootstrap shard (Shard 1) times out or fails
   - Retry logic may be blocking initialization

2. **Inter-shard Communication Deadlock**
   - Shards trying to discover each other but network is not ready
   - DNS resolution of shard hostnames fails (Docker DNS not propagated)
   - Blocking wait on shard list from bootstrap node

3. **RocksDB Column Family Synchronization**
   - MVCC initialization across multiple shards may have race conditions
   - Cross-shard column family coordination might be blocking

4. **Sharding Manager Initialization**
   - Shard Manager might wait for all cluster members before proceeding
   - Coordination with unreachable shards causes timeout/deadlock
   - Missing or incorrect `THEMIS_BOOTSTRAP_SHARD` configuration

### Architecture Pattern
ThemisDB uses a **Bootstrap Node** pattern:
- **Shard 1 (Bootstrap Node):** Knows all cluster members, full `THEMIS_SHARDS` list
- **Other Shards:** Only know Bootstrap Node via `THEMIS_BOOTSTRAP_SHARD`
- **Discovery:** Non-bootstrap shards discover full cluster topology from Shard 1

---

## Debugging Steps

### Step 1: Verify Standalone Server (NO SHARDING) ✅ CONFIRMED WORKING

```bash
docker run -d -p 18766:18765 -p 8081:8080 \
  -e THEMIS_PORT=18765 \
  -e THEMIS_DATA_DIR=/var/lib/themisdb \
  -e THEMIS_ENABLE_METRICS=true \
  themisdb/themisdb:hyperscaler

# Result: ✅ Server starts successfully, reaches "READY FOR OPERATIONS"
```

**Conclusion:** Build and server code are correct. Problem is **clustering-specific**.

### Step 2: Verify Bootstrap Shard Configuration

Check if Shard 1 (bootstrap node) has correct config:
```bash
docker logs themis-raid0-shard1 2>&1 | grep -i "shard\|bootstrap\|discovery\|Adaptive Index"
```

Key markers to look for:
- `✅ Shard Manager initialized` or similar
- `Cluster discovery timeout: 30000ms`
- `Bootstrap node role detected`

### Step 3: Verify Non-Bootstrap Shard Discovery

Check if Shard 2/3 can reach Bootstrap node:
```bash
docker logs themis-raid0-shard2 2>&1 | grep -E "bootstrap|discovery|connecting|timeout|ERROR"
```

### Step 4: Docker DNS Troubleshooting

Verify Docker container DNS resolution works:
```bash
docker exec themis-raid0-shard2 \
  getent hosts themis-raid0-shard1

# Should return something like: 172.20.0.2  themis-raid0-shard1
```

If this fails, containers cannot reach each other by hostname.

### Step 5: Check Cluster Configuration

Verify configuration with docker exec:
```bash
docker exec themis-raid0-shard1 env | grep -E "THEMIS_SHARD|THEMIS_BOOTSTRAP"
docker exec themis-raid0-shard2 env | grep -E "THEMIS_SHARD|THEMIS_BOOTSTRAP"
```

Expected:
- Shard1: `THEMIS_SHARDS=themis-raid0-shard1:18765,themis-raid0-shard2:18765,themis-raid0-shard3:18765`
- Shard2: `THEMIS_BOOTSTRAP_SHARD=themis-raid0-shard1:18765`

### Step 6: Enable Debug Logging

Rebuild with debug logging enabled:
```bash
cmake -DCMAKE_BUILD_TYPE=Debug \
  -DTHEMIS_ENABLE_SHARDING_DEBUG=ON \
  ...
```

---

## Temporary Workaround

**DISABLE** problematic features during investigation:

```dockerfile
# Temporarily disable features to identify culprit
-DTHEMIS_ENABLE_LLM=OFF        # Try disabling LLM
# OR
-DTHEMIS_ENABLE_GPU=OFF        # Try disabling GPU
# OR
# Remove THEMIS_ENABLE_SHARDING env var
```

---

## Related Files

- [Dockerfile.themis-server](./Dockerfile.themis-server) - Build configuration
- [docker-compose-sharding.yml](./docker/compose/docker-compose-sharding.yml) - RAID cluster setup
- [src/indexes/adaptive_index_manager.cpp](./src/indexes/adaptive_index_manager.cpp)
- [src/sharding/shard_manager.cpp](./src/sharding/shard_manager.cpp)

---

## Timeline

- **10:39:37** - Server starts, logs "Adaptive Index Manager initialized"
- **10:39:57** - Server restarts (health check timeout), same hang
- **10:40:14** - Server restarts (health check timeout), same hang
- **Pattern:** Deadlock consistent, appears ~20 seconds into startup

---

## Next Steps

1. ✅ Create this issue
2. ⏳ Run standalone server test (no sharding)
3. ⏳ Run minimal shard test
4. ⏳ Identify which feature causes hang (LLM, GPU, Sharding, Content Processors)
5. ⏳ Locate blocking operation in source code
6. ⏳ Fix deadlock
7. ⏳ Rebuild and test

---

## Build/Release Impact

- Cannot deploy RAID clustering until fixed
- Standalone and basic deployments may work (TBD after testing)
- v1.3.4 release blocked until resolved

---

**Assigned to:** Debug Team  
**Priority:** 🔴 CRITICAL (blocking v1.3.4 RAID deployment)  
**Severity:** High (server cannot start with shard config)  
**Date Created:** 2026-01-04 11:40 UTC
