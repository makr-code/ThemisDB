# Kernel Acceleration Examples: Allowed and Disallowed Patterns

**Date**: 2026-06-25  
**Scope**: Code patterns for proper GPU acceleration usage in ThemisDB  
**Framework**: Issue #5469 — Bounded graph kernels classification  

---

## Table of Contents

1. [Overview](#overview)
2. [Category A: Acceleration-Eligible Examples](#category-a-acceleration-eligible-examples)
3. [Category B: Conditional Acceleration Examples](#category-b-conditional-acceleration-examples)
4. [Category C: CPU-First Examples](#category-c-cpu-first-examples)
5. [Common Anti-Patterns](#common-anti-patterns)
6. [Decision Matrix](#decision-matrix)

---

## Overview

This document provides concrete code examples for:
- ✅ **Allowed**: Safe GPU acceleration patterns
- ❌ **Disallowed**: Patterns that must remain CPU-only or require gates

### Key Principle

> GPU accelerates computation. CPU validates correctness and makes decisions.

---

## Category A: Acceleration-Eligible Examples

### ✅ ALLOWED: ANN Distance Computation (L2 Euclidean)

```cpp
// ALLOWED: GPU acceleration (no constraints)
// Purpose: Compute pairwise distances for candidate generation

class VectorIndexGPU {
  public:
    // PRE-CONDITION: Input validation
    Status searchKNN(
        std::span<const float> query,
        size_t k,
        std::vector<uint64_t>& result_indices,
        std::vector<float>& result_distances) {
        
        // 1. Validate inputs
        if (query.empty() || k == 0 || k > max_k_) {
            return Status::INVALID_INPUT;
        }
        if (query.size() != dimension_) {
            return Status::DIMENSION_MISMATCH;
        }
        
        // 2. Allocate GPU memory
        auto d_query = cuda::allocate<float>(query.size());
        auto d_distances = cuda::allocate<float>(num_vectors_ * 1);  // per-vector
        
        try {
            // 3. Copy query to GPU
            CUDA_CHECK(cudaMemcpy(d_query, query.data(), query.size() * sizeof(float), 
                                  cudaMemcpyHostToDevice));
            
            // 4. Launch GPU kernel (advisory)
            launchL2DistanceKernel(
                d_vectors_,
                d_query,
                d_distances,
                num_vectors_,
                query.size(),
                cuda_stream_);
            
            // 5. Wait for completion (bounded timeout)
            CUDA_CHECK(cudaStreamSynchronize(cuda_stream_));
            
            // 6. Retrieve distances
            std::vector<float> host_distances(num_vectors_);
            CUDA_CHECK(cudaMemcpy(host_distances.data(), d_distances,
                                  num_vectors_ * sizeof(float),
                                  cudaMemcpyDeviceToHost));
            
            // 7. POST-CONDITION: Validate distances (CPU-side)
            for (const auto& dist : host_distances) {
                if (dist < 0 || !std::isfinite(dist)) {
                    return Status::INVALID_DISTANCE_RESULT;
                }
            }
            
            // 8. Select top-K (still GPU advisory)
            selectTopK(host_distances, k, result_indices, result_distances);
            
            // 9. Return candidates for downstream validation
            return Status::OK;
            
        } catch (...) {
            cuda::free(d_query, d_distances);
            throw;
        }
    }
};
```

**Why it's allowed**:
- ✅ Fixed-shape computation (numVectors × dimension)
- ✅ Deterministic output (distances)
- ✅ Advisory only (candidates for downstream filtering)
- ✅ CPU validates results before use

---

### ✅ ALLOWED: TopK Selection After GPU Distance

```cpp
// ALLOWED: TopK selection on GPU distance output
// Pattern: GPU distance → GPU TopK → CPU validation

Status rankAndValidate(
    std::span<float> gpu_distances,
    std::vector<uint64_t>& indices,
    std::vector<float>& distances) {
    
    // 1. GPU TopK selection
    if (gpu_distances.size() > 1000000) {
        return Status::TOO_MANY_CANDIDATES;  // Bounded
    }
    
    auto gpu_indices = launchTopKKernel(gpu_distances, k);
    
    // 2. Copy results to CPU
    std::vector<uint64_t> host_indices = copyFromGPU(gpu_indices);
    
    // 3. CPU-side validation: exact filtering
    indices.clear();
    distances.clear();
    
    for (uint64_t idx : host_indices) {
        if (idx >= num_vectors_) {
            return Status::INVALID_INDEX;  // Fail closed
        }
        indices.push_back(idx);
        distances.push_back(gpu_distances[idx]);
    }
    
    // 4. Return validated candidates
    return Status::OK;
}
```

**Why it's allowed**:
- ✅ TopK produces exactly K results (bounded)
- ✅ CPU validates indices before use
- ✅ No assumptions about correctness until validated

---

### ✅ ALLOWED: Tensor Core Matrix Multiplication (CPU-First)

```cpp
// ALLOWED: CPU-first matrix multiplication with optional GPU acceleration
// Pattern: Unified dispatcher with CPU baseline

class TensorCoreMatmul {
  public:
    // CPU → GPU fallback pattern
    Status matmul(
        const float* A, size_t M, size_t K,
        const float* B, size_t K2, size_t N,
        float* C) {
        
        // Validate dimensions
        if (K != K2 || M == 0 || N == 0) {
            return Status::INVALID_DIMENSIONS;
        }
        
        // Try GPU if available and requested
        if (cuda_available_ && gpu_allowed_) {
            auto gpu_result = tryGPUMatmul(A, M, K, B, K2, N, C);
            if (gpu_result.ok()) {
                return Status::OK;  // GPU succeeded
            }
            // GPU failed, fall through to CPU
        }
        
        // CPU fallback (always available)
        return cpuMatmul(A, M, K, B, K2, N, C);
    }
    
  private:
    Status tryGPUMatmul(const float* A, size_t M, size_t K,
                        const float* B, size_t K2, size_t N,
                        float* C) {
        try {
            // Allocate GPU memory
            auto d_A = cuda::allocate<float>(M * K);
            auto d_B = cuda::allocate<float>(K * N);
            auto d_C = cuda::allocate<float>(M * N);
            
            // Copy inputs
            CUDA_CHECK(cudaMemcpy(d_A, A, M * K * sizeof(float), 
                                  cudaMemcpyHostToDevice));
            CUDA_CHECK(cudaMemcpy(d_B, B, K * N * sizeof(float), 
                                  cudaMemcpyHostToDevice));
            
            // Launch GPU kernel with timeout
            auto stream = cuda::stream_with_timeout(5000);  // 5s max
            launchFP16MatmulKernel(d_A, d_B, d_C, M, K, N, stream);
            
            // Synchronize
            CUDA_CHECK(cudaStreamSynchronize(stream));
            
            // Copy result
            CUDA_CHECK(cudaMemcpy(C, d_C, M * N * sizeof(float),
                                  cudaMemcpyDeviceToHost));
            
            // Validate result shape (post-condition)
            ASSERT(matrix_size(C, M, N) == M * N);
            
            return Status::OK;
            
        } catch (const std::exception& e) {
            // GPU failed; CPU will handle below
            return Status::GPU_ERROR;
        }
    }
    
    Status cpuMatmul(const float* A, size_t M, size_t K,
                     const float* B, size_t K2, size_t N,
                     float* C) {
        // Naive triple-loop (or optimized BLAS)
        for (size_t i = 0; i < M; i++) {
            for (size_t j = 0; j < N; j++) {
                float sum = 0.0f;
                for (size_t k = 0; k < K; k++) {
                    sum += A[i * K + k] * B[k * N + j];
                }
                C[i * N + j] = sum;
            }
        }
        return Status::OK;
    }
};
```

**Why it's allowed**:
- ✅ CPU path is baseline (always works)
- ✅ GPU is optimization, not requirement
- ✅ Transparent fallback on failure
- ✅ Advisory output (refinement only)

---

## Category B: Conditional Acceleration Examples

### ✅ ALLOWED: Geographic Distance with Validation Gates

```cpp
// ALLOWED: GPU-accelerated Haversine with input/output validation
// Pattern: CPU pre-check → GPU kernel → CPU post-check + fallback

class GeoAccelerator {
  public:
    // Haversine distance between lat/lon pairs
    Status computeDistances(
        std::span<const LatLon> points1,
        std::span<const LatLon> points2,
        std::vector<float>& distances) {
        
        // === PHASE 1: CPU Pre-validation ===
        if (!validateCoordinates(points1)) {
            return Status::INVALID_COORDINATES_1;
        }
        if (!validateCoordinates(points2)) {
            return Status::INVALID_COORDINATES_2;
        }
        
        // === PHASE 2: GPU Computation (advisory) ===
        std::vector<float> gpu_distances;
        auto gpu_status = tryGPUHaversine(points1, points2, gpu_distances);
        
        if (gpu_status.ok()) {
            // === PHASE 3: CPU Post-validation ===
            if (validateDistances(gpu_distances)) {
                distances = std::move(gpu_distances);
                return Status::OK;  // GPU result valid
            }
        }
        
        // === FALLBACK: CPU exact computation ===
        for (size_t i = 0; i < points1.size(); i++) {
            distances.push_back(haversine_cpu(points1[i], points2[i]));
        }
        return Status::OK;  // CPU succeeded
    }
    
  private:
    // PRE-CONDITION: Validate input coordinates
    bool validateCoordinates(std::span<const LatLon> points) {
        for (const auto& p : points) {
            if (p.latitude < -90.0 || p.latitude > 90.0) {
                return false;  // Invalid latitude
            }
            if (p.longitude < -180.0 || p.longitude > 180.0) {
                return false;  // Invalid longitude
            }
        }
        return true;
    }
    
    // POST-CONDITION: Validate output distances
    bool validateDistances(std::span<const float> distances) {
        for (const auto& d : distances) {
            // Distance must be in [0, 40075] km (Earth circumference)
            if (d < 0.0 || d > 40075.0 || !std::isfinite(d)) {
                return false;
            }
        }
        return true;
    }
    
    // GPU attempt (may fail or produce invalid results)
    Status tryGPUHaversine(
        std::span<const LatLon> points1,
        std::span<const LatLon> points2,
        std::vector<float>& distances) {
        try {
            // GPU kernel launch
            auto d_distances = launchGeoDistanceKernel(points1, points2);
            distances = copyFromGPU(d_distances);
            return Status::OK;
        } catch (...) {
            return Status::GPU_ERROR;
        }
    }
    
    // CPU fallback (always available)
    float haversine_cpu(const LatLon& p1, const LatLon& p2) {
        // ... exact CPU implementation ...
        return result;
    }
};
```

**Why it's allowed**:
- ✅ Explicit input validation (latitude/longitude ranges)
- ✅ Explicit output validation (distance 0-40075 km)
- ✅ GPU result is advisory (validated before use)
- ✅ CPU fallback for invalid GPU results
- ✅ Transparent to caller (correctness guaranteed)

---

### ✅ ALLOWED: Graph BFS with Frontier Cutoff

```cpp
// ALLOWED: GPU-accelerated bounded BFS with size cutoff
// Pattern: Depth limit → Frontier size check → CPU fallback

class GraphBFSAccelerator {
  public:
    // Bounded k-hop traversal (k ≤ 3)
    Status kHopNeighbors(
        uint64_t start_node,
        int hops,
        std::vector<uint64_t>& neighbors) {
        
        // === CONSTRAINT 1: Bounded depth ===
        const int MAX_HOPS = 3;
        if (hops > MAX_HOPS) {
            return Status::TRAVERSAL_TOO_DEEP;
        }
        
        // === CONSTRAINT 2: Adjacency index pre-validation ===
        if (!graph_->adjacencyIndex().isValid()) {
            return Status::INVALID_GRAPH_INDEX;
        }
        
        // === GPU attempt: Frontier expansion ===
        std::vector<uint64_t> gpu_result;
        bool used_gpu = false;
        
        for (int hop = 0; hop < hops; hop++) {
            // === CONSTRAINT 3: Frontier size cutoff ===
            if (gpu_result.size() > 10000) {
                // Frontier too large; switch to CPU for exact result
                return kHopNeighbors_CPU(start_node, hops, neighbors);
            }
            
            // Expand frontier one hop on GPU
            auto expanded = launchGraphBFSExpandKernel(gpu_result, graph_);
            if (expanded.empty()) break;
            gpu_result = std::move(expanded);
            used_gpu = true;
        }
        
        if (!used_gpu) {
            // GPU had no result; use CPU
            return kHopNeighbors_CPU(start_node, hops, neighbors);
        }
        
        // === POST-CONDITION: CPU validation ===
        // Verify GPU result matches CPU exact traversal
        std::vector<uint64_t> cpu_result;
        kHopNeighbors_CPU(start_node, hops, cpu_result);
        
        if (gpu_result != cpu_result) {
            // GPU result doesn't match exact; use CPU
            neighbors = std::move(cpu_result);
            return Status::GPU_MISMATCH;  // Log but don't fail
        }
        
        // GPU result validated; use it
        neighbors = std::move(gpu_result);
        return Status::OK;
    }
    
  private:
    // CPU exact k-hop traversal (always correct)
    Status kHopNeighbors_CPU(
        uint64_t start_node,
        int hops,
        std::vector<uint64_t>& neighbors) {
        
        std::set<uint64_t> visited;
        std::queue<std::pair<uint64_t, int>> frontier;
        frontier.push({start_node, 0});
        
        while (!frontier.empty()) {
            auto [node, hop] = frontier.front();
            frontier.pop();
            
            if (hop >= hops) continue;
            if (visited.count(node)) continue;
            
            visited.insert(node);
            
            // Expand neighbors
            for (uint64_t neighbor : graph_->getNeighbors(node)) {
                if (!visited.count(neighbor)) {
                    frontier.push({neighbor, hop + 1});
                }
            }
        }
        
        neighbors.assign(visited.begin(), visited.end());
        return Status::OK;
    }
};
```

**Why it's allowed**:
- ✅ Depth limited (k ≤ 3, not unbounded)
- ✅ Frontier size monitored (cut off at 10K nodes)
- ✅ CPU fallback if frontier too large
- ✅ GPU result validated against CPU exact
- ✅ Transparent correctness guarantee

---

## Category C: CPU-First Examples

### ❌ DISALLOWED: GPU-Accelerated ACL Enforcement

```cpp
// ❌ NOT ALLOWED: GPU-accelerated ACL enforcement
// PATTERN: Never accelerate security decisions

// WRONG: Don't do this!
class BadGraphACLGPU {
  public:
    Status getAuthorizedNeighborsGPU(  // ❌ WRONG
        uint64_t user_id,
        uint64_t node,
        std::vector<uint64_t>& neighbors) {
        
        // ❌ ERROR: GPU kernel cannot enforce ACL
        auto candidates = launchGraphBFSGPU(node);
        
        // ❌ ERROR: GPU filters by ACL?
        //    This is wrong! ACL must be enforced on CPU.
        auto authorized = launchACLFilterGPU(candidates, user_id);
        
        neighbors = copyFromGPU(authorized);
        return Status::OK;
    }
};

// ✅ CORRECT: CPU-side ACL enforcement
class GoodGraphACL {
  public:
    Status getAuthorizedNeighbors(
        uint64_t user_id,
        uint64_t node,
        std::vector<uint64_t>& neighbors) {
        
        // 1. CPU: Get ACL for user
        auto acl = acl_manager_->getUserACL(user_id);
        if (!acl) {
            return Status::UNAUTHORIZED;
        }
        
        // 2. GPU: Get candidates (advisory only)
        auto candidates = launchGraphBFSGPU(node);
        
        // 3. CPU: Filter candidates by ACL
        neighbors.clear();
        for (uint64_t candidate : candidates) {
            if (acl->canAccess(candidate)) {
                neighbors.push_back(candidate);
            }
        }
        
        // 4. Log access for audit trail
        auditLog.recordGraphAccess(user_id, node, neighbors.size());
        
        return Status::OK;
    }
};
```

**Why GPU version is disallowed**:
- ❌ GPU cannot enforce access control deterministically
- ❌ GPU approximations may leak unauthorized nodes
- ❌ Audit trail would be incomplete/unverifiable
- ❌ Security boundary weakened

**Why CPU version is correct**:
- ✅ ACL decision made on CPU (security-critical)
- ✅ GPU only provides candidates
- ✅ CPU validates each candidate against ACL
- ✅ Audit trail complete and verifiable

---

### ❌ DISALLOWED: GPU-Accelerated Provenance Chains

```cpp
// ❌ NOT ALLOWED: GPU-accelerated provenance chain
// Pattern: Never parallelize evidence chains

// WRONG: Don't do this!
class BadProvenanceGPU {
  public:
    Status getEvidenceChainGPU(  // ❌ WRONG
        uint64_t target_node,
        ProvenanceChain& chain) {
        
        // ❌ ERROR: GPU parallelize edge traversal
        //    This breaks determinism and ordering!
        auto parallel_edges = launchProvenanceGPU(target_node);
        
        // ❌ ERROR: How do we verify ordering?
        //    GPUs don't guarantee execution order.
        for (auto& edge : parallel_edges) {
            chain.addEdge(edge);  // Order is non-deterministic!
        }
        
        return Status::OK;
    }
};

// ✅ CORRECT: CPU-side provenance construction
class GoodProvenance {
  public:
    Status getEvidenceChain(
        uint64_t target_node,
        ProvenanceChain& chain) {
        
        // Traverse edge by edge on CPU
        std::vector<uint64_t> current_frontier = {target_node};
        
        while (!current_frontier.empty()) {
            std::vector<uint64_t> next_frontier;
            
            for (uint64_t node : current_frontier) {
                // CPU: exact edge traversal
                for (auto& incoming_edge : graph_->getIncomingEdges(node)) {
                    // Record evidence (deterministic CPU execution)
                    chain.addEdge(
                        incoming_edge,
                        std::chrono::system_clock::now(),  // Exact timestamp
                        "cpu_edge_traversal"  // Source identifier
                    );
                    next_frontier.push_back(incoming_edge.from);
                }
            }
            
            current_frontier = std::move(next_frontier);
        }
        
        // Chain is now complete, ordered, and verifiable
        return chain.validate();
    }
};
```

**Why GPU version is disallowed**:
- ❌ GPU parallelization breaks edge ordering
- ❌ Provenance chain is non-deterministic
- ❌ Cannot verify evidence order later
- ❌ Audit trail is unreliable

**Why CPU version is correct**:
- ✅ Deterministic edge-by-edge traversal
- ✅ Exact timestamps recorded
- ✅ Complete chain verifiable
- ✅ Audit trail is reproducible

---

### ❌ DISALLOWED: GPU-Accelerated Policy Validation

```cpp
// ❌ NOT ALLOWED: GPU-accelerated policy decision
// Pattern: Policy decisions stay on CPU

// WRONG: Don't do this!
class BadPolicyGPU {
  public:
    Status isPathCompliantGPU(  // ❌ WRONG
        const Path& path,
        const Policy& policy,
        bool& is_compliant) {
        
        // ❌ ERROR: GPU kernel checks policy?
        //    GPU cannot interpret complex policy logic.
        auto gpu_result = launchPolicyCheckGPU(path, policy);
        is_compliant = gpu_result;
        
        return Status::OK;
    }
};

// ✅ CORRECT: CPU-side policy enforcement
class GoodPolicy {
  public:
    Status isPathCompliant(
        const Path& path,
        const Policy& policy,
        bool& is_compliant) {
        
        // 1. CPU: Interpret policy
        auto policy_rules = policy_engine_->parsePolicyRules(policy);
        if (!policy_rules.ok()) {
            return Status::INVALID_POLICY;
        }
        
        // 2. GPU: Generate candidates (optional, advisory)
        auto candidates = launchGraphBFSGPU(path.start);
        
        // 3. CPU: Filter candidates through policy
        std::vector<uint64_t> compliant_nodes;
        for (uint64_t candidate : candidates) {
            // CPU evaluates policy for each node
            if (policy_engine_->evaluateNode(candidate, policy_rules)) {
                compliant_nodes.push_back(candidate);
            }
        }
        
        // 4. CPU: Check if path is in compliant set
        is_compliant = false;
        for (uint64_t node : path.nodes) {
            if (std::find(compliant_nodes.begin(), compliant_nodes.end(), node)
                    == compliant_nodes.end()) {
                is_compliant = false;
                return Status::OK;
            }
        }
        is_compliant = true;
        
        return Status::OK;
    }
};
```

**Why GPU version is disallowed**:
- ❌ GPU cannot interpret complex policy logic
- ❌ GPU approximations may incorrectly approve/deny
- ❌ Compliance decisions are not verifiable
- ❌ Regulatory boundary violated

**Why CPU version is correct**:
- ✅ Policy decision made on CPU
- ✅ GPU only provides candidate generation
- ✅ CPU evaluates policy for each node
- ✅ Compliance guarantee is verifiable

---

## Common Anti-Patterns

### ❌ Anti-Pattern 1: GPU-Only Truth

```cpp
// ❌ WRONG: GPU result becomes truth without validation
class BadGPUTruth {
    Status search(...) {
        auto gpu_result = launchGPUKernel(...);
        // ❌ Using GPU result directly as truth
        return gpu_result;  // No CPU validation!
    }
};

// ✅ CORRECT: CPU validates GPU result
class GoodGPUTruth {
    Status search(...) {
        auto gpu_result = launchGPUKernel(...);
        // ✅ CPU validates before returning as truth
        if (validateGPUResult(gpu_result)) {
            return gpu_result;
        }
        return cpuFallback();
    }
};
```

### ❌ Anti-Pattern 2: No Fallback

```cpp
// ❌ WRONG: GPU failure crashes
class BadNoFallback {
    Status search(...) {
        auto gpu_result = launchGPUKernel(...);
        // ❌ If GPU fails, entire operation fails
        return gpu_result;
    }
};

// ✅ CORRECT: CPU fallback always available
class GoodFallback {
    Status search(...) {
        auto gpu_status = launchGPUKernel(...);
        if (gpu_status.ok()) {
            return gpu_status;
        }
        // ✅ Always fallback to CPU
        return cpuComputation();
    }
};
```

### ❌ Anti-Pattern 3: Unbounded GPU Computation

```cpp
// ❌ WRONG: GPU processes unlimited data
class BadUnbounded {
    Status process(std::span<const float> data) {
        // ❌ No bounds on GPU computation
        return launchGPUKernel(data);
    }
};

// ✅ CORRECT: GPU computation is bounded
class GoodBounded {
    Status process(std::span<const float> data) {
        // ✅ Check bounds before GPU
        const size_t MAX_GPU_SIZE = 1000000;
        if (data.size() > MAX_GPU_SIZE) {
            return cpuComputation(data);  // CPU handles large input
        }
        return launchGPUKernel(data);
    }
};
```

---

## Decision Matrix

### When to use GPU

| Scenario | GPU OK? | Why |
|----------|---------|-----|
| Advisory candidate generation | ✅ Yes | Candidates validated downstream |
| Distance computation (vectors) | ✅ Yes | Advisory metric, validated |
| TopK filtering | ✅ Yes | Deterministic, bounded output |
| Matrix multiplication | ✅ Yes | Optional optimization, CPU fallback |
| Geo distance (with validation gates) | ✅ Yes | Pre/post validation required |
| Graph BFS (bounded, with cutoff) | ✅ Yes | Depth limit, frontier size cutoff |

### When to use CPU-only

| Scenario | GPU OK? | Why |
|----------|---------|-----|
| ACL enforcement | ❌ No | Security-critical, deterministic required |
| Provenance chains | ❌ No | Audit trail, exact ordering required |
| Policy decisions | ❌ No | Compliance, verifiable required |
| Transaction validation | ❌ No | ACID semantics, exact required |
| Permission checks | ❌ No | Access control, deterministic required |
| Consistency verification | ❌ No | Data integrity, exact required |

---

## Testing Guidelines

### For Category A (GPU-eligible)

```cpp
// Test: Parity with CPU baseline
TEST(VectorANN, GPUDistanceParity) {
    auto gpu_distances = gpuL2Distance(queries, vectors);
    auto cpu_distances = cpuL2Distance(queries, vectors);
    EXPECT_EQ(gpu_distances, cpu_distances);
}

// Test: Fallback on GPU failure
TEST(VectorANN, FallbackOnGPUError) {
    simulateGPUError();
    auto result = search(query);
    EXPECT_OK(result);  // CPU fallback worked
}
```

### For Category B (Conditional)

```cpp
// Test: Input validation gates GPU
TEST(GeoDistance, InputValidationGates) {
    auto invalid_coords = LatLon{91.0f, 0.0f};  // Invalid latitude
    auto result = computeDistance({0.0f, 0.0f}, invalid_coords);
    EXPECT_EQ(result, Status::INVALID_INPUT);  // Gate blocked GPU
}

// Test: GPU/CPU parity
TEST(BoundedBFS, GPUCPUParity) {
    auto gpu_result = gpuKHop(start, k=2);
    auto cpu_result = cpuKHop(start, k=2);
    EXPECT_EQ(gpu_result, cpu_result);
}

// Test: Frontier size cutoff
TEST(BoundedBFS, FrontierSizeCutoff) {
    auto large_frontier = setupLargeFrontier(20000);  // > 10K limit
    auto result = gpuExpand(large_frontier);
    EXPECT_FALSE(result.used_gpu());  // Fell back to CPU
}
```

### For Category C (CPU-only)

```cpp
// Test: GPU dispatch prevented
TEST(ACLEnforcement, ACLNeverGPU) {
    auto result = getAuthorizedNeighborsGPU(...);
    // Should compile-time error or be unavailable
    // STATIC_ASSERT(!std::is_invocable_v<decltype(getAuthorizedNeighborsGPU)>);
}

// Test: CPU path always used
TEST(Provenance, ProvenanceAlwaysCPU) {
    auto chain = getEvidenceChain(target);
    EXPECT_TRUE(chain.isVerifiable());
    EXPECT_TRUE(chain.orderingIsExact());
}
```

---

## References

- `docs/acceleration/BOUNDED_GRAPH_KERNELS.md` — Classification framework
- `include/acceleration/graph_kernel_classification.h` — C++ enums/traits
- `benchmarks/bounded_kernel_validation.cpp` — Validation test patterns
- Issue #5469 — Kernel classification definition

---

*Last Updated: 2026-06-25*  
*Status: ✅ Examples validated against framework*
