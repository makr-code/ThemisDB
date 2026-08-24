# Phase 2 Detailed Implementation Plan — Performance & Scalability Readiness

**Duration:** 6-8 weeks (after Phase 1A completion, parallel with Phases 3–5)  
**Owner:** Query Optimization Team (Team B)  
**Status:** 🟤 PENDING — Starts after Phase 1A (2026-08-12)  
**Target Completion:** 2026-09-30

---

## Objective

Deliver measurable query optimization across plan-cache efficiency, cost-model refinement, and resource pooling, with all improvements gated on Wave-7 non-regression proof and validated under production-scale load.

---

## Scope

### In Scope

1. **Plan-Cache Efficiency**
   - Cache hit-rate tracking (target ≥ 95% on repeat queries)
   - Memory footprint optimization (target < 100 MB for typical workload)
   - Cache invalidation strategy (when plans are invalidated, why, impact)

2. **Cost-Model Refinement**
   - Complex AQL query cardinality estimation improvements
   - Join order optimization (multi-table queries)
   - Index selection heuristics (rule-based + statistics-based)
   - p99 query latency target < 200 ms for complex multi-shard queries

3. **Resource Pooling & Load-Balancing**
   - Connection pooling tuning for cluster-wide throughput
   - Thread pool sizing optimization
   - Queue depth management
   - Load-balancing strategy validation (round-robin, least-connections, weighted)
   - Target: ≥ 2× throughput improvement vs. baseline

4. **Benchmark Infrastructure**
   - Plan-cache measurement tools
   - Cost-model profiling harness
   - Resource pooling efficiency benchmarks
   - Wave-7 regression baseline validation

### Out of Scope (Phase 2+)

- Disk I/O optimization (Wave 3+)
- Distributed query optimization across shards (Phase 3)
- Adaptive indexing (Phase 3+)
- ML-based query optimization (Phase 4+)

---

## Work Breakdown

### Milestone 1: Baseline & Measurement Infrastructure (Weeks 1-2)

**Owner:** Query Optimization Team

**Tasks:**

1. **Establish Plan-Cache Measurement Tool**
   - [ ] Create `benchmarks/plan_cache_efficiency/` directory
   - [ ] Implement `plan_cache_hit_rate_counter.h` (instrument cache lookups)
   - [ ] Create benchmark suite: `bench_plan_cache_hit_rate.cpp`
     - Query types: SELECT, JOIN, GROUP BY, aggregate, subquery
     - Repeat patterns: same query 100× (should hit cache)
     - Different queries: 100 unique queries (should miss cache)
     - Mixed patterns: realistic workload (repeat rate varies)
   - [ ] Measurement output: `{ query_class, hit_rate%, memory_mb, latency_p50_p99 }`
   - [ ] Target: ≥ 95% hit rate on repeat patterns

   **Definition of Done:**
   - ✅ Measurement tool integrated with query engine
   - ✅ Benchmarks compile and run
   - ✅ Hit-rate output validated against known patterns

2. **Establish Cost-Model Profiling Harness**
   - [ ] Create `benchmarks/cost_model/` directory
   - [ ] Implement `cost_model_profiler.h` (instrument cardinality estimation)
   - [ ] Create benchmark suite: `bench_cost_model_accuracy.cpp`
     - Simple queries: SELECT, JOIN 2 tables
     - Complex queries: JOIN 4+ tables, GROUP BY, HAVING
     - Subqueries: nested SELECT, CTEs
     - Measure: estimated vs. actual cardinality, error rate
   - [ ] Profile query planning time (target < 50 ms for complex queries)

   **Definition of Done:**
   - ✅ Profiler integrated with query planner
   - ✅ Cardinality estimation accuracy measured
   - ✅ Planning time baseline established

3. **Establish Resource Pooling Efficiency Benchmarks**
   - [ ] Create `benchmarks/resource_pooling/` directory
   - [ ] Implement connection pool efficiency benchmark: `bench_connection_pool.cpp`
     - Throughput under varying concurrency (1, 10, 100, 1000 clients)
     - Pool wait time (when pool exhausted, how long clients wait)
     - Memory usage (per connection overhead)
   - [ ] Implement thread pool efficiency benchmark: `bench_thread_pool.cpp`
     - Queue depth impact on latency
     - Worker thread count tuning
     - Task stealing / work distribution

   **Definition of Done:**
   - ✅ Connection pool benchmarks run
   - ✅ Thread pool benchmarks run
   - ✅ Throughput and latency baseline established

4. **Validate Wave-7 Regression Baseline**
   - [ ] Run Wave 7 suite on current `develop` HEAD
   - [ ] Archive baseline evidence: `benchmarks/wave7/BASELINE_PHASE2_2026-08-12.json`
   - [ ] Confirm all 6 gates (GATE-W7-01..06) PASS
   - [ ] Document any deviations from Phase 1A baseline

   **Definition of Done:**
   - ✅ Wave 7 baseline established
   - ✅ No new regressions vs. Phase 1A

---

### Milestone 2: Plan-Cache Optimization (Weeks 2-4)

**Owner:** Query Optimization Team

**Tasks:**

1. **Analyze Cache Hit-Rate Data**
   - [ ] Run plan-cache measurement benchmarks
   - [ ] Identify query patterns with low hit rates (< 90%)
   - [ ] Root cause analysis:
     - Parameterization issues (similar queries not recognized as same plan)
     - Cache eviction (working set too large for cache)
     - Query normalization (queries should be canonicalized before caching)

2. **Implement Plan-Cache Improvements**
   - [ ] Enhanced query normalization:
     - Whitespace normalization
     - Constant folding (e.g., `WHERE x=5 AND x=5` → `WHERE x=5`)
     - Predicate reordering (commutative operations)
   - [ ] Cache eviction policy tuning:
     - Current policy: (LRU / LFU / other?) — document
     - Evaluate alternative policies (if needed)
     - Cap cache size at 100 MB (configurable)
   - [ ] Parameterization improvements:
     - Recognize similar queries with different constants
     - Use prepared statement parameters where applicable

   **Tests:**
   - [ ] `test_plan_cache_normalization.cpp` (PC-01..PC-04)
     - PC-01: Whitespace normalization
     - PC-02: Constant folding
     - PC-03: Predicate reordering
     - PC-04: Cache hit after normalization

   - [ ] `test_plan_cache_eviction.cpp` (PC-05..PC-08)
     - PC-05: Cache eviction policy correctness
     - PC-06: Cache size stays under 100 MB
     - PC-07: Evicted plan can be re-cached
     - PC-08: LRU/LFU fairness

3. **Benchmarking & Regression Testing**
   - [ ] Re-run plan-cache measurement benchmarks
   - [ ] Target: ≥ 95% hit-rate on repeat patterns
   - [ ] Confirm Wave 7 gates still PASS
   - [ ] Measure latency improvement (query execution should be faster with cached plans)

   **Definition of Done:**
   - ✅ Plan-cache hit-rate ≥ 95%
   - ✅ PC-01..PC-08 tests passing
   - ✅ Wave 7 non-regression confirmed
   - ✅ 8+ new tests committed

---

### Milestone 3: Cost-Model Refinement (Weeks 4-6)

**Owner:** Query Optimization Team

**Tasks:**

1. **Analyze Cost-Model Accuracy**
   - [ ] Run cost-model profiler on benchmark queries
   - [ ] Measure cardinality estimation error:
     - Simple queries (2-table JOIN): target < 5% error
     - Complex queries (4+ tables, GROUP BY, subqueries): target < 20% error
   - [ ] Identify worst-performing query patterns
   - [ ] Root cause analysis:
     - Missing statistics (table/index cardinality, column distributions)
     - Join selectivity estimation (errors propagate through multi-table plans)
     - Subquery cardinality underestimation

2. **Implement Cost-Model Improvements**
   - [ ] Enhance table/column statistics collection:
     - Cardinality per table
     - Distinct value counts per column
     - NULL distribution per column
     - (Optional) Histogram for skewed columns
   - [ ] Improve join selectivity estimation:
     - Use column correlations if available
     - Fall back to conservative estimates (e.g., assume independence)
   - [ ] Subquery cost estimation:
     - Recognize correlated subqueries vs. uncorrelated
     - Cost of materialization vs. semi-join

   **Tests:**
   - [ ] `test_cost_model_statistics.cpp` (CM-01..CM-04)
     - CM-01: Table cardinality accuracy
     - CM-02: Column distinct value counts
     - CM-03: NULL distribution tracking
     - CM-04: Statistics update after INSERT/UPDATE/DELETE

   - [ ] `test_cost_model_join_selectivity.cpp` (CM-05..CM-08)
     - CM-05: 2-table JOIN selectivity (< 5% error)
     - CM-06: 4-table JOIN selectivity (< 20% error)
     - CM-07: Skewed column handling
     - CM-08: Missing statistics fallback (conservative)

   - [ ] `test_cost_model_subquery.cpp` (CM-09..CM-12)
     - CM-09: Correlated subquery recognition
     - CM-10: Uncorrelated subquery materialization cost
     - CM-11: Semi-join estimation
     - CM-12: Subquery cardinality accuracy

3. **Join Order Optimization**
   - [ ] Implement dynamic programming join order enumeration (if not already present)
     - Prune search space (only consider promising join orders)
     - Estimated cost drives join order decisions
   - [ ] Measure join order quality:
     - Compare estimated join order cost vs. actual execution
     - Are cheaper join orders actually faster?

   **Tests:**
   - [ ] `test_join_order_optimization.cpp` (JO-01..JO-04)
     - JO-01: 3-way JOIN order optimization
     - JO-02: 5-way JOIN order optimization
     - JO-03: Join order estimate accuracy
     - JO-04: Cost-driven join order selection

4. **Query Latency Measurement & Tuning**
   - [ ] Run complex query benchmarks:
     - Multi-table JOINs (4-10 tables)
     - GROUP BY with aggregates
     - Subqueries
     - HAVING clauses
   - [ ] Measure p50, p95, p99 latencies
   - [ ] Target: p99 < 200 ms for complex multi-shard queries

   **Definition of Done:**
   - ✅ Cardinality estimation error < 5% (simple), < 20% (complex)
   - ✅ CM-01..CM-12 tests passing
   - ✅ JO-01..JO-04 tests passing
   - ✅ p99 query latency < 200 ms confirmed
   - ✅ Wave 7 non-regression confirmed
   - ✅ 16+ new tests committed

---

### Milestone 4: Resource Pooling & Load-Balancing (Weeks 6-7)

**Owner:** Query Optimization Team

**Tasks:**

1. **Connection Pool Tuning**
   - [ ] Measure current connection pool overhead:
     - Per-connection memory usage
     - Connection creation/recycling time
     - Pool wait time under high concurrency
   - [ ] Optimize pool parameters:
     - Minimum pool size (pre-allocated connections)
     - Maximum pool size (total available)
     - Connection timeout (when to close idle connections)
     - Idle connection cleanup threshold
   - [ ] Implement pool monitoring:
     - Current pool size metric
     - Queue wait time metric
     - Connection recycle rate metric

   **Tests:**
   - [ ] `test_connection_pool_efficiency.cpp` (CP-01..CP-04)
     - CP-01: Pool creation and initialization
     - CP-02: Connection allocation/recycling
     - CP-03: Pool queue wait time under high concurrency (100+ clients)
     - CP-04: Connection timeout + cleanup

2. **Thread Pool Tuning**
   - [ ] Measure current thread pool performance:
     - Queue depth impact on task latency
     - Worker thread count utilization
     - Task throughput (tasks/sec)
   - [ ] Optimize thread pool parameters:
     - Worker thread count (tune to CPU core count)
     - Queue depth limit (prevent unbounded growth)
     - Task priority (if applicable)
   - [ ] Implement work-stealing or load-balancing strategy:
     - Redistribute tasks if one queue backlog grows large
     - Measure fairness (all workers busy, no starvation)

   **Tests:**
   - [ ] `test_thread_pool_efficiency.cpp` (TP-01..TP-04)
     - TP-01: Thread pool initialization
     - TP-02: Task queue depth impact on latency
     - TP-03: Worker thread utilization
     - TP-04: Task throughput under load

3. **Load-Balancing Strategy Validation**
   - [ ] Measure current load-balancing strategy:
     - Round-robin: distribute queries evenly
     - Least-connections: send to server with fewest active connections
     - Weighted: prioritize less-loaded servers
   - [ ] Test under realistic cluster load:
     - Multiple backend nodes (3–10)
     - Varying query complexity
     - Varying backend response times
   - [ ] Measure load distribution fairness (std dev of per-node query count)

   **Tests:**
   - [ ] `test_load_balancing_strategy.cpp` (LB-01..LB-04)
     - LB-01: Round-robin distribution (requests per node = total / num_nodes ± 1)
     - LB-02: Least-connections strategy (uniform active connection count)
     - LB-03: Load balancing under varying backend latencies
     - LB-04: Failover behavior (skip failed node, redistribute)

4. **Throughput & Latency Validation**
   - [ ] Run resource pooling benchmarks with optimized parameters
   - [ ] Measure throughput improvement (target: ≥ 2× vs. baseline)
   - [ ] Measure latency improvement under high concurrency
   - [ ] Confirm Wave 7 gates still PASS

   **Definition of Done:**
   - ✅ Connection pool optimized (per-connection overhead < 1 MB)
   - ✅ Thread pool optimized (queue depth < 100, worker utilization > 90%)
   - ✅ Load-balancing fairness confirmed (std dev < 5%)
   - ✅ CP-01..CP-04, TP-01..TP-04, LB-01..LB-04 tests passing
   - ✅ Throughput improvement ≥ 2× vs. baseline
   - ✅ Wave 7 non-regression confirmed
   - ✅ 12+ new tests committed

---

### Milestone 5: Integration Testing & Final Validation (Weeks 7-8)

**Owner:** Query Optimization Team + QA

**Tasks:**

1. **End-to-End Integration Testing**
   - [ ] Run complex mixed workload:
     - Concurrent queries (100+ clients)
     - Varying complexity (simple SELECTs, complex JOINs, aggregates)
     - Plan-cache hits + misses
     - Resource pooling under pressure
   - [ ] Measure end-to-end latency, throughput, resource usage
   - [ ] Verify no performance regressions

   **Tests:**
   - [ ] `test_phase2_integration_mixed_workload.cpp` (INT-01..INT-04)
     - INT-01: 100 concurrent clients, mixed query types
     - INT-02: Plan-cache effectiveness under realistic workload
     - INT-03: Resource pooling stability (no connection leaks, thread leaks)
     - INT-04: Load-balancing fairness under mixed workload

2. **Wave-7 Final Regression Validation**
   - [ ] Run full Wave 7 suite on optimized code
   - [ ] All 6 gates (GATE-W7-01..06) must still PASS
   - [ ] Archive final evidence: `benchmarks/wave7/FINAL_EVIDENCE_PHASE2_2026-09-30.json`
   - [ ] Compare against Phase 2 baseline (Milestone 1):
     - No new regressions
     - Ideally, some improvements (cached query latency should be faster)

3. **Code Review & Documentation**
   - [ ] Peer review of all Phase 2 commits
   - [ ] Verify code follows existing style/patterns
   - [ ] Update relevant architecture documentation:
     - `docs/architecture/query_optimization.md` (new or updated)
     - Document plan-cache strategy, cost-model, pooling tuning
   - [ ] Add tuning guide: `docs/operations/QUERY_TUNING_GUIDE.md`
     - How to monitor plan-cache hit-rate
     - How to adjust pool sizes
     - How to profile slow queries

   **Definition of Done:**
   - ✅ INT-01..INT-04 tests passing
   - ✅ Wave 7 gates all PASS (no regression)
   - ✅ Code review approved by 2+ reviewers
   - ✅ Documentation updated
   - ✅ All Phase 2 commits merged to develop

---

## Test Summary

**Total New Tests:** 130+ (breakdown below)

| Test Suite | Count | Coverage |
|------------|-------|----------|
| test_plan_cache_*.cpp | 8 | Normalization, eviction, hit-rate |
| test_cost_model_*.cpp | 12 | Statistics, join selectivity, subquery |
| test_join_order_optimization.cpp | 4 | Join order enumeration, cost accuracy |
| test_connection_pool_efficiency.cpp | 4 | Pool allocation, queue, timeout |
| test_thread_pool_efficiency.cpp | 4 | Thread pool, queue depth, utilization |
| test_load_balancing_strategy.cpp | 4 | Round-robin, least-conn, weighted |
| test_phase2_integration_mixed_workload.cpp | 4 | End-to-end integration, stability |
| Additional unit/integration tests | 86+ | Various edge cases and scenarios |

**Acceptance Criteria:**
- [x] All 130+ tests passing
- [x] Code coverage > 80% for optimized code paths
- [x] TIMEOUT 300 for long-running benchmarks

---

## Benchmark Summary

**New Benchmark Suites:**

| Benchmark | Metrics | Target | Owner |
|-----------|---------|--------|-------|
| bench_plan_cache_hit_rate | Hit-rate %, memory, latency | ≥ 95% hit rate | Query Team |
| bench_cost_model_accuracy | Cardinality error % | < 5% (simple), < 20% (complex) | Query Team |
| bench_connection_pool | Throughput, wait time | 2× vs baseline | Query Team |
| bench_thread_pool | Task throughput, latency | Task latency < 100 µs | Query Team |
| bench_load_balancing | Distribution fairness | Std dev < 5% | Query Team |
| bench_phase2_mixed_workload | End-to-end throughput, latency | p99 < 200 ms | QA |

---

## Acceptance Criteria (Phase 2 Gate)

**All must pass for Phase 2 to be considered complete:**

- [x] All 130+ tests passing
- [x] Cache-hit-rate ≥ 95% on repeat query patterns
- [x] Cardinality estimation error < 5% (simple queries), < 20% (complex queries)
- [x] p99 query latency < 200 ms for complex multi-shard queries
- [x] Resource pooling throughput ≥ 2× vs. baseline
- [x] Wave 7 gates (GATE-W7-01..06) all PASS (no regression)
- [x] Code review approved
- [x] Documentation updated (architecture + tuning guide)
- [x] All commits merged to develop

---

## Risk Register (Phase 2)

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| Plan-cache hit-rate stuck < 95% | Medium | Latency improvement limited | Revisit normalization strategy; investigate query patterns |
| Cost-model error > 20% for complex queries | Medium | Join order suboptimal | Collect more statistics; use conservative estimates |
| Wave 7 regression detected | Low | Gate fails; Phase 2 blocked | Identify root cause; revert problematic optimization |
| Resource pooling overhead exceeds baseline | Low | No improvement | Reduce pool size; optimize allocation strategy |
| Integration tests timeout | Medium | Phase 2 blocked | Reduce test load; increase TIMEOUT; investigate bottleneck |

---

## Timeline Summary

| Week | Milestone | Deliverable |
|------|-----------|-------------|
| 1-2 | Baseline & Infrastructure | Measurement tools, Wave 7 baseline |
| 2-4 | Plan-Cache Optimization | 8 new tests, ≥ 95% hit-rate |
| 4-6 | Cost-Model Refinement | 16+ new tests, < 5% cardinality error |
| 6-7 | Resource Pooling | 12+ new tests, ≥ 2× throughput |
| 7-8 | Integration & Validation | 130+ total tests, Wave 7 PASS, docs updated |

**Total Duration:** 6-8 weeks (after Phase 1A)  
**Target Completion:** 2026-09-30

---

## Next Steps (After Phase 2)

1. Merge all Phase 2 work to develop
2. Archive Wave 7 final evidence
3. Coordinate with Phase 3/4/5 progress checks
4. (No community release promotion until all phases complete)

---

## References

- `ROADMAP.md` — Phase 2 definition
- `docs/architecture/query_optimization.md` — (to be updated)
- `benchmarks/wave7/` — Wave 7 regression gate definition
- `CMakeLists.txt` for benchmark/test registration
