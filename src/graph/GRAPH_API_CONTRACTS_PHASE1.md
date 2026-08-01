/**
 * @file GRAPH_API_CONTRACTS_PHASE1.md
 * @brief Graph Module API Contract Definitions (Phase 1 - Frozen 2026-08-01)
 *
 * This document defines the canonical API contracts for all Graph module
 * components. Once frozen in Phase 1, these contracts are binding for all
 * Phase 2-6 implementation work.
 *
 * ## Modification Policy
 *
 * Contract-breaking changes are forbidden after Phase 1 freeze without:
 * 1. Full team review and consensus
 * 2. Deprecation period (minimum 2 releases)
 * 3. Migration documentation for all affected consumers
 * 4. Update to this document with rationale
 *
 * Non-breaking additions (new methods, parameters with defaults) may be
 * added in Phase 2-6 with minimal review (single approver).
 */

# Graph Module API Contracts — Phase 1 (2026-08-01)

## 1. Planning & Optimization Layer

### Component: GraphQueryOptimizer

**File**: `include/graph/graph_query_optimizer.h`

#### Contract Summary

Responsibility: Cost-based selection of traversal algorithm and execution plan.

**Preconditions**:
- Graph statistics populated via `GraphStatistics` struct
- Query AST well-formed (validated by parser)
- Cost model coefficients initialized (fallback to defaults if missing)

**Postconditions**:
- Returned plan is deterministic for identical inputs on identical platform
- Cost estimate is within ±10% of actual runtime (or documented exceptions)
- Selected algorithm is valid for query pattern (no invalid algorithm selection)

**Key Methods** (frozen):

```cpp
// Deterministic plan generation
Expected<QueryPlan, GraphErrorCode>
optimizeBFS(const QueryAST& query, const GraphStatistics& stats) noexcept;

Expected<QueryPlan, GraphErrorCode>
optimizeDFS(const QueryAST& query, const GraphStatistics& stats) noexcept;

Expected<QueryPlan, GraphErrorCode>
optimizeBidirectional(const QueryAST& query, const GraphStatistics& stats) noexcept;

// Cost model (deterministic for identical hardware)
Expected<double, GraphErrorCode>
estimateCost(const QueryPlan& plan, const GraphStatistics& stats) noexcept;

// Query pattern detection
Expected<QueryPattern, GraphErrorCode>
classifyPattern(const QueryAST& query) noexcept;
```

**Thread Safety**: Read-only; `GraphStatistics` accessed concurrently (safe).

**Error Contract**:
- `OPT_INVALID_QUERY_AST`: Input query is null or malformed
- `OPT_UNSUPPORTED_QUERY_PATTERN`: Pattern not implemented in this optimizer version
- `OPT_MISSING_GRAPH_STATISTICS`: Stats unavailable (fallback to defaults or error)
- `OPT_COST_CALC_OVERFLOW`: Cost calculation produces NaN/infinity/overflow
- `OPT_GPU_UNAVAILABLE`: GPU acceleration not available (fallback to CPU)
- `OPT_UNSATISFIABLE_CONSTRAINTS`: Constraint conjunction is unsatisfiable

---

### Component: GraphPlanCache

**File**: `include/graph/graph_plan_cache.h`

#### Contract Summary

Responsibility: LRU cache of optimized query plans for query pattern reuse.

**Preconditions**:
- Cache capacity configured (default: 1000 plans)
- Query hash function provided (must be deterministic)

**Postconditions**:
- Cache hit returns identical plan (byte-for-byte match)
- Cache eviction follows LRU policy (oldest accessed removed first)
- No memory leak even if cache is filled and flushed repeatedly

**Key Methods** (frozen):

```cpp
// Deterministic lookup
Optional<QueryPlan>
lookup(std::string_view query_hash) const noexcept;

// Insert with LRU eviction
void
insert(std::string_view query_hash, const QueryPlan& plan) noexcept;

// Stats for monitoring
struct CacheStats {
    size_t hit_count = 0;
    size_t miss_count = 0;
    size_t eviction_count = 0;
    double hit_rate() const noexcept { return hit_count / (hit_count + miss_count); }
};
```

**Thread Safety**: Concurrent reads are safe. Concurrent write requires external locking
(caller provides synchronization via `std::shared_mutex`).

**Error Contract**:
- `CACHE_PLAN_CACHE_FULL`: Capacity exceeded during insert (LRU eviction handles)
- `CACHE_MISS`: Planned cache miss on frequently-accessed query (fallback to re-optimize)

---

## 2. Traversal & Execution Layer

### Component: ParallelTraversal

**File**: `include/graph/parallel_traversal.h`

#### Contract Summary

Responsibility: Concurrent BFS/DFS from multiple source vertices.

**Preconditions**:
- Source vertex list non-empty
- Graph index valid and accessible
- Thread pool initialized (or auto-create from hardware concurrency)

**Postconditions**:
- All source vertices explored to `max_depth` (or until `max_results` reached)
- Per-source results independent (no cross-contamination)
- Merged result contains union of per-source discoveries

**Key Methods** (frozen):

```cpp
Expected<TraversalResult, GraphErrorCode>
multiSourceBFS(
    const std::vector<std::string>& sources,
    const Config& cfg
) noexcept;

Expected<TraversalResult, GraphErrorCode>
multiSourceDFS(
    const std::vector<std::string>& sources,
    const Config& cfg
) noexcept;
```

**Thread Safety**: Per-source traversal runs in isolated `std::async` task; no shared
mutable state between tasks. Result merge is thread-safe (copy-free).

**Error Contract**:
- `TRAV_VERTEX_NOT_FOUND`: Source vertex not in graph
- `TRAV_INVALID_EDGE_FILTER`: Edge filter predicate is null or malformed
- `TRAV_FRONTIER_OVERFLOW`: Frontier size exceeds `max_frontier_size`
- `TRAV_MAX_DEPTH_EXCEEDED`: Path depth > `max_depth`
- `TRAV_TIMEOUT`: Per-source timeout elapsed
- `TRAV_THREAD_CREATION_FAILED`: Thread pool creation failed
- `TRAV_CONSTRAINT_VIOLATION`: Constraint unsa tifiable during traversal

---

### Component: GPUTraversal

**File**: `include/graph/gpu_traversal.h`

#### Contract Summary

Responsibility: GPU-accelerated BFS/DFS with CPU fallback.

**Preconditions**:
- GPU available and initialized (CUDA/HIP)
- Graph adjacency data loaded to GPU memory
- Vertex count < 2^31 (GPU addressing limit)

**Postconditions**:
- Result identical to CPU algorithm (bit-for-bit, deterministic seeding)
- GPU memory managed automatically (allocated on first call, freed on destroy)
- Automatic fallback to CPU if GPU unavailable or fails

**Key Methods** (frozen):

```cpp
Expected<TraversalResult, GraphErrorCode>
traverseBFS(
    std::string_view source,
    size_t max_depth,
    const ConstraintSet& constraints
) noexcept;

// Query GPU availability
bool hasGPUSupport() const noexcept;
```

**Thread Safety**: Single-threaded GPU context per GPUTraversal instance.
Multiple instances may run concurrently (different GPU streams).

**Error Contract**:
- `TRAV_GPU_MEMORY_EXHAUSTED`: GPU memory insufficient; fallback to CPU
- `TRAV_GPU_KERNEL_FAILED`: Kernel launch or execution error; fallback to CPU
- `TRAV_VERTEX_NOT_FOUND`: Source not found (checked on CPU before GPU)
- `TRAV_CONSTRAINT_VIOLATION`: Constraint unsatisfiable (detected on CPU)

---

### Component: DistributedGraph

**File**: `include/graph/distributed_graph.h`

#### Contract Summary

Responsibility: Orchestrate graph queries across multiple shards.

**Preconditions**:
- Shard configuration complete (all shards reachable)
- Vertex-to-shard mapping deterministic (hash-based)
- RPC layer operational (or graceful degradation to single-shard)

**Postconditions**:
- Cross-shard query result is union of per-shard results (merge operation is idempotent)
- Partial failures handled: unavailable shards logged, partial results returned
- Query determinism guaranteed only if all shards available

**Key Methods** (frozen):

```cpp
Expected<TraversalResult, GraphErrorCode>
crossShardTraversal(
    std::string_view query_hash,
    const QueryPlan& plan
) noexcept;

// Shard membership query
Expected<ShardId, GraphErrorCode>
getShardForVertex(std::string_view vertex_id) const noexcept;
```

**Thread Safety**: Thread-safe for concurrent cross-shard queries.
RPC calls are async (non-blocking).

**Error Contract**:
- `DIST_INVALID_SHARD_CONFIG`: Shard configuration incomplete or malformed
- `DIST_VERTEX_UNHASHED`: Vertex does not hash to any shard (logic error)
- `DIST_MERGE_FAILED`: Per-shard results incompatible (schema mismatch)
- `DIST_SHARD_PEER_OFFLINE`: Shard unavailable; partial result returned
- `DIST_RPC_TIMEOUT`: RPC call exceeded deadline

---

## 3. Semantic & Reasoning Layer

### Component: OntologyManager

**File**: `include/graph/ontology_manager.h`

#### Contract Summary

Responsibility: Load and manage RDF ontology (YAML or Turtle format).

**Preconditions**:
- Ontology file accessible and readable
- YAML/Turtle syntax valid
- Namespace prefixes defined (or auto-mapped)

**Postconditions**:
- Ontology loaded into memory (no lazy loading)
- All triples indexed and queryable
- Thread-safe read access guaranteed

**Key Methods** (frozen):

```cpp
Expected<void, GraphErrorCode>
loadFromFile(std::string_view file_path) noexcept;

Expected<std::vector<Triple>, GraphErrorCode>
queryTriples(std::string_view subject_pattern) noexcept;
```

**Thread Safety**: Immutable after load; concurrent reads safe.

**Error Contract**:
- `REASON_ONTOLOGY_LOAD_FAILED`: File not found or parse error
- `REASON_INVALID_RULE_SYNTAX`: Ontology rule syntax malformed

---

### Component: KnowledgeGraphReasoner

**File**: `include/graph/knowledge_graph_reasoner.h`

#### Contract Summary

Responsibility: Forward-chaining inference engine for Horn clauses.

**Preconditions**:
- Ontology loaded
- Rules non-cyclic (or cycle detection enabled)
- Base facts provided

**Postconditions**:
- All derivable facts inferred (fixpoint reached)
- Inference deterministic for identical input (same seed)
- Conflict detection: contradictions reported

**Key Methods** (frozen):

```cpp
Expected<std::vector<Triple>, GraphErrorCode>
infer(const std::vector<Triple>& base_facts, size_t max_iterations = 1000) noexcept;

// Conflict detection
Expected<bool, GraphErrorCode>
hasConflict(const Triple& inferred, const Triple& base_fact) const noexcept;
```

**Thread Safety**: Single-threaded inference. Multiple reasoners may run concurrently.

**Error Contract**:
- `REASON_BINDING_FAILED`: Rule variable binding failed (no matching facts)
- `REASON_CYCLIC_RULE`: Cyclic rule dependency detected
- `REASON_INFERENCE_CONFLICT`: Inferred fact contradicts base fact
- `REASON_INVALID_RULE_SYNTAX`: Rule syntax error

---

## 4. Tensor Utilities Layer

### Component: TensorFingerprintGraph

**File**: `include/graph/tensor_fingerprint_graph.h`

#### Contract Summary

Responsibility: Compute graph fingerprint vectors for similarity/deduplication.

**Preconditions**:
- Graph non-empty
- Fingerprint dimension in range [16, 512]
- Embedding model trained or initialized

**Postconditions**:
- Fingerprint is deterministic (identical graph → identical vector)
- Vector is normalized (L2 norm = 1.0, ±1e-6)
- Distance metric: cosine similarity in [0, 1]

**Key Methods** (frozen):

```cpp
Expected<std::vector<float>, GraphErrorCode>
computeFingerprint(std::string_view graph_id) noexcept;

Expected<double, GraphErrorCode>
cosineSimilarity(
    std::string_view graph_id_a,
    std::string_view graph_id_b
) noexcept;
```

**Thread Safety**: Concurrent fingerprint computation safe (read-only graph access).

**Error Contract**:
- `TENSOR_INVALID_SHAPE`: Fingerprint dimension out of range
- `TENSOR_FINGERPRINT_FAILED`: Computation produced NaN/infinity
- `TENSOR_GPU_OP_UNAVAILABLE`: GPU tensor op unavailable; CPU fallback

---

### Component: TensorDeduplicationManager

**File**: `include/graph/tensor_deduplication_manager.h`

#### Contract Summary

Responsibility: Identify duplicate or near-duplicate graphs using fingerprints.

**Preconditions**:
- Fingerprint threshold in range (0.0, 1.0]
- Graphs registered and fingerprinted

**Postconditions**:
- Returned deduplication groups are reflexive and symmetric
- All graphs with similarity ≥ threshold in same group
- Group assignment deterministic

**Key Methods** (frozen):

```cpp
Expected<std::vector<DeduplicationGroup>, GraphErrorCode>
dedup(double threshold = 0.95) noexcept;
```

**Thread Safety**: Thread-safe for concurrent dedup operations.

**Error Contract**:
- `TENSOR_INVALID_THRESHOLD`: Threshold outside (0.0, 1.0]

---

## 5. Resource & Planning Layer

### Component: GraphResourcePool

**File**: `include/graph/graph_resource_pool.h`

#### Contract Summary

Responsibility: Bounded allocation of traversal resources (memory, threads).

**Preconditions**:
- Pool capacity configured (default: 1 GB, 8 threads)
- Resource requests bounded

**Postconditions**:
- No allocation exceeds pool capacity
- Fairness: no requester starved indefinitely
- Resource released automatically on scope exit (RAII)

**Key Methods** (frozen):

```cpp
Expected<ResourceHandle, GraphErrorCode>
acquireTraversalBudget(size_t memory_bytes, uint32_t thread_count) noexcept;
```

**Thread Safety**: Thread-safe concurrent acquisition with fairness guarantees.

**Error Contract**:
- `POOL_RESOURCE_EXHAUSTED`: Requested allocation exceeds available resources

---

## 6. Cross-Cutting Concerns

### Thread Safety Guarantees (Frozen)

| Component | Read Concurrency | Write Concurrency | Synchronization |
|-----------|------------------|--------------------|-----------------|
| GraphQueryOptimizer | ✅ Safe | ❌ Not allowed | RWMutex (optimizer internals) |
| GraphPlanCache | ✅ Safe | ⚠️ External sync needed | Caller-provided mutex |
| ParallelTraversal | ✅ Safe | N/A (per-instance) | Async task isolation |
| GPUTraversal | ⚠️ Sequential | ❌ Not allowed | GPU context serialization |
| DistributedGraph | ✅ Safe | ✅ Safe | Internal RWMutex + RPC sync |
| OntologyManager | ✅ Safe | ❌ Not allowed | Immutable after load |
| KnowledgeGraphReasoner | ⚠️ Sequential | ❌ Not allowed | Reasoner-internal state |
| TensorFingerprintGraph | ✅ Safe | ⚠️ Compute-intensive | GPU/CPU resource mgmt |
| TensorDeduplicationManager | ✅ Safe | ✅ Safe | Internal locking |
| GraphResourcePool | ✅ Safe | ✅ Safe | Atomic counters + mutex |

### Error Handling Convention (Frozen)

All component methods return `Expected<T, GraphErrorCode>` (C++23 `std::expected`-like).

- **Success**: `Expected` contains result `T`
- **Recoverable Fallback (category FALLBACK)**: Caller may retry on CPU or defer
- **Denial (category DENIAL)**: Caller must fix precondition or abort
- **Conflict (category REASONING_CONFLICT)**: Operator intervention required

### Determinism Contract (Frozen)

Methods tagged `deterministic` must produce identical results on identical hardware
for identical input (given fixed random seed if applicable).

```cpp
[[nodiscard]] Expected<QueryPlan, GraphErrorCode>
optimize(const QueryAST& query) noexcept; // deterministic
```

---

## 7. Phase 1 Freeze Attestation

**Date**: 2026-08-01  
**Frozen By**: Graph Module Hardening Phase 1  
**Status**: ✅ LOCKED FOR PHASE 2-6

These contracts are binding. No breaking changes permitted without full review.

---

## Related Documents

- `graph_error_taxonomy.h` — Error code definitions and category semantics
- `ROADMAP.md` — Phase 2-6 timeline and acceptance criteria
- `FUTURE_ENHANCEMENTS.md` — Planned features and forward compatibility
