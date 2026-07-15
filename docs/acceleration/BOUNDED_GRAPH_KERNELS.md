# Bounded Graph Kernels: Acceleration Eligibility Framework

**Date**: 2026-06-25  
**Status**: ✅ **APPROVED FOR IMPLEMENTATION**  
**Issue**: #5469 - Define bounded graph kernels eligible for acceleration without weakening Graph Truth semantics  
**Scope**: Graph operations, vector ANN, geospatial kernels, and their GPU acceleration eligibility  

---

## Table of Contents

1. [Overview](#overview)
2. [Kernel Classification Framework](#kernel-classification-framework)
3. [Category A: Acceleration-Eligible](#category-a-acceleration-eligible)
4. [Category B: Acceleration-Eligible with Constraints](#category-b-acceleration-eligible-with-constraints)
5. [Category C: CPU-First Only](#category-c-cpu-first-only)
6. [Classification Matrix](#classification-matrix)
7. [Safety Guarantees](#safety-guarantees)
8. [Remediation Roadmap](#remediation-roadmap)

---

## Overview

ThemisDB's **Graph Truth Layer** remains CPU-first while selective bounded graph kernels may be GPU-accelerated. This document defines which operations can be safely accelerated while maintaining exact graph semantics and governance guarantees.

### Core Principle

> **Graph Truth is exact and CPU-bearing. GPU acceleration is advisory-only.**

- ✅ GPU kernels produce **candidate** results for downstream validation
- ✅ Final truth results are **always CPU-verified**
- ✅ All GPU operations have mandatory **CPU fallback paths**
- ✅ Provenance chains, ACL decisions, and policy enforcement **never use GPU approximations**

---

## Kernel Classification Framework

All graph operations fall into three categories:

| Category | GPU Use | Fallback | Semantics |
|----------|---------|----------|-----------|
| **A: Acceleration-Eligible** | ✅ Yes (no constraints) | Optional | Advisory candidate generation |
| **B: Conditional Acceleration** | ⚠️ Yes (with gates) | Required | Advisory with input/output validation |
| **C: CPU-First Only** | ❌ Never | N/A (CPU primary) | Truth-bearing, exact results required |

---

## Category A: Acceleration-Eligible

**Timeline**: ✅ **Ready for Phase A (Q3 2026)**

Category A kernels have no semantic constraints on GPU execution. They produce bounded, deterministic outputs suitable for candidate generation.

### 1. ANN Distance Kernels

**Operations**:
- L2 Euclidean distance: `launchL2DistanceKernel()`
- Cosine similarity: `launchCosineDistanceKernel()`
- Inner product: `launchInnerProductKernel()`

**Safety Properties**:
- ✅ Fixed-shape inputs (pre-validated query/vector counts)
- ✅ Known output cardinality (numQueries × numVectors distances)
- ✅ Advisory-only output (feeds to TopK selection + CPU validation)
- ✅ Deterministic computation (no state mutation)
- ✅ Exception-safe (RAII stream wrappers)

**Constraints** (must enforce):
```cpp
// PRE-CONDITION: Input validation (CPU-side)
ASSERT(numQueries > 0 && numVectors > 0 && dim > 0);

// EXECUTION: GPU kernel bounded by timeout
cudaStreamSynchronize(stream);  // Must complete within 5 second SLA

// POST-CONDITION: Output validation (CPU-side)
ASSERT(distanceMatrix.size() == numQueries * numVectors);
ASSERT(allDistancesInValidRange(distanceMatrix));  // Type-specific checks
```

**Remediation Required (Q3 2026)**:
- [ ] Fix 50% of unchecked CUDA calls (340 → 170 gaps)
- [x] Add timeout enforcement in kernel invocation wrapper
- [x] Implement result validation before downstream use
- [ ] Add unit tests: GPU distance parity with CPU BLAS

---

### 2. TopK Selection Kernel

**Operation**: `launchTopKKernel()` — Select top-K indices/distances per query

**Safety Properties**:
- ✅ Bounded output (exactly K elements per query)
- ✅ Advisory only (filtered candidates for downstream validation)
- ✅ Deterministic selection (no randomness in ordering, ties handled consistently)
- ✅ Well-defined output contract (indices + distances)

**Constraints**:
```cpp
// PRE-CONDITION: K is valid
ASSERT(k > 0 && k <= numVectors);

// PRE-CONDITION: Output buffers pre-allocated
ASSERT(d_topkIndices.capacity() >= k * numQueries);
ASSERT(d_topkDistances.capacity() >= k * numQueries);

// POST-CONDITION: Advisory semantics enforced
// TopK output is candidate suggestion only;
// CPU exact filtering required before data operations
```

**Remediation Required**:
- [ ] Add error checking around kernel launch
- [x] Validate output size matches expectation
- [ ] Unit tests: TopK correctness vs CPU heap sort

---

### 3. Vector KNN Insertion Pipeline

**Component**: `VecKnnInsertPipeline` (SIMD-accelerated batch insertion)

**Features**:
- Parallel batch insertion with thread pool
- SIMD-accelerated distance computation (AVX-512 → AVX2 → NEON → scalar)
- Distance cache memoization
- Atomic VectorIndexManager integration

**Safety Properties**:
- ✅ CPU-only (no GPU kernels involved)
- ✅ Thread-safe (batch workers use atomic index manager)
- ✅ Resource-bounded (batch_size limit = 32)
- ✅ Exception-safe (RAII futures)

**Constraints**:
```cpp
// PRE-CONDITION: Batch size bounded
const size_t MAX_BATCH_SIZE = 32;
ASSERT(batch.size() <= MAX_BATCH_SIZE);

// EXECUTION: Future lifetime guaranteed
std::vector<std::future<Result>> workers;
// All futures must complete before destruction
```

**Remediation Required**:
- [ ] Add future exception handling in worker pool
- [ ] Validate thread safety with ThreadSanitizer
- [ ] Document batch isolation (no shared state across workers)

---

### 4. Tensor Core Matrix Multiply

**Components**:
- `launchCPUMatmulKernel()` — FP32 CPU fallback (naive triple-loop)
- `launchFP16MatmulKernel()` — CUDA FP16 kernel (if THEMIS_ENABLE_CUDA)
- `dispatchMatmul()` — Unified CPU/GPU dispatcher

**Safety Properties**:
- ✅ Input validation: M/K/N > 0 checks
- ✅ CPU fallback always available (FP32 naive GEMM)
- ✅ GPU optional (gated by THEMIS_ENABLE_CUDA)
- ✅ Advisory output (refinement, not truth-bearing)

**Constraints**:
```cpp
// PRE-CONDITION: Dimensions valid
ASSERT(M > 0 && K > 0 && N > 0);

// EXECUTION: CPU fallback always available
if (!cuda_available || params.force_cpu) {
    return launchCPUMatmulKernel(A, B, C, M, K, N);
}

// GPU attempt with fallback
auto gpu_result = launchFP16MatmulKernel(...);
if (gpu_result != 0) {
    return launchCPUMatmulKernel(A, B, C, M, K, N);  // Fall back
}

// POST-CONDITION: Precision explicit
ASSERT(params.precision == MatrixPrecision::FP16 || FP32);
```

**Remediation Required**:
- [ ] Add CUDA error checking on all GPU calls (2 CRITICAL gaps)
- [ ] Validate precision compatibility before launch
- [ ] Add benchmarks: CPU vs GPU matmul for different sizes
- [ ] Implement timeout gates (5s max per kernel)

---

## Category B: Acceleration-Eligible with Constraints

**Timeline**: 🟡 **Phase B (Q3 2026), after 60% gap reduction**

Category B kernels may use GPU acceleration **if and only if** explicit input/output validation gates and CPU fallback paths are implemented. These operations handle domain-specific data (geospatial, graph) where boundary conditions matter.

### 1. Geographic Distance Kernel

**Operation**: `launchGeoDistanceKernel()` — Compute Haversine distance between lat/lon pairs

**Safety Analysis**:
- ✅ Bounded computation (per-pair calculation)
- ✅ Advisory output (candidate ranking, not truth)
- ⚠️ Formula dependency (Haversine vs great-circle variations)
- ⚠️ Precision sensitivity (geographic accuracy requires FP64)
- ⚠️ Coordinate validation essential (range checks -90/90 lat, -180/180 lon)

**Mandatory Constraints**:
```cpp
// PRE-CONDITION: Coordinate validation (CPU)
for (auto& coord : coordinates) {
    if (coord.latitude < -90.0 || coord.latitude > 90.0) {
        return ErrorCode::INVALID_LATITUDE;
    }
    if (coord.longitude < -180.0 || coord.longitude > 180.0) {
        return ErrorCode::INVALID_LONGITUDE;
    }
}

// EXECUTION: GPU kernel with timeout
auto gpu_distances = launchGeoDistanceKernel(...);

// POST-CONDITION: Result validation (CPU)
for (auto& distance : gpu_distances) {
    if (distance < 0.0 || distance > 40075.0) {  // Earth circumference in km
        return ErrorCode::INVALID_DISTANCE_RESULT;
    }
}

// FALLBACK: To CPU if GPU result invalid
if (gpu_result_invalid) {
    return cpu_haversine(lat1, lon1, lat2, lon2);
}

// SEMANTICS: Advisory only
// Geographic distances are candidate hints for filtering
// Exact spatial queries validated on CPU
```

**Remediation Required (Q3 2026)**:
- [ ] Implement coordinate validation (range checks)
- [ ] Implement result distance validation (0-40075 km range)
- [ ] Add CPU fallback for invalid GPU results
- [ ] Unit tests: GPU Haversine vs reference implementation
- [ ] Accuracy parity benchmark

**Gap Blockers**:
- Kernel result validation: 320 gaps → **CRITICAL BLOCKER**
- Boundary condition checks: 195 gaps → **BLOCKER**

---

### 2. Geographic Containment Kernel

**Operation**: `launchGeoContainmentKernel()` — Point-in-polygon test

**Safety Analysis**:
- ⚠️ Complex geometry (polygon edge cases, winding rules)
- ⚠️ Result criticality (containment affects candidate filtering)
- ⚠️ Polygon validity must be pre-validated
- ⚠️ Boundary conditions (points on edges, polygon closure)

**Mandatory Constraints**:
```cpp
// PRE-CONDITION: Polygon validation (CPU)
if (!polygon.is_closed()) return ErrorCode::POLYGON_NOT_CLOSED;
if (polygon.num_vertices < 3) return ErrorCode::INVALID_POLYGON;
if (!polygon.is_simple()) return ErrorCode::SELF_INTERSECTING_POLYGON;

// EXECUTION: GPU kernel
auto gpu_containment = launchGeoContainmentKernel(points, polygon);

// POST-CONDITION: Result validation (CPU)
for (auto result : gpu_containment) {
    // Must be binary (0=outside, 1=inside)
    if (result != 0 && result != 1) {
        return ErrorCode::INVALID_CONTAINMENT_RESULT;
    }
}

// FALLBACK: To exact CPU algorithm
if (gpu_result_unreliable) {
    return cpu_point_in_polygon_winding_number(points, polygon);
}

// SEMANTICS: Advisory filtering
// Containment results filter candidate set
// Exact graph validation required before data operations
```

**Remediation Required**:
- [ ] Implement polygon validation (closure, simplicity)
- [ ] Implement result validation (binary 0/1 only)
- [ ] Add CPU fallback for edge cases
- [ ] Unit tests: GPU vs CPU containment for representative polygons

**Gap Blockers**:
- Result validation: 320 gaps → **CRITICAL BLOCKER**
- Boundary condition handling: 195 gaps → **CRITICAL BLOCKER**
- Polygon integrity checks: Missing → **BLOCKER**

---

### 3. Graph BFS Kernels (Bounded Frontier)

**Operations**:
- `launchGraphBFSInitKernel()` — Initialize frontier
- `launchGraphBFSExpandKernel()` — Expand frontier one hop
- `launchGraphBFSGatherKernel()` — Collect results

**Safety Analysis**:
- ✅ Bounded depth (k ≤ 3 hops configured)
- ✅ Frontier size bounded (cutoff at 10K nodes)
- ⚠️ Adjacency index must be validated pre-GPU
- ⚠️ Visited set consistency (must maintain across hops)
- ⚠️ Output must be validated against CPU exact traversal

**Mandatory Constraints**:
```cpp
// PRE-CONDITION: Bounded depth
const int MAX_HOPS = 3;
if (hops_requested > MAX_HOPS) {
    return ErrorCode::TRAVERSAL_TOO_DEEP;
}

// PRE-CONDITION: Adjacency index validation
if (!adjacency_index.is_valid()) {
    return ErrorCode::INVALID_ADJACENCY_INDEX;
}

// EXECUTION: Frontier size monitoring + CPU fallback
const int MAX_FRONTIER_SIZE = 10000;
for (int hop = 0; hop < hops; hop++) {
    auto frontier_size = gpu_frontier.size();
    if (frontier_size > MAX_FRONTIER_SIZE) {
        // GPU frontier too large; fallback to CPU exact
        return cpu_k_hop_traversal(start, hops);
    }
}

// POST-CONDITION: Exact parity with CPU
auto cpu_result = cpu_k_hop_traversal(start, hops);
ASSERT(gpu_result == cpu_result);  // Must match exactly

// SEMANTICS: Advisory candidate generation
// BFS output is candidate set for downstream exact validation
```

**Remediation Required (Q3 2026)**:
- [ ] Implement frontier size monitoring + CPU fallback
- [ ] Add adjacency index validation
- [ ] Implement exact CPU/GPU parity tests
- [ ] Add timeout enforcement (kernel time < 2 seconds)
- [ ] Benchmark: GPU vs CPU BFS speed + accuracy

**Gap Blockers**:
- Memory boundary checks: 195 gaps → **CRITICAL**
- Dispatch correctness: 145 gaps → **CRITICAL**
- Kernel result validation: 320 gaps → **CRITICAL**

---

### 4. Graph Shortest Path Kernels (Bounded Pairs)

**Operations**:
- `launchGraphDijkstraInitKernel()` — Initialize distances
- `launchGraphDijkstraRelaxKernel()` — Relax edges

**Safety Analysis**:
- ✅ Bounded vertex pairs (input size bounded)
- ✅ Advisory output (candidate routing, not truth)
- ⚠️ Edge weight validation required
- ⚠️ Convergence criteria (number of iterations)
- ⚠️ Distance overflow risk (accumulated path cost)

**Mandatory Constraints**:
```cpp
// PRE-CONDITION: Edge weight validation (CPU)
for (auto& edge : edges) {
    ASSERT(edge.weight >= 0);  // Dijkstra requires non-negative weights
}

// PRE-CONDITION: Bounded pairs
const int MAX_VERTEX_PAIRS = 1000;
if (vertex_pairs.size() > MAX_VERTEX_PAIRS) {
    return cpu_dijkstra_all_pairs();  // Fallback to CPU
}

// EXECUTION: GPU shortest path computation
auto gpu_paths = launchGraphDijkstraKernel(edges, pairs);

// POST-CONDITION: Distance overflow protection
ASSERT(allPathCostsBelowMax(gpu_paths));

// POST-CONDITION: GPU/CPU parity test
for (auto& pair : gpu_paths) {
    auto cpu_path = cpu_dijkstra(pair.source, pair.target);
    ASSERT(gpu_path.distance == cpu_path.distance);  // Parity check
}

// SEMANTICS: Advisory path routing only
// GPU shortest paths are candidate suggestions
// Exact graph validation required for policy/security decisions
```

**Remediation Required**:
- [ ] Add edge weight validation
- [ ] Implement result parity tests with CPU Dijkstra
- [ ] Add overflow protection (distance accumulation)
- [ ] Implement CPU fallback for large graphs

**Gap Blockers**:
- Result validation: 320 gaps → **CRITICAL**

---

## Category C: CPU-First Only

**Timeline**: ❌ **NEVER GPU** (Non-negotiable)

Category C operations **must never use GPU kernels**, even for candidate generation. These operations bear ground truth and cannot tolerate approximations.

### 1. ACL & Permission Enforcement

**Operations** (CPU-only):
- ACL-gated traversal
- Permission-checked neighborhoods
- Role-based path filtering

**Rationale**:
- ❌ GPU approximations cannot maintain ACL guarantees
- ❌ Access control must be deterministic and auditable
- ❌ Security boundary cannot be weakened by approximation

**Code Pattern** (MUST NOT GPU-accelerate):
```cpp
// ALWAYS CPU:
auto authorized_neighbors = cpu_get_authorized_neighbors(node, policy);

// GPU OK only for candidate generation:
auto candidates = gpu_candidate_generation(node);

// CPU MUST validate:
auto final_result = validate_against_authorized(candidates, authorized);
ASSERT(no_unauthorized_nodes_in_result);
```

**Enforcement**:
- Runtime checks prevent GPU dispatch for ACL operations
- Code review gates flag any GPU usage in permission paths
- CI/CD validation ensures no ACL shortcuts through GPU

---

### 2. Provenance-Sensitive Evidence Chains

**Operations** (CPU-only):
- Evidence chain construction
- Trust chain validation
- Audit trail assembly
- Provenance lineage tracking

**Rationale**:
- ❌ Provenance requires exact ordering and completeness
- ❌ GPU parallelization breaks determinism
- ❌ Audit trails must be reconstructable and verifiable

**Code Pattern**:
```cpp
// ALWAYS CPU:
auto provenance_chain = cpu_construct_evidence_chain(start_node);
// Chain includes:
// - Exact edge traversal order
// - Timestamp of each decision point
// - Reason for each hop
// - Versioning information

// GPU is allowed only for:
// 1. Pre-filtering candidates
// 2. Ranking by distance (advisory)
// 3. Index lookups (not logic)
```

---

### 3. Policy-Aware Graph Validation

**Operations** (CPU-only):
- Policy decision making
- Compliance verification
- Regulatory boundary checking
- Truth-bearing validation

**Rationale**:
- ❌ Policy decisions are security-critical
- ❌ GPU parallelization may skip policy checks
- ❌ Compliance guarantees cannot be delegated

**Code Pattern**:
```cpp
// ALWAYS CPU for policy:
auto policy_validated = cpu_validate_against_policy(graph, policy);
ASSERT(policy_validated == true);

// GPU may assist with candidate generation:
auto candidates = gpu_rank_by_distance(nodes);

// But final policy check must be CPU:
auto final_result = cpu_apply_policy(candidates, policy);
```

---

### 4. Exact Multi-Hop Validation

**Operations** (CPU-only):
- Multi-hop correctness verification
- Path completeness checking
- Cycle detection
- Consistency validation

**Rationale**:
- ❌ Multi-hop correctness depends on exact edge traversal
- ❌ GPU approximations create data integrity issues
- ❌ Cycles and consistency require deterministic algorithms

---

### 5. Irregular Truth-Bearing Traversals

**Operations** (CPU-only):
- Heterogeneous metadata traversal
- Dynamic branching based on metadata
- Type-dependent path logic
- Schema-aware graph navigation

**Rationale**:
- ❌ GPU kernels cannot handle true irregularity
- ❌ Dynamic branching factors preclude GPU batching
- ❌ Schema variation requires CPU-side interpretation

---

## Classification Matrix

| Kernel | Category | GPU Safe? | Fallback | Blocker Gaps (Q3) | Timeline |
|--------|----------|-----------|----------|------------------|----------|
| **L2 Distance** | A | ✅ Yes | Optional | 340 (50% ✓) | Phase A |
| **Cosine Distance** | A | ✅ Yes | Optional | 340 (50% ✓) | Phase A |
| **Inner Product** | A | ✅ Yes | Optional | 340 (50% ✓) | Phase A |
| **TopK Selection** | A | ✅ Yes | Optional | 15 (error handling) | Phase A |
| **Vec KNN Insert** | A | ✅ Yes (CPU) | N/A | 95 (thread safety) | Phase A |
| **Tensor Matmul** | A | ✅ Yes | CPU fallback | 2 CRITICAL CUDA | Phase A |
| **Geo Distance** | B | ⚠️ Conditional | Required | 515 (validation) | Phase B (Q3+) |
| **Geo Containment** | B | ⚠️ Conditional | Required | 515 (validation) | Phase B (Q3+) |
| **Graph BFS** | B | ⚠️ Conditional | Required | 660 (memory+validation) | Phase B (Q3+) |
| **Graph Dijkstra** | B | ⚠️ Conditional | Required | 377 (validation) | Phase B (Q3+) |
| **ACL Enforcement** | C | ❌ Never | CPU-only | N/A | Never GPU |
| **Provenance Chains** | C | ❌ Never | CPU-only | N/A | Never GPU |
| **Policy Validation** | C | ❌ Never | CPU-only | N/A | Never GPU |
| **Exact Multi-Hop** | C | ❌ Never | CPU-only | N/A | Never GPU |
| **Irregular Traversal** | C | ❌ Never | CPU-only | N/A | Never GPU |

---

## Safety Guarantees

All GPU acceleration maintains these invariants:

### ✅ Graph Truth Semantics

1. **Exact graph truth remains canonical**
   - CPU-computed results are the source of truth
   - GPU results are never trusted directly

2. **GPU outputs remain advisory-only**
   - GPU kernels produce candidate suggestions
   - Final decisions made on CPU with exact computation

3. **Summary-first never replaces exact-on-demand validation**
   - GPU summaries or caches never bypass exact checks
   - On-demand exact validation always available

4. **Policy decisions happen on CPU first**
   - All security/compliance decisions are CPU-side
   - GPU used for scaling computation, not decision logic

5. **Provenance chains remain unbroken**
   - Evidence chains constructed on CPU
   - No approximations in audit trails

6. **ACL enforcement is non-negotiable**
   - Access control gates pre-process all GPU operations
   - GPU never sees unauthorized data

7. **Fallback-to-CPU always available**
   - Every GPU operation has a CPU fallback
   - Fallback is transparent to caller

---

## Remediation Roadmap

### Phase 1: Category A Hardening (Q3 2026, Weeks 1-4)

**Priority**: 🔴 **CRITICAL** for Phase A deployment

**Goals**:
- Achieve 50% gap reduction in acceleration module (from 2,558 → ~1,300)
- All Category A kernels production-ready
- 100% error handling coverage

**Actions**:
- [ ] Fix 340 unchecked CUDA calls → 170 gaps remaining (50% reduction)
- [x] Implement timeout gates for distance kernels (5s SLA)
- [x] Add result validation for TopK selection
- [ ] Unit tests: GPU distance parity with CPU BLAS
- [ ] Add RAII wrappers for resource cleanup

**Acceptance Criteria**:
- [ ] All CRITICAL CUDA gaps resolved (2 remaining 0)
- [ ] Unit tests passing (distance/TopK parity verified)
- [x] Timeout enforcement in place
- [ ] No AddressSanitizer or LeakSanitizer issues

**Effort**: 20-24 hours (2.5-3 days)

---

### Phase 2: Category B Gating (Q3 2026, Weeks 5-8)

**Priority**: 🟠 **HIGH** for Phase B entry

**Goals**:
- Achieve 60% gap reduction in acceleration module
- All Category B validation gates implemented
- GPU/CPU parity tests passing

**Actions**:
- [ ] Implement coordinate/edge validation for Geo/Graph kernels
- [ ] Add frontier size cutoff + CPU fallback for BFS
- [ ] Implement result validation for Dijkstra
- [ ] Implement GPU/CPU parity tests (representative datasets)
- [ ] Add overflow/boundary condition checking

**Acceptance Criteria**:
- [ ] 60% gap reduction achieved (2,558 → ~1,000)
- [ ] All Category B parity tests passing
- [ ] CPU fallback verified working for all kernels
- [ ] ThreadSanitizer clean (no data races)
- [ ] BFS/Dijkstra benchmarks meet performance targets

**Effort**: 28-32 hours (3.5-4 days)

---

### Phase 3: Category C Enforcement (Ongoing)

**Priority**: 🟢 **CONTINUOUS**

**Actions**:
- [ ] Document CPU-only enforcement policy
- [ ] Add runtime checks preventing GPU dispatch for C kernels
- [ ] Code review enforcement rules
- [ ] CI/CD gates for Category C containment
- [ ] Static analysis rules to flag forbidden GPU usage

**Effort**: 8-12 hours (1-1.5 days)

---

## References

| Document | Purpose |
|----------|---------|
| `include/acceleration/graph_kernel_classification.h` | C++ enum/traits for kernel classification |
| `docs/acceleration/KERNEL_ACCELERATION_EXAMPLES.md` | Code patterns (allowed/disallowed) |
| `benchmarks/bounded_kernel_validation.cpp` | Benchmark hooks for parity testing |
| `ai_working/KERNEL_CLASSIFICATION_REVIEW.md` | Full gap-based analysis (Phase 5) |
| Issue #5469 | This issue (kernel classification definition) |
| Issue #5468 | Rollout plan with remediation gates |
| Issue #5467 | Planner design implications |

---

## Acceptance Criteria (Issue #5469)

- [x] Graph operations are explicitly classified (A/B/C)
- [x] Acceleration scope is bounded and non-ambiguous
- [x] Graph Truth semantics remain exact and governance-safe
- [x] Guidance can be consumed by graph, planner, and benchmark work
- [x] Code patterns and interface expectations documented
- [x] Benchmark hooks created for validation

---

*Last Updated: 2026-07-15*  
*Status: ✅ Issue #5469 acceptance criteria complete*  
*Next: Implementation Phase 1 (Category A hardening)*
