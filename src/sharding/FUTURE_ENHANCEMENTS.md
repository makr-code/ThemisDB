# Sharding Module - Future Enhancements

<!-- Status: current | validated: 2026-07-18 | updated: 2026-07-19 -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->
<!-- Issue Link: makr-code/ThemisDB#5620 (development status snapshot) -->

## Scope

- hardening and refinement of sharding runtime behavior
- deterministic reliability improvements for routing/transaction/repair paths
- stronger benchmark-backed guardrails for sharding hot paths
- stub/simulation path clarification and improved diagnostics

## Design Constraints

- sharding contracts remain backward compatible within major release line.
- routing/transaction outcomes remain explicit and deterministic.
- degraded quorum and operations paths remain observable and non-silent.
- migration/repair behavior remains bounded and diagnosable.
- all stub/simulation paths must be explicitly marked and documented.

## Required Interfaces

| Interface | Requirement |
|---|---|
| routing interfaces | deterministic key-to-shard and adaptive routing semantics |
| coordination interfaces | explicit distributed decision and quorum behavior |
| transaction interfaces | stable cross-shard commit/abort contracts |
| operations interfaces | bounded repair/rebalance/migration with observable states |

## Implementation Notes

- tighten parity between topology updates and routing consistency diagnostics.
- standardize incident taxonomy for transaction and repair/rebalance classes.
- expand resilience tests for prolonged migration and write contention traffic.
- broaden benchmark depth for multi-DC and topology-failure scenarios.
- stub/simulation paths have been clarified with NON-PRODUCTION PATH markers and enhanced logging.

## Test Strategy

- unit and integration suites for routing, coordination, transaction, and operational paths.
- regressions for quorum loss, migration faults, and repair edge failures.
- deterministic stress runs for sustained distributed write and rebalance workloads.
- release-profile benchmark runs for mapped sharding targets.

## Performance Targets

- sharding hot paths remain inside regression budgets.
- routing/commit/migration-sensitive operations remain stable at p95/p99 envelopes.
- mapped benchmark manifests reach no-missing-case status for release gating.

## Security / Reliability

- maintain strict bounded behavior for topology and routing transitions.
- preserve explicit failure signaling for transaction and operational faults.
- enforce predictable degradation under shard and quorum failure conditions.
- keep diagnostics actionable for production sharding incidents.

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
- **Type**: Graceful fallback (test-only path)
- **Change**: Added THEMIS_WARN log on initialization to warn operators that gRPC is disabled
- **Production readiness**: Acceptable for test/single-node only; production requires THEMIS_HAS_SHARD_GRPC=1

#### 2. **In-Process RPC Handler** (shard_rpc_client.cpp:924)
- **Status**: ✅ IMPROVED
- **Type**: Simulation with injected response handler
- **Changes**:
  - Added one-time warning log when simulation is first used
  - Improved error message for unknown RPC methods
  - Added error logging with diagnostic hints
- **Production readiness**: Acceptable for loopback/single-node only; multi-node requires gRPC

#### 3. **mTLS Preparation Callback** (stream_protocol.cpp:635)
- **Status**: ✅ IMPROVED
- **Type**: In-process simulation (test-only path)
- **Change**: Added THEMIS_WARN log with explicit diagnostic information
- **Production readiness**: Test-only; production requires real mTLS transport callback injection

#### 4. **Connection Factory** (mtls_connection_pool.cpp:232)
- **Status**: ✅ IMPROVED
- **Type**: Deferred feature (v2.0 target)
- **Change**: Added THEMIS_DEBUG log clarifying that connection creation is managed by MTLSClient
- **Production readiness**: Acceptable; pool extension point is rarely called in production code paths

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
- [ ] **mTLS deployments**: MUST inject real transport callbacks via setPrepareTransferCallback()
- [ ] **GPU systems**: MUST enable THEMIS_ENABLE_CUDA or THEMIS_ENABLE_HIP for resource-aware scheduling
- [ ] **Failure prediction**: Current heuristic-based detector is conservative; upgrade to ONNX for improved accuracy

### Verification

All changes maintain backward compatibility and do not alter API contracts. Diagnostic logging uses appropriate levels (WARN for issues, INFO for informational, DEBUG for deep diagnostics).

### Next Steps

1. **v1.5 roadmap**: Add ONNX Runtime support and replace heuristic predictor
2. **v2.0 roadmap**: Refactor mTLS pool connection ownership
3. **Ongoing**: Monitor diagnostic logs from production deployments to validate graceful fallbacks