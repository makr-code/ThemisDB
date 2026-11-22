---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-11-21
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
Source_Level: level1
SOT_Domain: module-behavior
Canonical_References:
  - ../../include/sharding/sharding_api_contract.h
  - ../../tests/sharding/CMakeLists.txt
  - ../../benchmarks/sharding/CMakeLists.txt
Related_Issue: "#7381, #7493"
Related_Milestone: DOC-WEEKLY-2026-W44
---

> Exact model backend not verifiable from final document text alone.

# Sharding Module - Future Enhancements

> **Status note (2026-10-31):** This is a planning document, not evidence that a feature or
> remediation is currently implemented or validated. Targets and implementation-status statements
> in the historical 2026-07-19 stub-remediation section require source revalidation. **TODO:**
> reconcile outstanding items with the current module roadmap and registered test/benchmark targets.

<!-- Status: current | validated: 2026-07-18 | updated: 2026-07-19 -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->
<!-- Issue Link: makr-code/ThemisDB#5620 (development status snapshot) -->

## Scope

- complete the GA hardening pass for routing, topology rebalancing, and GSI behavior already present in source
- verify production semantics under quorum loss, migration churn, and multi-DC latency variability
- convert current runtime logic into formal release-gate evidence on representative hardware
- keep non-production paths fail-closed and explicitly documented

## Design Constraints

- sharding contracts remain backward compatible within major release line.
- routing/transaction outcomes remain explicit and deterministic.
- degraded quorum and operations paths remain observable and non-silent.
- migration/repair behavior remains bounded and diagnosable.
- all non-production paths must be explicitly marked and documented.
- no fallback may silently route to an arbitrary shard when valid latency evidence is absent.

## Required Interfaces

| Interface | Requirement |
|---|---|
| routing interfaces | deterministic key-to-shard and adaptive routing semantics |
| coordination interfaces | explicit distributed decision and quorum behavior |
| Raft peer transport | Keep logical Raft group IDs distinct from member node IDs; authenticate each RPC peer against the configured member ID before durable vote or append handling |
| transaction interfaces | stable cross-shard commit/abort contracts |
| operations interfaces | bounded repair/rebalance/migration with observable states |

## Implementation Notes

- tighten parity between topology updates and routing consistency diagnostics.
- standardize incident taxonomy for transaction and repair/rebalance classes.
- expand resilience tests for prolonged migration and write contention traffic.
- broaden benchmark depth for multi-DC and topology-failure scenarios.
- keep the latency fallback semantics fail-closed when no measured RTT is valid under the timeout budget.
- GSI stale-entry invalidation must remain deterministic after shard migration or shard removal.

### Raft peer transport delivery boundary

The shard protobuf service now defines vote and AppendEntries RPCs. The client
binds its local node ID to the request, checks the configured remote member ID
in the response, verifies the server's DNS SAN through TLS, and enforces a
positive deadline capped at five seconds. The server requires verified mTLS,
maps the authenticated certificate SAN to a configured Raft member, resolves
the requested logical group, and dispatches to the durable `RaftState`
handlers. `RaftShardManager` installs the configured vote, append, and heartbeat
callbacks; without an explicit peer-config resolver the callbacks remain absent
and multi-node consensus cannot obtain quorum.

The acceptance gate is still open. Linux-only two- and three-process test
sources now cover manager-driven election, durable follower append,
follower SIGKILL/restart from the same journal, rejection of a conflicting
persisted log entry, persisted vote rejection, volatile-journal vote/append
rejection, stale-term/partial-prefix rejection, bounded timeout failure, and
proposal failure after both remote nodes are killed, plus separate leader-process
SIGKILL/restart recovery. They have not been compiled or run in the current
Windows benchmark configuration. Separate leader SIGKILL/restart is source
coverage and follower processes are now wired for independent re-election.
The focused gRPC
runtime test plus stale-term/log, unavailable-journal, partial-prefix, timeout,
and fault-injection cases must still execute before the gate can close. This
transport does not add conflict repair, safe membership changes, or a
quorum-backed 2PC decision state machine. CockroachDB Parallel Commits remain
out of scope until the full gate passes.

## Test Strategy

- unit and integration suites for routing, coordination, transaction, and operational paths.
- regressions for quorum loss, migration faults, and repair edge failures.
- deterministic stress runs for sustained distributed write and rebalance workloads.
- release-profile benchmark runs for mapped sharding targets.
- GA proof: 3-DC latency-routing test, topology churn rebalancing test, and GSI stale-entry test must all pass in `tests/sharding/test_sharding_core.cpp`.

## Performance Targets

- routing p95/p99 should remain stable within the release budget under degraded quorum and migration pressure.
- rebalancing under topology churn must preserve ≥80% steady-state throughput and finish within configured bounds.
- GSI equality and range queries must remain deterministic and bounded under shard churn.
- mapped benchmark manifests reach no-missing-case status for release gating.

## Security / Reliability

- maintain strict bounded behavior for topology and routing transitions.
- preserve explicit failure signaling for transaction and operational faults.
- enforce predictable degradation under shard and quorum failure conditions.
- keep diagnostics actionable for production sharding incidents.
- any route decision made without valid RTT evidence must remain fail-closed rather than silently redirecting to an arbitrary shard.

---

## Stub/Simulation Path Remediation (2026-07-19)

This section documents improvements made to stub and simulation code paths in the sharding module to ensure they are production-ready and properly documented.

### Summary of Changes

All identified stub/simulation paths have been reviewed and improved with:
1. **NON-PRODUCTION PATH markers** - replaced generic STUB/SIMULATION NOTE with explicit NON-PRODUCTION PATH comments
2. **Enhanced diagnostics** - added THEMIS_WARN/THEMIS_INFO logging to identify when stubs are used
3. **Error handling** - improved error messages and validation for edge cases
4. **Documentation** - clarified production deltas and removal plans

### Path-by-Path Improvements

#### 1. **gRPC Fallback Mode** (shard_rpc_client.cpp:127)
- **Status**: ✅ IMPROVED
- **Type**: Explicit in-process transport for test-mode builds only
- **Change**: Client construction rejects `inproc://` and `loopback://` endpoints outside `THEMIS_TEST_MODE`; production mode also rejects them. Non-Minimal release builds require the real shard-gRPC dependencies at configure time.
- **Production readiness**: Non-Minimal release artifacts require the real shard gRPC transport; explicit in-process endpoints are available only in test-mode builds.

#### 2. **In-Process RPC Handler** (shard_rpc_client.cpp:1037)
- **Status**: ✅ IMPROVED
- **Type**: Simulation with injected response handler
- **Changes**:
  - Added one-time warning log when simulation is first used
  - Rejects in-process endpoints outside test-mode builds and in production mode
  - Improved error message for unknown RPC methods
  - Added error logging with diagnostic hints
- **Production readiness**: Available only for test-mode endpoint simulations; multi-node requires gRPC

#### 3. **Shard-Transfer Stream Transport** ([DIS-03 / #8197](https://github.com/makr-code/ThemisDB/issues/8197))
- **Status**: Partial — dedicated mTLS gRPC staging, canonical snapshot
  materialization, and local verified staging reads are implemented. The
  application now also has typed routing-token descriptors, exact immutable
  ring-ownership snapshots, and a process-local MVCC snapshot reader with one
  RocksDB sequence, HLC boundary, bounded output, cancellation, and explicit
  `PUT`/`DELETE`. Focused test sources exist; build/execution evidence is
  pending. The application still needs durable assignment wiring, a restart-
  durable export, enforceable write quiescence, and authoritative fenced
  cutover.
- **Source-export blockers**: The new token and snapshot APIs are not wired to
  existing routing calls or to one another. The snapshot reader does not create
  a durable export or resume checkpoint; a source restart requires a fresh
  migration snapshot. Existing MVCC empty values are tombstones, so empty live
  values cannot be represented by this export API. No durable assignment
  authorizes the range, and snapshot capture does not quiesce concurrent
  writers.
- **Implementation record**: [Architecture and API record](ARCHITECTURE.md#dis-03-routing--und-snapshot-grundlagen)
  documents the implemented typed key/algorithm metadata, exact
  `(predecessor, token]` segments, snapshot sequence plus HLC, explicit
  `PUT`/`DELETE` and separate transport/HA gates. Implementation approval does
  not establish production authorization or cutover readiness.
- **Boundary**: The old `StreamSession` PREPARE callback remains fail-closed and
  is not the rebalance data path. See [API](API.md) and [Operations](OPERATIONS.md)
  for durable acknowledgements, resume, and explicit abort.
- **Remaining work**: Enforce range write quiescence through fenced cutover and
  reconcile lost cutover acknowledgements against the authoritative decision.

#### 4. **Connection Factory** (mtls_connection_pool.cpp:232)
- **Status**: ✅ COMPLETED (v2.0)
- **Type**: Factory pattern for connection lifecycle management
- **Changes**: 
  - Created IEndpointConnectionFactory abstract interface (include/sharding/mtls_connection_factory.h)
  - Implemented MTLSConnectionFactory concrete class (src/sharding/mtls_connection_factory.cpp)
  - Added EndpointConnectionPool constructor overload accepting factory
  - Updated createNewConnection() stub comment to mark COMPLETED
  - Added comprehensive factory-based integration tests (9 new tests)
  - Added deprecation warnings to MTLSClient methods
- **Production readiness**: COMPLETED - Factory pattern fully implemented and tested for v2.0

#### 5. **GPU VRAM Fallback** (shard_resource_manager.cpp:666)
- **Status**: ✅ VERIFIED
- **Type**: Graceful degradation (CPU-only systems)
- **Change**: Renamed marker from STUB/SIMULATION to NON-PRODUCTION PATH (Graceful Degradation)
- **Production readiness**: Acceptable; expected behavior on systems without CUDA/HIP

#### 6. **ONNX Model Predictor** (predictive_detector.cpp:45)
- **Status**: ✅ IMPROVED
- **Type**: Heuristic-based simulation (acceptable fallback)
- **Changes**:
  - Clarified that sigmoid-calibrated heuristics are conservative and acceptable
  - Added THEMIS_INFO log to loadModel() explaining heuristic-based operation
  - Improved documentation noting that this is a valid early warning system
- **Production readiness**: Acceptable for current release; upgrade to ONNX in v1.5+

### Migration Checklist

For operators deploying these modules in production:

- [ ] **Single-node deployments**: All in-process/simulation paths are acceptable
- [ ] **Multi-node deployments**: MUST enable THEMIS_HAS_SHARD_GRPC=1 and deploy real shard peers
- [ ] **Multi-node deployments**: Configure the gRPC staging store, trusted
  assignment and export adapters; keep automatic cutover disabled without an
  authoritative fenced ownership service.
- [ ] **GPU systems**: MUST enable THEMIS_ENABLE_CUDA or THEMIS_ENABLE_HIP for resource-aware scheduling
- [ ] **Failure prediction**: Current heuristic-based detector is conservative; upgrade to ONNX for improved accuracy

### Verification

`tests/sharding/test_dis03_routing_contract_focused.cpp` and
`tests/storage/test_mvcc_range_export.cpp` cover the new source-level
contracts but have not yet been executed.
`StreamSessionTransportFocusedTests` covers the old fail-closed seam.
`ShardTransferStoreTests` and `ShardTransferGrpcTests` target durable staging and
mTLS transport, not production snapshot or HA ownership transitions. DIS-03
remains open until the deployment wiring and cutover evidence are available.

### Next Steps

1. **v1.5 roadmap**: Add ONNX Runtime support and replace heuristic predictor
2. **v2.0 roadmap** (COMPLETED Phase 2):
   - ✅ IEndpointConnectionFactory interface created
   - ✅ MTLSConnectionFactory concrete implementation
   - ✅ EndpointConnectionPool factory-based constructor
   - ✅ Factory-based integration tests (9 comprehensive tests)
   - ✅ MTLSClient deprecation warnings added
   - **Phase 3 (Next)**: Wire factory injection into MTLSClient for production use
3. **Ongoing**: Monitor diagnostic logs from production deployments to validate graceful fallbacks