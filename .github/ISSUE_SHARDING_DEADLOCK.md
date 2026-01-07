# Sharding Startup Deadlock after "Adaptive Index Manager initialized"

**Status:** 🔴 Open
**Priority:** Critical (blocks RAID sharding deployments)
**Version:** v1.3.4
**Date:** 2026-01-04

## Summary
The server hangs during startup when sharding is enabled. Startup stops immediately after the log line:
```
Adaptive Index Manager initialized
```
The server never reaches PKI initialization, endpoint listing, or "READY FOR OPERATIONS".

## Impact
- All RAID/sharding deployments fail to start (RAID0/1/5 compose stack)
- Standalone (non-sharding) deployments work correctly

## Environment
- Image: themisdb/themisdb:hyperscaler (v1.3.4)
- Base: Ubuntu 24.04
- Flags: THEMIS_ENABLE_LLM=ON, THEMIS_ENABLE_GPU=ON, THEMIS_ENABLE_CONTENT_PROCESSORS=ON, THEMIS_ENABLE_TRACING=ON, THEMIS_ENABLE_AVX2=ON, THEMIS_BUILD_TESTS=OFF, THEMIS_BUILD_BENCHMARKS=OFF
- Env (example shard):
  - THEMIS_ROLE=shard
  - THEMIS_SHARD_ID=raid0-1
  - THEMIS_RAID_MODE=stripe
  - THEMIS_RAID_GROUP=raid0
  - THEMIS_ENABLE_SHARDING=true

## Evidence
- **Standalone (works):** RocksDB opens with 1 column family → server ready
- **Sharding (hangs):** RocksDB opens with 2 column families → last log: "Adaptive Index Manager initialized"
- PKIKeyProvider and later components never initialize

## Suspected Root Cause
Initialization order: `AdaptiveIndexManager` is constructed before the sharding/cluster manager is ready. With sharding enabled, RocksDB opens a second CF for cluster coordination; AdaptiveIndexManager attempts MVCC/snapshot work that requires sharding context and blocks.

## Proposed Fix
Reorder initialization in `src/server/http_server.cpp`:
1) Initialize Sharding Manager (or sharding context) **before** creating `AdaptiveIndexManager`.
2) Then initialize `AdaptiveIndexManager` with a sharding-ready context (or skip cross-shard coordination if single-node).

## Acceptance Criteria
- Logs proceed past "Adaptive Index Manager initialized" to PKI init and "READY FOR OPERATIONS" with sharding enabled.
- docker-compose-sharding.yml boots all shards healthy.
- Health endpoints respond 200 on all shard ports.

## Files of Interest
- src/server/http_server.cpp (constructor init order)
- include/indexes/adaptive_index_manager.h / src/indexes/adaptive_index_manager.cpp
- include/sharding/shard_manager.h / src/sharding/shard_manager.cpp
- docker/compose/docker-compose-sharding.yml (runtime env)

## Next Steps
- [ ] Implement init reordering or sharding-aware AdaptiveIndexManager
- [ ] Rebuild image
- [ ] Verify cluster startup (logs show PKI init and READY)
- [ ] Update CHANGELOG and docs
