# Cross-Module Integration Map: Tensor Hardening Phase 2-3

**Status**: PLANNED (Aug 21-26, Phase 3)  
**Scope**: Tensor module post-Phase 2 hardening integration validation  
**Owner**: general-purpose integration test agent

---

## Module Dependency Matrix

### Direct Dependencies (Tensor → Other Modules)

```
┌─────────────────────────────────────────────────────────────┐
│                    TENSOR MODULE                             │
├─────────────────────────────────────────────────────────────┤
│ Phase 2: A2 Diagnostics + Error Codes (TENSOR-9500..9599)  │
│ Phase 3: Integration validation across downstream modules   │
└─────────────────────────────────────────────────────────────┘
     ↓        ↓        ↓         ↓         ↓
  QUERY    STORAGE    LLM      SERVER   SHARDING
```

### Detailed Dependency Graph

| Tensor Subsystem | Dependent Module | Interaction Type | Integration Test | Priority |
|------------------|------------------|------------------|------------------|----------|
| fingerprint_graph | query (AQL) | Deduplication, mutations | TINT-Q-01..04 | **HIGH** |
| index_manager | storage (RocksDB) | Blob reads/writes, persistence | TINT-S-01..04 | **HIGH** |
| core_bridge | llm (GGML) | Embedding routing, inference | TINT-L-01..04 | **HIGH** |
| ingestion_bridge | server (multi-tenant) | Concurrent indexing, isolation | TINT-SV-01..04 | **HIGH** |
| redundancy_detection | sharding (distribution) | Graph state consistency, replication | TINT-SH-01..04 | **HIGH** |
| compression_routing | query + storage | Data format negotiation | TINT-Q-05, TINT-S-05 | **MEDIUM** |
| workflow_observability | server (diagnostics) | Event emission, correlation | TINT-SV-05..08 | **MEDIUM** |

---

## Integration Test Suite Design

### Test 1: Tensor ↔ Query (AQL Mutations)

**Test File**: `tests/tensor/test_tensor_query_integration_focused.cpp`  
**Duration**: 5-10 min each

#### TINT-Q-01: Basic Deduplication via AQL
```gherkin
Given: Tensor index with fingerprint graph (1,000 vectors)
When: Execute AQL mutation `INSERT OVERWRITE ... SELECT dedup(...)`
Then: Verify fingerprint detection prevents duplicates
  AND: No new CRITICAL errors logged
  AND: Diagnostics show successful deduplication
```

#### TINT-Q-02: Deduplication with Error Paths
```gherkin
Given: Tensor index with a corrupted fingerprint
When: Execute AQL with 50% adapter failure rate
Then: Verify graceful degradation (fallback to content hash)
  AND: Emit TENSOR-9505 diagnostic
  AND: Query succeeds with reduced confidence
```

#### TINT-Q-03: AQL Query + Tensor Bridge Routing
```gherkin
Given: Multi-index query (tensor + traditional)
When: Execute complex FILTER on tensor results
Then: Verify index_manager bridges to LLM embeddings correctly
  AND: No silent failures in GGML graph selection
```

#### TINT-Q-04: Concurrent Mutations with Locking
```gherkin
Given: 10 concurrent AQL INSERT threads
When: Execute simultaneous mutations on same index
Then: Verify no race conditions in fingerprint_graph
  AND: All mutations committed atomically
```

#### TINT-Q-05: Compression Negotiation
```gherkin
Given: Query engine with compression_routing
When: Request vector data with ZstdCompression
Then: Verify tensor_core_bridge compresses correctly
  AND: Query engine decompresses on read
```

### Test 2: Tensor ↔ Storage (RocksDB Integration)

**Test File**: `tests/tensor/test_tensor_storage_integration_focused.cpp`  
**Duration**: 5-10 min each

#### TINT-S-01: Blob Write + Fingerprint Persistence
```gherkin
Given: In-memory tensor index
When: Flush to RocksDB via core_bridge::write()
Then: Verify fingerprint_graph persisted correctly
  AND: No silent RocksDB delete() errors
  AND: Blob metadata includes fingerprints
```

#### TINT-S-02: RocksDB Corruption Recovery
```gherkin
Given: RocksDB with corrupted blob entry
When: tensor_index_manager loads index
Then: Detect corruption via fingerprint mismatch
  AND: Emit TENSOR-9520 diagnostic
  AND: Fallback to alternative blob or skip
```

#### TINT-S-03: Concurrent Read/Write via core_bridge
```gherkin
Given: Concurrent reader + writer threads on storage
When: Reader accesses index while writer flushes
Then: Verify atomic blob operations (no partial reads)
  AND: Reader sees consistent snapshot
```

#### TINT-S-04: Index Manager → Storage Mapping
```gherkin
Given: tensor_index_manager with 3-mode error cases (TINT-S-04)
When: Each failure mode triggers (HIGH-3 from A2)
Then: Verify emitDiagnostic() differentiates:
  AND: TENSOR-9510 for graph construction failure
  AND: TENSOR-9511 for index lookup failure
  AND: TENSOR-9512 for adapter routing failure
```

### Test 3: Tensor ↔ LLM (GGML Embedding Pipeline)

**Test File**: `tests/tensor/test_tensor_llm_integration_focused.cpp`  
**Duration**: 5-15 min each (GGML can be slower)

#### TINT-L-01: GGML Graph Routing via core_bridge
```gherkin
Given: Tensor fingerprint_graph with 10 GGML adapters
When: core_bridge selects adapter for embedding
Then: Verify adapter selection is deterministic
  AND: Graph construction succeeds or emits TENSOR-9501
  AND: No silent failures (CRITICAL-1 fix validation)
```

#### TINT-L-02: Concurrent GGML Inference
```gherkin
Given: 8 concurrent embedding requests
When: All route through different adapters
Then: Verify no race conditions in shared GGML context
  AND: Each inference completes successfully
  AND: Concurrent diagnostics are atomic
```

#### TINT-L-03: GGML Context Cleanup on Error
```gherkin
Given: Tensor core_bridge with adapter failure
When: GGML inference fails
Then: Verify resource cleanup (emitDiagnostic + cleanup)
  AND: No dangling GGML contexts
```

#### TINT-L-04: Embedding Batch Processing
```gherkin
Given: 1,000 vectors requiring embeddings
When: core_bridge::processEmbeddingBatch() executes
Then: Verify batching optimization
  AND: All diagnostics emitted correctly
  AND: No message formatting issues (LOW-1)
```

### Test 4: Tensor ↔ Server (Multi-Tenant Isolation)

**Test File**: `tests/tensor/test_tensor_server_integration_focused.cpp`  
**Duration**: 5-10 min each

#### TINT-SV-01: Multi-Tenant Index Isolation
```gherkin
Given: Tensor index with 3 different tenants
When: Concurrent operations (create, query, drop) per tenant
Then: Verify complete isolation (no cross-tenant leakage)
  AND: Each tenant sees only their fingerprints
```

#### TINT-SV-02: Bounded Concurrency Under Load
```gherkin
Given: Concurrent hardening limits (256 creates, 16 decomposes)
When: 512 concurrent create ops requested
Then: Verify queue bounded (never exceeds limit)
  AND: Excess ops fail gracefully with TENSOR-9530
  AND: No resource exhaustion crashes
```

#### TINT-SV-03: Observability + Event Correlation
```gherkin
Given: Tensor operations generating diagnostics
When: workflow_observability emits events
Then: Verify server-side correlation (trace IDs)
  AND: Events captured in field_diagnostics_collector
  AND: Queries can trace errors end-to-end
```

#### TINT-SV-04: Security: Principal Isolation
```gherkin
Given: RBAC-controlled tensor access
When: Unprivileged principal accesses index
Then: Verify rejection with TENSOR-9540 error
  AND: Audit log entry created
  AND: No diagnostic leakage (sensitive fields masked)
```

#### TINT-SV-05..08: Stress Scenarios
- SV-05: 48h concurrent workload stability
- SV-06: Memory leak detection (ASan during workload)
- SV-07: ThreadSanitizer race detection
- SV-08: Graceful shutdown with in-flight ops

### Test 5: Tensor ↔ Sharding (Distributed State)

**Test File**: `tests/tensor/test_tensor_sharding_integration_focused.cpp`  
**Duration**: 10-20 min each (distributed is slower)

#### TINT-SH-01: Distributed Fingerprint Graph Consistency
```gherkin
Given: 3-shard tensor cluster with replicated graphs
When: Shard 2 fails during replication
Then: Verify failover restores graph consistency
  AND: No fingerprint divergence detected
  AND: Reconciliation completes successfully
```

#### TINT-SH-02: Graph State Replication Atomicity
```gherkin
Given: Shard with pending fingerprint additions
When: Replication network partitions mid-sync
Then: Verify all-or-nothing replication (no partial state)
  AND: Emit TENSOR-9550 diagnostic on partition
  AND: Automatic recovery on reconnect
```

#### TINT-SH-03: Cross-Shard Deduplication
```gherkin
Given: Identical vectors ingested on Shard A and B
When: Sharding layer reconciles
Then: Verify global deduplication works
  AND: Fingerprints matched across shards
```

#### TINT-SH-04: Consistent Snapshot Export
```gherkin
Given: Distributed tensor index under concurrent writes
When: Graph export triggered
Then: Verify snapshot is consistent (no torn writes)
  AND: Export includes all shard fingerprints
```

---

## Integration Test Configuration

### Test Framework

```cmake
# tests/tensor/CMakeLists.txt additions

# Integration test targets
themis_register_module_focused_test(
  NAME test_tensor_query_integration_focused
  SOURCES test_tensor_query_integration_focused.cpp
  TIMEOUT 600
  LABELS tensor integration release_critical
)

themis_register_module_focused_test(
  NAME test_tensor_storage_integration_focused
  SOURCES test_tensor_storage_integration_focused.cpp
  TIMEOUT 600
  LABELS tensor integration release_critical
)

themis_register_module_focused_test(
  NAME test_tensor_llm_integration_focused
  SOURCES test_tensor_llm_integration_focused.cpp
  TIMEOUT 900  # GGML tests may be slower
  LABELS tensor integration release_critical
)

themis_register_module_focused_test(
  NAME test_tensor_server_integration_focused
  SOURCES test_tensor_server_integration_focused.cpp
  TIMEOUT 600
  LABELS tensor integration release_critical
)

themis_register_module_focused_test(
  NAME test_tensor_sharding_integration_focused
  SOURCES test_tensor_sharding_integration_focused.cpp
  TIMEOUT 1200  # Distributed tests are slowest
  LABELS tensor integration release_critical
)
```

### CI/CD Integration

```yaml
# .github/workflows/09-pr-gates_release-critical-tests.yml

- name: Run Tensor Integration Tests
  run: |
    ctest \
      --preset linux-release \
      -L "release_critical" \
      --output-on-failure \
      -j 4 \
      --timeout 120
```

---

## Dependency Order (Phase 3 Execution)

Execute integration tests in this order to catch upstream failures early:

1. **TINT-S-01..04** (Storage) — lowest-level, most critical for persistence
2. **TINT-Q-01..05** (Query) — depends on storage working
3. **TINT-L-01..04** (LLM) — depends on storage + query
4. **TINT-SV-01..08** (Server) — depends on all above
5. **TINT-SH-01..04** (Sharding) — depends on all above (distributed)

---

## Success Criteria (Phase 3 DONE)

- ✅ All 40+ integration tests PASS (100% pass rate)
- ✅ ThreadSanitizer clean across all modules
- ✅ No new CRITICAL/HIGH findings introduced by Phase 2 fixes
- ✅ Release_critical CI gate stable (no flakes)
- ✅ Cross-module dependency matrix verified
- ✅ Ready for Phase 4 PR preparation

---

## Known Dependencies & Constraints

| Constraint | Impact | Mitigation |
|-----------|--------|-----------|
| GGML tests run serially (GPU contention) | Slow CI | Run with `-j 1` for L tests |
| RocksDB requires librocksdb-dev on Linux | Setup overhead | Pre-install in CI environment |
| Sharding tests need 3+ instances | Resource intensive | Use Docker Compose or local multi-process |
| Network partition simulation (TINT-SH-02) | Flaky | Use chaos engineering libs (pumba, toxiproxy) |

