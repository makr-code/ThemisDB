/**
 * @file GRAPH_FAILURE_MODES_DOCUMENTATION.md
 * @brief Graph Module Failure Modes & Recovery Strategies
 * 
 * Comprehensive catalog of failure modes, symptoms, causes, and recovery procedures
 * for all graph module components across Phases 2-6 hardening.
 *
 * @version 1.0.0
 * @date 2026-08-02
 */

# Graph Module Failure Modes & Recovery Strategies

This document catalogs all known failure modes in the hardened graph module,
along with symptoms, root causes, recovery procedures, and prevention strategies.

---

## Category A: Optimization Failures

### A1. Cost Overflow (OPT_COST_CALC_OVERFLOW)

**Symptom**:
- Cost estimation returns error `OPT_COST_CALC_OVERFLOW`
- Query plan generation fails or falls back to CPU

**Root Causes**:
- High fan-out graph (>1000 children per vertex)
- Deep traversal request (max_depth > 10) combined with high fan-out
- Frontier explosion: estimated frontier exceeds 100M vertices
- Cost calculation produces NaN or infinity

**Recovery Procedures**:
1. **Immediate**: Fall back to CPU-only traversal with reduced scope
2. **Short-term**: Add vertex constraints to reduce frontier (e.g., labels)
3. **Medium-term**: Reduce max_depth parameter
4. **Long-term**: Rebuild graph statistics or restructure high-fan-out areas

**Example Recovery Code**:
```cpp
auto result = optimizer.estimateCost(plan, stats);
if (!result) {
    if (result.error() == GraphErrorCode::OPT_COST_CALC_OVERFLOW) {
        // Recovery: reduce depth and retry
        plan.max_depth = std::min(plan.max_depth, 5);
        result = optimizer.estimateCost(plan, stats);
        // Or: add constraints
        constraints.node_labels.push_back("important");
        // Re-run optimization
    }
}
```

**Prevention**:
- Pre-flight check: estimate fan-out before traversal
- Guard against frontier explosion with `isCostOverflowRisk()`
- Enforce max_depth ≤ 1000 (via validation)

---

### A2. Stale Graph Statistics (OPT_MISSING_GRAPH_STATISTICS)

**Symptom**:
- Cost estimation returns `OPT_MISSING_GRAPH_STATISTICS`
- Query optimization uses fallback default cost model

**Root Causes**:
- Graph statistics older than 1 hour (staleness threshold)
- Statistics collection never completed
- Statistics corrupted or invalid

**Recovery Procedures**:
1. **Immediate**: Use fallback cost model (assumes avg degree = 10)
2. **Short-term**: Trigger on-demand statistics refresh (may block 1-2 seconds)
3. **Long-term**: Investigate statistics collection pipeline

**Example Recovery Code**:
```cpp
auto stats = graph_manager.getStatistics();
if (!areStatisticsFresh(stats.last_updated)) {
    // Option 1: Use fallback
    stats = GraphStatistics::defaultFallback();
    
    // Option 2: Refresh (blocking)
    stats = graph_manager.refreshStatistics();
}
```

**Prevention**:
- Schedule statistics refresh every 30 minutes
- Monitor statistics staleness in metrics
- Alert when last refresh > 45 minutes ago

---

### A3. Invalid Query AST (OPT_INVALID_QUERY_AST)

**Symptom**:
- Optimization immediately returns `OPT_INVALID_QUERY_AST`
- No query plan generated

**Root Causes**:
- Null or dangling pointer to QueryAST
- Malformed query AST (missing required fields)
- Parser error not caught upstream

**Recovery Procedures**:
1. **Immediate**: Reject query with user-facing error message
2. **Short-term**: Inspect query parser logs
3. **Long-term**: Improve query parser validation

**Example Recovery Code**:
```cpp
if (!query.isValid()) {
    return Err(GraphErrorCode::OPT_INVALID_QUERY_AST,
               "Query AST validation failed; check parser output");
}
```

**Prevention**:
- Enforce query AST validation in parser
- Add schema validation before optimizer
- Unit tests for malformed inputs

---

## Category B: Traversal Failures

### B1. Frontier Overflow (TRAV_FRONTIER_OVERFLOW)

**Symptom**:
- Traversal aborts with `TRAV_FRONTIER_OVERFLOW`
- Partial results returned (up to 10M vertices)

**Root Causes**:
- High fan-out graph with multi-source BFS
- Aggregate frontier exceeds 10M vertices
- Per-source frontier exceeds 1M vertices
- Inadequate max_results limit

**Recovery Procedures**:
1. **Immediate**: Use partial results (valid but incomplete)
2. **Short-term**: Retry with reduced max_depth or max_results
3. **Medium-term**: Add vertex constraints to focus traversal
4. **Long-term**: Use distributed graph if available

**Example Recovery Code**:
```cpp
auto result = traversal.multiSourceBFS(sources, config);
if (!result && result.error() == GraphErrorCode::TRAV_FRONTIER_OVERFLOW) {
    // Partial results available
    config.max_depth = std::min(config.max_depth, 5);
    result = traversal.multiSourceBFS(sources, config);
}
```

**Prevention**:
- Pre-flight frontier size estimation
- Guard with `isFrontierValid(per_source, aggregate)`
- Enforce per-source ≤ 1M, aggregate ≤ 10M limits

---

### B2. Vertex Not Found (TRAV_VERTEX_NOT_FOUND)

**Symptom**:
- Traversal fails with `TRAV_VERTEX_NOT_FOUND`
- Specific vertex is unreachable or doesn't exist

**Root Causes**:
- Source vertex doesn't exist in graph
- Source vertex ID is empty string
- Typo in vertex ID
- Graph snapshot changed between queries

**Recovery Procedures**:
1. **Immediate**: Return empty result or error to user
2. **Short-term**: Verify vertex exists via getVertex()
3. **Medium-term**: Check graph version/snapshot timestamp
4. **Long-term**: Implement vertex existence cache

**Example Recovery Code**:
```cpp
if (!isValidVertexId(source)) {
    return Err(GraphErrorCode::TRAV_VERTEX_NOT_FOUND,
               "Source vertex is empty or invalid");
}
if (!graph.hasVertex(source)) {
    return Err(GraphErrorCode::TRAV_VERTEX_NOT_FOUND,
               "Source vertex does not exist in current graph");
}
```

**Prevention**:
- Validate vertex IDs early (non-empty, within length limits)
- Check vertex existence before traversal
- Fail-fast on invalid inputs

---

### B3. Max Depth Exceeded (TRAV_MAX_DEPTH_EXCEEDED)

**Symptom**:
- Traversal rejects max_depth parameter
- Error: `TRAV_MAX_DEPTH_EXCEEDED`

**Root Causes**:
- max_depth = 0 (invalid, must be ≥ 1)
- max_depth > 1000 (exceeds practical limit)
- Off-by-one error in client code

**Recovery Procedures**:
1. **Immediate**: Use default max_depth = 10 or clamp to [1, 1000]
2. **Short-term**: Log warning about invalid depth
3. **Long-term**: Educate users on depth constraints

**Example Recovery Code**:
```cpp
if (!isDepthValid(config.max_depth)) {
    spdlog::warn("Invalid max_depth {}; clamping to [1, 1000]", config.max_depth);
    config.max_depth = std::clamp(config.max_depth, 1UL, 1000UL);
}
```

**Prevention**:
- Document depth constraints in API
- Validate on input with `isDepthValid()`
- Provide reasonable defaults

---

## Category C: Distributed Graph Failures

### C1. Shard Peer Offline (DIST_SHARD_PEER_OFFLINE)

**Symptom**:
- Cross-shard traversal returns partial results
- Error code: `DIST_SHARD_PEER_OFFLINE`
- Some shards' results missing from aggregate

**Root Causes**:
- Shard server crashed or restarted
- Network partition between coordinator and shard
- Shard undergoing maintenance

**Recovery Procedures**:
1. **Immediate**: Return partial results from available shards (graceful degradation)
2. **Short-term**: Retry via failover replica shard
3. **Medium-term**: Trigger shard health check and monitoring alert
4. **Long-term**: Implement shard replication

**Example Recovery Code**:
```cpp
std::vector<GraphErrorCode> shard_errors;
for (auto& [shard_id, future] : shard_tasks) {
    auto result = future.get();
    if (!result.is_ok) {
        shard_errors.push_back(GraphErrorCode::DIST_SHARD_PEER_OFFLINE);
        // Continue with other shards (partial result acceptable)
        continue;
    }
    merged.merge(result.data);
}
// Return partial results if some shards failed
return Ok(merged);
```

**Prevention**:
- Implement shard health monitoring
- Use heartbeat checks (e.g., ping every 5s)
- Configure per-shard timeout (30s default)
- Alert on shard downtime > 60s

---

### C2. RPC Timeout (DIST_RPC_TIMEOUT)

**Symptom**:
- Cross-shard RPC call times out
- Error: `DIST_RPC_TIMEOUT`
- Shard result not included in aggregate

**Root Causes**:
- Slow network between coordinator and shard
- Shard processing slow (high query load)
- RPC timeout too aggressive

**Recovery Procedures**:
1. **Immediate**: Timeout and skip shard (return partial results)
2. **Short-term**: Retry on replica shard
3. **Medium-term**: Increase RPC timeout (30s → 60s) if acceptable
4. **Long-term**: Load-balance traffic to reduce shard latency

**Example Recovery Code**:
```cpp
try {
    auto result = rpc_client_->executeQuery(
        shard.endpoint, plan, 
        std::chrono::seconds(30)  // 30s timeout
    );
} catch (const TimeoutException&) {
    shard_errors.push_back(GraphErrorCode::DIST_RPC_TIMEOUT);
    // Continue with other shards
}
```

**Prevention**:
- Set RPC timeout > p99 of shard latency + 2σ
- Monitor per-shard RPC latency
- Alert if p99 latency > 20s
- Implement RPC timeout backoff (retry with longer timeout)

---

### C3. Merge Conflict (DIST_MERGE_FAILED)

**Symptom**:
- Cross-shard merge fails
- Error: `DIST_MERGE_FAILED`
- Full traversal cannot proceed

**Root Causes**:
- Schema mismatch between shards
- Duplicate vertex IDs from different shards
- Conflicting edge weights or attributes

**Recovery Procedures**:
1. **Immediate**: Return error with diagnostic info
2. **Short-term**: Implement per-shard schema validation
3. **Medium-term**: Use schema evolution version tracking
4. **Long-term**: Enforce schema consistency across shards

**Example Recovery Code**:
```cpp
if (!canMerge(merged, shard_result)) {
    spdlog::error("Merge conflict: shard {} schema mismatch", shard_id);
    return Err(GraphErrorCode::DIST_MERGE_FAILED,
               "Cannot merge results due to schema mismatch");
}
```

**Prevention**:
- Validate schema before merge
- Track schema version per shard
- Implement schema evolution procedures
- Unit tests for schema compatibility

---

## Category D: GPU & Acceleration Failures

### D1. GPU Unavailable (OPT_GPU_UNAVAILABLE)

**Symptom**:
- GPU acceleration not available
- Optimizer automatically falls back to CPU
- Error: `OPT_GPU_UNAVAILABLE`

**Root Causes**:
- No CUDA-capable GPU present
- CUDA drivers not installed
- GPU already allocated to other processes
- GPU device offline

**Recovery Procedures**:
1. **Immediate**: Automatic fallback to CPU (transparent to caller)
2. **Short-term**: Log info-level message
3. **Medium-term**: Monitor GPU availability
4. **Long-term**: Provision GPU resources or use CPU-only deployment

**Example Recovery Code**:
```cpp
if (!hasGPUSupport()) {
    spdlog::info("GPU not available; falling back to CPU traversal");
    return cpuTraversal(source, max_depth, constraints);
}
```

**Prevention**:
- Check GPU availability at startup
- Monitor GPU health periodically
- Have CPU fallback path always available
- Document GPU requirements

---

### D2. GPU Memory Exhausted (TRAV_GPU_MEMORY_EXHAUSTED)

**Symptom**:
- GPU kernel cannot allocate required memory
- Error: `TRAV_GPU_MEMORY_EXHAUSTED`
- May or may not have fallback available

**Root Causes**:
- Frontier too large for GPU memory (typically <8GB)
- Other GPU processes consuming memory
- GPU memory leak

**Recovery Procedures**:
1. **Immediate**: Return error (no automatic fallback)
2. **Short-term**: 
   - Option A: Fall back to CPU
   - Option B: Reduce traversal scope and retry
3. **Medium-term**: Implement GPU memory fragmentation cleanup
4. **Long-term**: Use unified memory (UMM) or smaller batch sizes

**Example Recovery Code**:
```cpp
if (!hasGPUMemory(GPU_MEMORY_REQUIRED)) {
    spdlog::warn("GPU memory exhausted ({} bytes required)", GPU_MEMORY_REQUIRED);
    // Option 1: Fallback to CPU
    return cpuTraversal(source, max_depth, constraints);
    // Option 2: Return error and let caller decide
    // return Err(GraphErrorCode::TRAV_GPU_MEMORY_EXHAUSTED);
}
```

**Prevention**:
- Check GPU memory before GPU traversal
- Estimate GPU memory requirement upfront
- Implement GPU memory pooling
- Monitor GPU memory usage

---

### D3. Constraint Violation on GPU (TRAV_CONSTRAINT_VIOLATION)

**Symptom**:
- GPU cannot satisfy certain constraint types
- Traversal falls back to CPU or returns error

**Root Causes**:
- Policy constraints not supported by GPU (Category C)
- Complex semantic constraints requiring reasoning
- Time-based constraints

**Recovery Procedures**:
1. **Immediate**: CPU fallback (if constraint type known to need it)
2. **Short-term**: Document which constraints work on GPU
3. **Medium-term**: Implement GPU support for more constraint types
4. **Long-term**: Hybrid execution (GPU for compatible parts)

**Example Recovery Code**:
```cpp
for (const auto& constraint : constraints) {
    if (!gpuSupportsConstraint(constraint)) {
        spdlog::info("Constraint type {} not GPU-supported; using CPU", 
                     constraint.type);
        return cpuTraversal(source, max_depth, constraints);
    }
}
```

**Prevention**:
- Document GPU constraint support matrix
- Validate constraints before GPU commit
- Provide CPU fallback for all constraint types
- Gradually expand GPU constraint support

---

## Category E: Resource Management Failures

### E1. Resource Pool Exhausted (POOL_RESOURCE_EXHAUSTED)

**Symptom**:
- Resource acquisition returns `POOL_RESOURCE_EXHAUSTED`
- Traversal request blocked or rejected

**Root Causes**:
- All traversal budgets already allocated
- High concurrent load (many simultaneous queries)
- Long-running traversals holding resources

**Recovery Procedures**:
1. **Immediate**: Block and wait (FIFO fairness, max 30s timeout)
2. **Short-term**: If timeout: return error with diagnostic
3. **Medium-term**: Implement resource preemption for high-priority queries
4. **Long-term**: Scale resource pool size based on load

**Example Recovery Code**:
```cpp
auto budget = pool.acquireTraversalBudget(
    memory_bytes, thread_count,
    std::chrono::seconds(30)  // 30s timeout
);
if (!budget) {
    if (budget.error() == GraphErrorCode::POOL_RESOURCE_EXHAUSTED) {
        spdlog::warn("Resource pool exhausted; timeout after 30s");
        // Option: retry with smaller budget
        // Option: use degraded mode with fewer threads
        return Err(GraphErrorCode::POOL_RESOURCE_EXHAUSTED);
    }
}
```

**Prevention**:
- Monitor resource pool utilization
- Pre-allocate resources for critical queries
- Set resource timeout limits per query priority
- Implement resource fairness guarantees (FIFO)

---

### E2. Starvation Prevention (Resource Fairness)

**Symptom**:
- Some threads wait indefinitely for resources
- Max wait time exceeds 30s
- FIFO fairness violated

**Root Causes**:
- Lack of fairness in resource allocation
- High-priority queries starving low-priority ones
- Incorrect condition variable signaling

**Recovery Procedures**:
1. **Immediate**: Wake waiting threads in FIFO order
2. **Short-term**: Implement monotonic fairness counter
3. **Medium-term**: Add priority-level fairness
4. **Long-term**: Load balancing and admission control

**Example Code (FIFO enforcement)**:
```cpp
std::queue<ResourceRequest> waiting_queue_;

// In release: wake oldest waiting request (FIFO)
while (!waiting_queue_.empty() && hasResources()) {
    auto request = waiting_queue_.front();
    if (canSatisfy(request)) {
        waiting_queue_.pop();
        allocate(request);
        resource_available_.notify_one();  // Wake specific thread
    } else {
        break;  // Cannot satisfy, stop here
    }
}
```

**Prevention**:
- Use FIFO queue for resource requests
- Track request enqueue time
- Monitor thread wait times
- Alert if any thread waits >30s

---

## Category F: Cache Failures

### F1. Cache Corruption (Race Condition)

**Symptom**:
- Cache returns corrupted or inconsistent plans
- Query results differ between runs
- ThreadSanitizer reports data race

**Root Causes**:
- Unsynchronized concurrent access to cache
- Lost updates during LRU eviction
- Use-after-free on evicted entries

**Recovery Procedures**:
1. **Immediate**: Disable caching (cache_enabled = false)
2. **Short-term**: Investigate ThreadSanitizer report
3. **Medium-term**: Implement proper synchronization
4. **Long-term**: Continuous ThreadSanitizer testing in CI

**Prevention**:
- Use `std::shared_mutex` for read/write synchronization
- Hold locks during entire eviction + insert operation
- Run ThreadSanitizer on all cache operations
- Unit tests for concurrent cache access

---

### F2. Cache Hit Rate Degradation

**Symptom**:
- Cache hit ratio drops below 70%
- Query performance increases
- Memory usage increases (more cache misses)

**Root Causes**:
- Insufficient cache capacity
- High diversity in query patterns
- Cache invalidation too aggressive
- LRU eviction removing hot entries

**Recovery Procedures**:
1. **Immediate**: Monitor hit ratio trend
2. **Short-term**: Increase cache capacity (if memory available)
3. **Medium-term**: Analyze query pattern distribution
4. **Long-term**: Implement adaptive cache sizing

**Prevention**:
- Monitor cache hit ratio continuously
- Alert if hit ratio < 70% for 5 min window
- Right-size cache capacity based on patterns
- Implement cache warming for critical queries

---

## Monitoring & Alerting

### Recommended Metrics
1. Cost overflow frequency
2. Statistics staleness (max age)
3. Frontier overflow frequency
4. GPU availability and memory usage
5. Resource pool utilization and wait times
6. Cache hit ratio
7. Shard availability and RPC latency
8. Thread wait times and deadlock detection

### Alert Thresholds
- Cost overflow: >5 per minute → investigate optimizer
- Stale statistics: >1 hour → trigger refresh
- Frontier overflow: >10% of queries → reduce max_depth
- GPU memory exhausted: >5 per hour → scale GPU memory
- Shard offline: >1 shard → alert ops team
- Cache hit ratio: <70% for 5 min → increase cache size
- Resource wait: max >30s → scale resource pool

---

**END OF DOCUMENT**
