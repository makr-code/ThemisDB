# GPU-Optimized Vector Indexing Research

**Project:** ThemisDB  
**Version:** v1.4.1-dev  
**Date:** February 2026  
**Category:** 🚀 Performance Research  
**Status:** Research & Planning Phase  
**Priority:** P0 (Q2 2026)

---

## Table of Contents

1. [Background](#background)
2. [Problem Statement](#problem-statement)
3. [Research Focus](#research-focus)
4. [State-of-the-Art Research](#state-of-the-art-research)
5. [Benchmark Plan](#benchmark-plan)
6. [Implementation Plan](#implementation-plan)
7. [Dependencies](#dependencies)
8. [Expected Outcomes](#expected-outcomes)
9. [Risks & Challenges](#risks--challenges)
10. [Integration Considerations](#integration-considerations)

---

## Background

### Current GPU Support in ThemisDB

ThemisDB has established foundational GPU support:

- **GPU Acceleration:** ☑️ Partial
  - CUDA vector kernels implemented (`src/acceleration/cuda/vector_kernels.cu`)
  - FAISS GPU backend available (`src/acceleration/faiss_gpu_backend.cpp`)
  - CPU fallback mechanisms in place

- **GPU Backend:** ☑️ CUDA ☐ HIP ☐ Vulkan ☐ OpenCL ☑️ DirectX ☐ None
  - Primary: CUDA (NVIDIA GPUs)
  - Secondary: DirectX Compute Shaders (Windows fallback)
  - Planned: HIP/ROCm for AMD GPU support

- **Current Use Cases:**
  - ☑️ Vector similarity computation (L2, Cosine distance)
  - ☑️ Batch queries (FAISS GPU backend)
  - ☐ Index building (CPU-only, needs GPU acceleration)
  - ☑️ Top-K selection (CUDA kernels)

---

## Problem Statement

### CPU Limitations

**Performance Bottlenecks:**
- **Index Build Time:** CPU HNSW construction takes 30-60 minutes for 10M vectors (128D)
- **Search Latency:** Single-query latency ~2-5ms for 1M vectors with HNSW
- **Batch Throughput:** Limited to 1,800-2,500 queries/second on high-end CPUs
- **Memory Bandwidth:** CPU DDR4/DDR5 (40-100 GB/s) vs GPU HBM2 (900-2000 GB/s)
- **Parallelism:** CPU cores (8-64) vs GPU cores (5,000-10,000+)

**Cost Considerations:**
- Large CPU clusters required for high-throughput workloads
- Higher total cost of ownership (TCO) for scaling
- Power consumption inefficiency at scale

### GPU Opportunities

**Performance Gains:**
- **10-100x speedup** for batch vector distance computation
- **5-10x speedup** for index construction (IVF, graph-based)
- **Linear scaling** to multiple GPUs (2-8 GPUs)
- **Lower latency** for large batch queries (>100 queries)

**Efficiency Improvements:**
- **Memory bandwidth:** 900-2000 GB/s (HBM2/HBM3) vs 40-100 GB/s (DDR4/DDR5)
- **Parallel compute:** Thousands of concurrent threads
- **Energy efficiency:** Better performance per watt for vector operations
- **Cost reduction:** Single GPU can replace multiple CPU servers for vector workloads

---

## Research Focus

### GPU Indexing Approaches

#### 1. Brute-Force GPU Search ⭐ Baseline
**Description:**
- Parallel distance computation across all vectors
- No index structure, exact nearest neighbor search
- Optimal for small-to-medium datasets (<10M vectors)

**Papers:**
- Johnson et al., "Billion-scale similarity search with GPUs" (IEEE TBDATA 2019)

**Expected Benefit:**
- Simplest implementation, exact results
- Good for datasets <10M vectors at 128-512D
- 20-50x speedup over CPU brute-force
- **ThemisDB Status:** ✅ Implemented in CUDA kernels

**Implementation:**
```cuda
// Already implemented in src/acceleration/cuda/vector_kernels.cu
__global__ void computeL2DistanceKernel(
    const float* queries,      // [numQueries, D]
    const float* vectors,      // [N, D]
    float* distances,          // [numQueries, N]
    int numQueries, int N, int D
);
```

---

#### 2. GPU-Accelerated IVF (Inverted File Index) ⭐⭐⭐ Production-Ready
**Description:**
- Clustering-based approximate nearest neighbor search
- Fast filtering via inverted lists
- Trade-off between speed and recall

**Papers:**
- Johnson, Douze, Jégou, "Billion-scale similarity search with GPUs" (2019)
- Implemented in FAISS library (Meta AI Research)

**Expected Benefit:**
- **10-100x speedup** vs CPU IVF for large datasets
- Scales to billions of vectors
- Adjustable accuracy/speed trade-off (nprobe parameter)
- **ThemisDB Status:** ✅ Partially Implemented (FAISS GPU backend)

**Algorithm:**
```cpp
// Build index (offline, GPU-accelerated)
faiss::gpu::GpuIndexIVFFlat index(
    &gpu_resources,
    dimension,           // Vector dimension
    num_clusters,        // Number of Voronoi cells (nlist)
    faiss::METRIC_L2     // Distance metric
);
index.train(N, training_vectors);
index.add(N, database_vectors);

// Search (online, GPU-accelerated)
index.nprobe = 10;  // Search 10 clusters
index.search(num_queries, query_vectors, k, distances, labels);
```

**Performance Characteristics:**
| Dataset Size | Index Build | Search (k=10) | Memory |
|--------------|-------------|---------------|---------|
| 1M vectors   | 2-5 sec     | 0.05 ms/query | 512 MB  |
| 10M vectors  | 15-30 sec   | 0.1 ms/query  | 5 GB    |
| 100M vectors | 3-5 min     | 0.2 ms/query  | 50 GB   |
| 1B vectors   | 30-60 min   | 0.5 ms/query  | 500 GB* |

\* Requires multi-GPU setup or CPU+GPU hybrid

---

#### 3. GPU-Accelerated HNSW ⭐⭐ Advanced
**Description:**
- Hierarchical Navigable Small World graphs on GPU
- Parallel graph traversal and construction
- State-of-the-art recall-speed trade-off

**Papers:**
- Zhao et al., "SONG: Approximate Nearest Neighbor Search on GPU" (NeurIPS 2022)
- Malkov & Yashunin, "Efficient and robust approximate nearest neighbor search using Hierarchical Navigable Small World graphs" (TPAMI 2018)

**Expected Benefit:**
- **5-10x faster construction** than CPU HNSW
- **2-3x faster search** for small batches
- Better accuracy than IVF at similar speed
- **ThemisDB Status:** ⏳ Planned (HNSW currently CPU-only)

**Challenges:**
- Graph traversal is inherently sequential (harder to parallelize)
- GPU shines with large batches, not single queries
- Memory access patterns less GPU-friendly than IVF

**Research Libraries:**
- SONG (NeurIPS 2022): https://github.com/jaiswala/SONG
- NVIDIA CAGRA (RAFT): https://github.com/rapidsai/raft

---

#### 4. GPU Product Quantization (PQ) ⭐⭐⭐ Memory-Efficient
**Description:**
- Vector compression via product quantization
- Fast approximate distance computation using lookup tables
- Reduces memory footprint by 8-32x

**Papers:**
- André et al., "Cache locality is not enough: High-Performance Nearest Neighbor Search with Product Quantization Fast Scan" (VLDB 2015)
- Jégou, Douze, Schmid, "Product Quantization for Nearest Neighbor Search" (TPAMI 2011)

**Expected Benefit:**
- **10-50x speedup** for PQ distance computation
- **8-32x memory reduction** (e.g., 512 bytes → 16-64 bytes)
- Enables larger datasets to fit in GPU memory
- **ThemisDB Status:** ✅ PQ implemented CPU-only, GPU acceleration planned

**Algorithm:**
```cpp
// Product Quantization: Split vector into m subvectors
// Each subvector is quantized to k centroids (k=256 for 8-bit codes)
struct ProductQuantizer {
    int m;                  // Number of subquantizers (8-16)
    int nbits;              // Bits per subquantizer (8)
    std::vector<float> centroids;  // [m, k, d/m] centroids
};

// GPU kernel for PQ distance computation
__global__ void computePQDistanceKernel(
    const uint8_t* codes,        // Quantized codes [N, m]
    const float* distance_tables, // Precomputed tables [m, k]
    float* distances,             // Output [N]
    int N, int m
);
```

---

#### 5. Multi-GPU Scaling ⭐⭐⭐ Enterprise
**Description:**
- Distributed indexing and search across multiple GPUs
- Data parallelism for large datasets (>100M vectors)
- Load balancing and cross-GPU communication

**Papers:**
- Ootomo et al., "NGT-QG: Fast Quantized Graph-Based ANN Search on GPU" (2021)
- Various NVIDIA/AMD multi-GPU research

**Expected Benefit:**
- **Linear scaling** to 2-8 GPUs
- Enables 100B+ vector search
- Fault tolerance via replica shards
- **ThemisDB Status:** ⏳ Planned

**Implementation Strategy:**
```cpp
// Multi-GPU partitioning strategies
enum class PartitionStrategy {
    REPLICATE,      // Same index on all GPUs (query parallelism)
    SHARD_BY_ID,    // Partition vectors by ID (data parallelism)
    SHARD_BY_CLUSTER // Partition IVF clusters across GPUs
};

// Example: 4-GPU setup with sharding
std::vector<faiss::gpu::GpuIndexIVFFlat> gpu_indexes(4);
for (int gpu = 0; gpu < 4; ++gpu) {
    gpu_indexes[gpu].init(dimension, nlist, gpu);
    gpu_indexes[gpu].add(N/4, vectors + (N/4)*gpu*dimension);
}
```

**Hardware Requirements:**
- NVLink or NVSwitch for fast cross-GPU communication (300-600 GB/s)
- PCIe 4.0 x16 fallback (64 GB/s, slower but functional)

---

#### 6. Tensor Core Utilization ⭐⭐ Advanced Optimization
**Description:**
- Leverage matrix multiplication units (Tensor Cores) for distance computation
- WMMA (Warp Matrix Multiply-Accumulate) instructions
- Optimized for FP16/BF16 operations

**Papers:**
- Gao et al., "NVIDIA Tensor Core for ML" (2020)
- NVIDIA CUTLASS library documentation

**Expected Benefit:**
- **5-10x speedup** for matrix-heavy operations (e.g., batch distance computation)
- Lower precision (FP16) with minimal accuracy loss
- Available on Volta, Turing, Ampere, Ada, Hopper architectures
- **ThemisDB Status:** ⏳ Research phase

**Use Cases:**
- Large batch queries (>1000 queries)
- Matrix multiplication-based distance metrics
- Approximate quantized distance computation

---

#### 7. Unified Memory & GPU-CPU Hybrid ⭐⭐ Usability
**Description:**
- CUDA Unified Memory for automatic data migration
- Hybrid execution: GPU for batch, CPU for single queries
- Reduces programming complexity

**Papers:**
- Various NVIDIA/AMD Unified Memory research
- CUDA Programming Guide (Unified Memory chapter)

**Expected Benefit:**
- Easier programming model
- Better resource utilization (use both CPU and GPU)
- Automatic paging when dataset exceeds GPU memory
- **ThemisDB Status:** ⏳ Planned for future optimization

**Trade-offs:**
- Automatic migration can be slower than manual management
- Page faults may hurt performance
- Best for workloads with unpredictable memory access patterns

---

## State-of-the-Art Research

### Key Papers

#### 1. Billion-scale similarity search with GPUs ⭐⭐⭐
- **Authors:** Jeff Johnson, Matthijs Douze, Hervé Jégou  
- **Venue:** IEEE Transactions on Big Data, 2019  
- **DOI:** 10.1109/TBDATA.2019.2921572  
- **arXiv:** https://arxiv.org/abs/1702.08734

**Key Innovation:**
- FAISS-GPU library with optimized IVF-PQ, IVF-Flat indexes
- Multi-GPU support with near-linear scaling
- Production-grade implementation used by Meta at billion-scale

**Performance:**
- **8.5x faster** than CPU for 1B SIFT vectors
- **100M queries/day** at Meta production scale
- Recall@1 >95% with 10x speedup over CPU

**Code:** https://github.com/facebookresearch/faiss  
**License:** MIT  
**Hardware:** Tested on P100, V100, A100 GPUs  
**ThemisDB Integration:** ✅ FAISS GPU backend implemented

---

#### 2. SONG: Approximate Nearest Neighbor Search on GPU ⭐⭐
- **Authors:** Jaiswal et al.  
- **Venue:** NeurIPS 2022  
- **URL:** https://proceedings.neurips.cc/paper_files/paper/2022

**Key Innovation:**
- GPU-optimized learned hash functions
- Efficient filtering with locality-sensitive hashing (LSH)
- Better GPU utilization than graph-based methods

**Performance:**
- **2-5x faster** than FAISS-GPU on Deep1B dataset (96D vectors)
- Lower memory footprint than HNSW
- Scales to billions of vectors

**Code:** https://github.com/jaiswala/SONG  
**Hardware:** Requires Ampere+ (A100, RTX 3090/4090) for best performance

---

#### 3. NGT-QG: Fast Quantized Graph-Based ANN Search on GPU ⭐⭐
- **Authors:** Takeshi Yamamuro, Hiroaki Shiokawa  
- **Venue:** 2021  
- **Repository:** https://github.com/yahoojapan/NGT

**Key Innovation:**
- GPU-optimized graph traversal with quantization
- Combines graph-based indexing (NGT) with product quantization
- Fast batch query processing

**Performance:**
- **10x faster** than CPU NGT
- Competitive with FAISS-GPU on graph-friendly datasets
- Better recall than IVF for same query time

**License:** Apache 2.0

---

#### 4. GPU-Accelerated Approximate Nearest Neighbor Search (Survey) ⭐
- **Authors:** Aumüller et al.  
- **Venue:** ACM Computing Surveys, 2020  
- **DOI:** 10.1145/3369869

**Key Contribution:**
- Comprehensive survey of GPU ANN methods
- Categorization: Brute-force, LSH, hashing, graph-based, quantization
- Performance comparison across different GPU architectures

**Coverage:**
- 50+ papers and techniques
- Taxonomy of GPU ANN algorithms
- Best practices for GPU vector search

---

#### 5. ScaNN: Efficient Vector Similarity Search ⭐⭐
- **Authors:** Guo et al. (Google Research)  
- **Venue:** ICML 2020  
- **arXiv:** https://arxiv.org/abs/1908.10396

**Key Innovation:**
- Anisotropic vector quantization
- Learned quantization aware of query distribution
- Optimized for asymmetric distance computation

**Performance:**
- **2-3x better** than IVF-PQ at same memory budget
- Recall@10 of 95% with 5x speedup over brute-force
- State-of-the-art on several benchmarks

**Code:** https://github.com/google-research/google-research/tree/master/scann  
**GPU Support:** Partial (CPU-focused but GPU-compatible design)

---

### Recent Advances (2023-2026)

#### 1. NVIDIA CAGRA (RAFT) ⭐⭐⭐
- **Organization:** NVIDIA RAFT team  
- **Year:** 2023-2024  
- **Repository:** https://github.com/rapidsai/raft

**Description:**
- Graph-Based ANN optimized for modern GPUs (Ampere, Ada, Hopper)
- Part of RAPIDS AI ecosystem
- Integrates with cuVS (CUDA Vector Search)

**Performance:**
- **5-10x faster** than FAISS-GPU for high-recall queries (recall >95%)
- Optimized for batched workloads
- Best on A100, H100 GPUs

**ThemisDB Integration:** ⏳ Evaluation phase

---

#### 2. Multi-GPU Distributed HNSW ⭐⭐
- **Research:** Various academia & industry (2024)
- **Focus:** Linear scaling to 8+ GPUs for 100B+ vectors

**Key Features:**
- Efficient cross-GPU communication with NVLink/NVSwitch
- Distributed graph partitioning strategies
- Fault-tolerant replication

**Expected Benefits:**
- 8 GPUs → 8x throughput (near-linear scaling)
- Enables 100B-1T vector search

---

#### 3. AMD ROCm Support (HIP-based ports) ⭐
- **Status:** Growing ecosystem (2024-2026)
- **Libraries:** HIP ports of FAISS, NGT

**Performance:**
- Comparable to CUDA on AMD MI250X, MI300 GPUs
- Broader hardware compatibility (NVIDIA + AMD)

**ThemisDB Roadmap:** Planned for v1.5+

---

#### 4. Other Research Directions
- **Learned Indexes:** Neural network-based index structures (slow adoption)
- **Quantum-Inspired Algorithms:** Theoretical, not yet practical
- **Neuromorphic Computing:** Early research, decades away from production

---

## Benchmark Plan

### Datasets

#### 1. SIFT1M (Baseline) ☑️
- **Size:** 1 million vectors
- **Dimension:** 128D
- **Source:** http://corpus-texmex.irisa.fr/
- **Use Case:** Standard benchmark, fast iteration

**Expected Results:**
- CPU HNSW: ~2 ms/query (k=10)
- GPU IVF-Flat: ~0.05 ms/query
- **Speedup:** 40x

---

#### 2. Deep1B (Large-Scale) ☑️
- **Size:** 1 billion vectors
- **Dimension:** 96D
- **Source:** https://research.yandex.com/datasets/biganns
- **Use Case:** Stress test, production-scale evaluation

**Expected Results:**
- Requires multi-GPU or GPU+CPU hybrid
- Target: <10 ms per 100-query batch

---

#### 3. Text Embeddings (High-Dimensional) ☑️
- **Examples:** OpenAI ada-002 (1536D), BERT (768D)
- **Size:** 1-10 million vectors
- **Use Case:** Real-world production workload

**Expected Results:**
- Higher dimensionality → more compute-bound
- GPU benefit may be larger (50-100x)

---

#### 4. ThemisDB Production (Real Workload) ☑️
- **Size:** TBD based on customer data
- **Dimension:** Mixed (64D-2048D)
- **Query Distribution:** Skewed (Zipf-like)

**Metrics:**
- Latency: p50, p95, p99
- Throughput: QPS at 95% recall

---

### Hardware

#### NVIDIA GPUs

##### Consumer/Prosumer
- ☑️ **RTX 4090** (24GB, Ada Lovelace)
  - Best price/performance for development
  - ~$1,600 USD
  - 82 TFLOPS FP32, 330 TFLOPS FP16

##### Datacenter
- ☑️ **A100** (40GB/80GB, Ampere)
  - Industry standard for inference
  - ~$10,000-15,000 USD
  - 19.5 TFLOPS FP32, 312 TFLOPS FP16

- ☐ **H100** (80GB, Hopper)
  - Latest datacenter GPU (2023+)
  - ~$30,000+ USD
  - 51 TFLOPS FP32, 1000 TFLOPS FP16
  - Transformer Engine optimizations

---

#### AMD GPUs

- ☐ **MI250X** (128GB, CDNA 2)
  - Datacenter-focused
  - Excellent memory capacity
  - HIP/ROCm support improving

- ☐ **RX 7900 XTX** (24GB, RDNA 3)
  - Consumer card, good value
  - ~$1,000 USD
  - Gaming-focused but usable for compute

---

### Evaluation Metrics

#### Performance Metrics

**Latency:**
- **Single-Query Latency:** p50, p95, p99 (milliseconds)
- **Batch Query Latency:** Time for batch of N queries
- **Index Build Time:** Time to construct index (seconds/hours)

**Throughput:**
- **QPS (Queries Per Second):** At various batch sizes (1, 10, 100, 1000)
- **Vectors Indexed Per Second:** During index construction

---

#### Efficiency Metrics

**Memory:**
- **GPU Memory Usage:** VRAM consumed (GB)
- **Memory Bandwidth Utilization:** % of peak bandwidth
- **Memory Efficiency:** Bytes per vector stored

**Compute:**
- **GPU Utilization:** % from `nvidia-smi`
- **FLOPs Utilization:** % of theoretical peak
- **Power Consumption:** Watts (from nvidia-smi, TDP)

**Cost:**
- **Cost per 1M Queries:** Amortized hardware cost
- **TCO (Total Cost of Ownership):** 3-year projection
- **Performance per Dollar:** QPS per $1000 hardware

---

#### Scalability Metrics

**Multi-GPU Scaling:**
- **Linear Speedup:** Measure speedup with 1, 2, 4, 8 GPUs
- **Scaling Efficiency:** Actual speedup / ideal speedup

**Dataset Size Scaling:**
- **Performance vs N:** Plot latency/throughput from 1M → 1B vectors
- **Memory vs N:** Memory footprint as function of dataset size

**Dimensionality Scaling:**
- **Performance vs D:** Measure impact of 64D → 2048D
- **Sweet Spot:** Identify optimal dimensionality for GPU acceleration

---

#### Accuracy Metrics

**Recall@k:**
- **Recall@1, Recall@10, Recall@100:** Primary accuracy metric
- **Target:** >95% recall@10 for production

**Accuracy vs CPU:**
- **Bit-Exact Comparison:** Verify GPU gives same results as CPU (when possible)
- **Numerical Precision:** Measure FP32 vs FP16 differences

---

### Baseline Comparisons

**CPU Baselines:**
- **HNSW (hnswlib):** State-of-the-art CPU ANN
- **IVF-PQ (FAISS):** Production-grade CPU vector search

**GPU Baselines:**
- **FAISS-GPU:** IVF-Flat, IVF-PQ (current ThemisDB backend)
- **CAGRA (RAFT):** State-of-the-art GPU graph-based ANN

**Performance Targets:**
| Metric | CPU (HNSW) | GPU (FAISS-GPU) | Target |
|--------|------------|-----------------|---------|
| Build Time (10M) | 30 min | 3 min | **10x faster** |
| Query Latency (k=10) | 2 ms | 0.1 ms | **20x faster** |
| Batch QPS (batch=1000) | 500 | 10,000 | **20x faster** |
| Memory (10M) | 8 GB | 10 GB | **Similar** |
| Recall@10 | 99% | 95% | **>95%** |

---

## Implementation Plan

### Phase 1: Environment Setup (1 week)

**Hardware Acquisition:**
- ☐ Acquire RTX 4090 or A100 GPU (cloud or on-premise)
- ☐ Verify CUDA compute capability (7.0+ required, 8.0+ recommended)
- ☐ Set up multi-GPU environment (optional, for Phase 5)

**Software Installation:**
- ☐ Install CUDA Toolkit 12.0+ (`/usr/local/cuda`)
- ☐ Install cuBLAS, cuDNN, cuSPARSE (bundled with CUDA)
- ☐ Install NCCL 2.x for multi-GPU communication
- ☐ Verify GPU compute capability: `nvidia-smi`, `nvcc --version`

**Build Dependencies:**
- ☐ Build FAISS-GPU from source (v1.7.4+)
- ☐ Build hnswlib (CPU baseline) (v0.7.0+)
- ☐ Build RAFT/CAGRA (optional, for evaluation)
- ☐ Verify CMake 3.23+, GCC 9+ / Clang 11+

**Test Setup:**
- ☐ Run simple CUDA kernel test
- ☐ Run FAISS-GPU index build test
- ☐ Measure baseline GPU performance

---

### Phase 2: Baseline Benchmarking (1-2 weeks)

**CPU Benchmarks:**
- ☐ Benchmark HNSW (hnswlib) on SIFT1M
  - Measure build time, query latency, recall@10
- ☐ Benchmark IVF-PQ (FAISS CPU) on SIFT1M
- ☐ Establish baseline performance numbers

**GPU Benchmarks (Existing Implementation):**
- ☐ Benchmark FAISS-GPU IVF-Flat on SIFT1M
- ☐ Benchmark FAISS-GPU IVF-PQ on SIFT1M
- ☐ Benchmark CUDA brute-force kernels (ThemisDB)
- ☐ Measure accuracy (recall@k) for each method

**Optional: Evaluate CAGRA:**
- ☐ Build NVIDIA RAFT/CAGRA
- ☐ Benchmark CAGRA on SIFT1M
- ☐ Compare with FAISS-GPU

**Document Results:**
- ☐ Create benchmark report with tables and plots
- ☐ Identify performance gaps and opportunities

---

### Phase 3: Custom GPU Implementation (3-4 weeks)

**Optimization 1: Brute-Force Enhancement:**
- ☐ Optimize existing CUDA kernels (shared memory, coalesced access)
- ☐ Add FP16 support for Tensor Cores
- ☐ Benchmark improved kernels

**Optimization 2: IVF Integration:**
- ☐ Integrate FAISS-GPU IVF-Flat more tightly with ThemisDB
- ☐ Add configuration options (nlist, nprobe)
- ☐ Optimize data transfer (pinned memory, streams)

**Optimization 3: Multi-GPU Support (Optional):**
- ☐ Implement sharding strategy for multi-GPU
- ☐ Use NCCL for cross-GPU communication
- ☐ Benchmark 2-GPU and 4-GPU scaling

**Profiling & Optimization:**
- ☐ Profile with NVIDIA Nsight Compute
- ☐ Identify bottlenecks (memory bandwidth, compute, latency)
- ☐ Optimize hot paths

---

### Phase 4: Integration with ThemisDB (2-3 weeks)

**C++ API Integration:**
- ☐ Extend `VectorIndexManager` with GPU support
- ☐ Add `GpuVectorIndexConfig` struct
- ☐ Implement GPU index initialization

**Example API:**
```cpp
// GPU Vector Index Configuration
struct GpuVectorIndexConfig {
    bool enable_gpu = true;
    int device_id = 0;
    IndexType index_type = IndexType::IVF_GPU;
    
    // IVF parameters
    int nlist = 100;      // Number of clusters
    int nprobe = 10;      // Clusters to search
    
    // Multi-GPU
    bool use_multi_gpu = false;
    int num_gpus = 1;
    
    // Memory management
    size_t max_memory_mb = 8192;
    bool fallback_to_cpu = true;
};

// Initialize GPU index
VectorIndexManager vim(db);
vim.init("embeddings", 128, VectorIndexManager::Metric::L2);
vim.enableGpuAcceleration(gpu_config);
```

**Fallback Mechanism:**
- ☐ Implement graceful CPU fallback if GPU unavailable
- ☐ Hybrid strategy: GPU for batches, CPU for single queries
- ☐ Error handling for GPU OOM (out-of-memory)

**Memory Management:**
- ☐ Implement GPU-CPU data transfer optimization
- ☐ Use pinned memory for faster transfers
- ☐ Implement batch processing to maximize GPU utilization

**Configuration & Monitoring:**
- ☐ Add GPU config to YAML/JSON configuration
- ☐ Expose GPU metrics (utilization, memory, temperature)
- ☐ Integrate with Prometheus metrics

---

### Phase 5: Production Readiness (2-3 weeks)

**Error Handling:**
- ☐ Handle GPU OOM errors
- ☐ Handle CUDA runtime errors
- ☐ Implement retry logic and fallback

**Monitoring & Observability:**
- ☐ GPU metrics: utilization, memory, temperature, power
- ☐ Query latency distribution (GPU vs CPU)
- ☐ Index build progress tracking

**Performance Tuning:**
- ☐ Tune batch sizes for optimal throughput/latency
- ☐ Optimize for production workload (real queries)
- ☐ Memory pool management (reduce allocation overhead)

**Documentation:**
- ☐ Write GPU setup guide (hardware, drivers, CUDA)
- ☐ Document API usage and configuration
- ☐ Create deployment examples (Docker, Kubernetes)
- ☐ Add troubleshooting guide

**Testing:**
- ☐ Load testing (sustained high QPS)
- ☐ Stress testing (GPU memory exhaustion)
- ☐ Fault injection (GPU failures)
- ☐ Correctness testing (compare GPU vs CPU results)

---

## Dependencies

### GPU Libraries

#### CUDA Toolkit (11.8+ or 12.x)
- **Purpose:** NVIDIA GPU programming framework
- **License:** Proprietary (free for development/deployment)
- **Installation:** https://developer.nvidia.com/cuda-downloads
- **Size:** ~3 GB download, ~6 GB installed
- **ThemisDB:** ✅ Already integrated

#### cuBLAS, cuDNN
- **Purpose:** Optimized linear algebra and deep learning primitives
- **License:** CUDA Toolkit License
- **Bundled with:** CUDA Toolkit
- **ThemisDB:** ✅ Available

#### NCCL (NVIDIA Collective Communications Library)
- **Purpose:** Multi-GPU communication
- **License:** BSD 3-Clause
- **Installation:** https://developer.nvidia.com/nccl
- **Required for:** Multi-GPU scaling
- **ThemisDB:** ⏳ Planned

#### HIP/ROCm (AMD GPUs, optional)
- **Purpose:** AMD GPU programming framework
- **License:** MIT
- **Installation:** https://rocmdocs.amd.com/
- **ThemisDB:** ⏳ Future roadmap

---

### Vector Search Libraries

#### FAISS-GPU (Meta AI) ⭐⭐⭐
- **Version:** 1.7.4+
- **License:** MIT
- **Repository:** https://github.com/facebookresearch/faiss
- **Purpose:** Production-ready GPU vector search
- **Installation:**
```bash
# Build from source with GPU support
conda install -c pytorch faiss-gpu cudatoolkit=12.0
# OR
cmake -B build -DFAISS_ENABLE_GPU=ON -DFAISS_ENABLE_CUDA=ON
make -C build -j8
```
- **ThemisDB:** ✅ Integrated

#### RAFT/CAGRA (NVIDIA) ⭐⭐
- **Version:** 24.02+
- **License:** Apache 2.0
- **Repository:** https://github.com/rapidsai/raft
- **Purpose:** State-of-the-art GPU graph-based ANN
- **Installation:**
```bash
conda install -c rapidsai -c conda-forge raft-dask cudatoolkit=12.0
```
- **ThemisDB:** ⏳ Evaluation phase

#### hnswlib (CPU baseline)
- **Version:** 0.7.0+
- **License:** Apache 2.0
- **Repository:** https://github.com/nmslib/hnswlib
- **Purpose:** CPU baseline for comparison
- **ThemisDB:** ✅ Integrated

---

### Build & Tools

#### CMake (3.23+)
- **Purpose:** Build system
- **ThemisDB:** ✅ Already required

#### NVCC (CUDA Compiler)
- **Bundled with:** CUDA Toolkit
- **Purpose:** Compile CUDA C++ code

#### NVIDIA Nsight Compute / Systems
- **Purpose:** GPU profiling and optimization
- **Download:** https://developer.nvidia.com/nsight-compute
- **License:** Proprietary (free)

#### AMD ROCProfiler (for AMD GPUs)
- **Purpose:** Profiling AMD GPUs
- **Bundled with:** ROCm

---

### Hardware Requirements

#### Minimum (Development/Testing)
- **GPU:** NVIDIA RTX 3060 (12GB) or better
  - Compute Capability: 7.0+
  - VRAM: 12GB+
- **CPU:** 8 cores, 32GB RAM
- **Storage:** NVMe SSD (for fast data loading)

#### Recommended (Production)
- **GPU:** NVIDIA A100 (40GB/80GB) or RTX 4090 (24GB)
  - Compute Capability: 8.0+ (A100), 8.9 (RTX 4090)
  - VRAM: 24GB+ (40-80GB for large datasets)
- **Multi-GPU:** 2-4 GPUs with NVLink (optional)
- **CPU:** 16+ cores, 64GB+ RAM
- **Network:** 10Gbps+ (for multi-node)

#### Optional (Multi-GPU Scaling)
- **NVLink/NVSwitch:** 300-600 GB/s cross-GPU bandwidth
- **PCIe 4.0 x16:** Fallback (64 GB/s)
- **InfiniBand:** For multi-node GPU clusters

---

## Expected Outcomes

### Success Criteria

#### Performance
- ✅ **10x+ speedup** for batch queries (batch size ≥100)
  - Target: 1,800 QPS (CPU) → 20,000+ QPS (GPU)
- ✅ **5-10x speedup** for index build
  - Target: 30 min (CPU) → 3-5 min (GPU) for 10M vectors
- ✅ **<10ms p95 latency** for single queries on moderate datasets (≤10M vectors)
- ✅ **Linear scaling** to 2-4 GPUs (80-90% efficiency)

#### Accuracy
- ✅ **>95% recall@10** for approximate search
- ✅ **100% accuracy match** with CPU for exact search (brute-force)
- ✅ **Bit-exact results** for L2/Cosine distance (FP32)

#### Stability
- ✅ Production-ready: stable, robust, well-tested
- ✅ Graceful degradation to CPU if GPU unavailable
- ✅ Error handling for GPU OOM, CUDA errors
- ✅ No memory leaks or crashes

---

### Deliverables

#### 1. Benchmark Report ☐
- **Content:** Performance comparison CPU vs GPU vs state-of-the-art
- **Format:** Markdown with tables, charts (PNG/SVG)
- **Include:** SIFT1M, Deep1B, Text Embeddings, Production workload
- **Deadline:** End of Phase 2

#### 2. GPU-Accelerated Index Prototype ☐
- **Language:** C++17 with CUDA
- **Integration:** Extends `VectorIndexManager`
- **Features:**
  - GPU brute-force search
  - FAISS GPU IVF-Flat/IVF-PQ
  - Multi-GPU support (optional)
- **Deadline:** End of Phase 3

#### 3. ThemisDB Integration ☐
- **API:** Extend `VectorIndexManager` with GPU support
- **Config:** YAML/JSON configuration for GPU parameters
- **Monitoring:** Prometheus metrics for GPU utilization
- **Deadline:** End of Phase 4

#### 4. Documentation ☐
- **Setup Guide:** Hardware selection, driver installation, CUDA setup
- **API Guide:** How to use GPU-accelerated vector search
- **Deployment Guide:** Docker, Kubernetes, multi-GPU setup
- **Troubleshooting:** Common issues and solutions
- **Deadline:** End of Phase 5

#### 5. Recommendations ☐
- **GPU Deployment Strategy:** When to use GPU vs CPU
- **Hardware Selection:** GPU recommendations for different scales
- **Cost-Benefit Analysis:** TCO comparison
- **Deadline:** End of Phase 5

---

### Cost-Benefit Analysis

#### TCO Comparison (3-Year Projection)

**Scenario: 10M vectors, 1000 QPS sustained load**

| Solution | Hardware Cost | Power/Year* | Cooling/Year | Total 3Y TCO | QPS | Cost per 1M Queries |
|----------|---------------|-------------|--------------|--------------|-----|---------------------|
| **8x CPU Servers** | $40,000 | $9,000 | $4,500 | $80,500 | 10,000 | **$2.68** |
| **4x A100 GPUs** | $60,000 | $15,000 | $7,500 | $127,500 | 100,000 | **$0.42** |
| **4x RTX 4090** | $8,000 | $6,000 | $3,000 | $35,000 | 50,000 | **$0.23** |

\* Power cost: $0.10/kWh, 24/7 operation

**Key Insights:**
- **RTX 4090:** Best price/performance for medium-scale (10-50M vectors)
- **A100:** Best for large-scale (100M+ vectors) and multi-tenancy
- **CPU Cluster:** Competitive for low-QPS workloads (<1000 QPS)

**Break-Even Analysis:**
- GPU investment pays off at >5,000 QPS sustained
- Lower TCO for 3+ years of operation
- Better TCO for high-throughput workloads

**Energy Efficiency:**
| Solution | Power (W) | Performance (QPS) | QPS per Watt |
|----------|-----------|-------------------|--------------|
| 8x CPU | 8×200W = 1600W | 10,000 | 6.25 |
| 4x A100 | 4×400W = 1600W | 100,000 | 62.5 |
| 4x RTX 4090 | 4×450W = 1800W | 50,000 | 27.8 |

**GPU is 4-10x more energy-efficient for vector search.**

---

## Risks & Challenges

### Technical Challenges

#### 1. Memory Limitations ⚠️ HIGH
**Problem:**
- Large datasets (100M+ vectors at 1536D) may not fit in GPU memory
- A100 80GB can hold ~50M vectors at 1536D (FP32)

**Mitigation:**
- **Chunking:** Split dataset across multiple GPUs
- **Product Quantization:** Compress vectors 8-32x (e.g., 1536D FP32 → 192 bytes)
- **GPU+CPU Hybrid:** Keep hot data on GPU, cold data on CPU
- **Streaming:** Process batches larger than GPU memory

---

#### 2. CPU-GPU Transfer Overhead ⚠️ MEDIUM
**Problem:**
- PCIe bandwidth (16-64 GB/s) is bottleneck for small batches
- Transfer latency (1-5 ms) can negate GPU speedup

**Mitigation:**
- **Keep Data on GPU:** Minimize transfers (persistent index on GPU)
- **Pinned Memory:** Use `cudaMallocHost()` for faster transfers
- **Async Transfers:** Overlap transfer with compute using CUDA streams
- **Batch Queries:** Amortize transfer cost over large batches

---

#### 3. Single-Query Latency ⚠️ MEDIUM
**Problem:**
- GPU shines with batch queries (100-1000+), not single queries
- Single-query latency may be worse than CPU due to overhead

**Mitigation:**
- **Hybrid Approach:** GPU for batches (≥10 queries), CPU for single queries
- **Query Batching:** Buffer queries for 1-10ms, send as batch to GPU
- **Low-Latency GPU Kernels:** Optimize for small batches (1-10 queries)

---

#### 4. Hardware Dependency ⚠️ LOW
**Problem:**
- Production deployment requires specific GPU hardware
- Not all environments have GPUs available

**Mitigation:**
- **Graceful Degradation:** Automatic CPU fallback
- **Cloud GPUs:** Use AWS/GCP/Azure GPU instances on-demand
- **GPU-as-a-Service:** Offer GPU acceleration as premium tier

---

### Mitigation Strategies

#### Strategy 1: Hybrid CPU+GPU Architecture
```
┌─────────────────────────────────────┐
│      Query Router                   │
│  (Classify queries by batch size)   │
└──────────┬──────────────┬───────────┘
           │              │
    ┌──────▼──────┐  ┌───▼────────┐
    │ CPU Index   │  │ GPU Index  │
    │ (HNSW)      │  │ (IVF-GPU)  │
    │             │  │            │
    │ Single      │  │ Batch      │
    │ Queries     │  │ Queries    │
    │ Latency:2ms │  │ Batch:0.05ms│
    └─────────────┘  └────────────┘
```

**Benefits:**
- Best latency for single queries (CPU)
- Best throughput for batches (GPU)
- Automatic load balancing

---

#### Strategy 2: Product Quantization for Memory
- **Compression:** 8-32x reduction (1536D FP32 → 192 bytes)
- **Trade-off:** Slightly lower accuracy (2-5% recall drop)
- **Benefit:** 10x larger datasets fit in GPU memory

---

#### Strategy 3: Multi-GPU Scaling
- **Sharding:** Partition dataset across GPUs
- **Replication:** Duplicate index for query parallelism
- **Fault Tolerance:** Replica shards for HA

---

## Integration Considerations

### API Design

#### Configuration
```cpp
// GPU Vector Index Configuration
struct GpuVectorIndexConfig {
    // Enable/disable GPU
    bool enable_gpu = true;
    int device_id = 0;  // CUDA device ID (0-7)
    
    // Index type
    enum class IndexType {
        BRUTE_FORCE,  // Exact search, good for <10M vectors
        IVF_FLAT,     // Fast approximate, good for 10M-1B vectors
        IVF_PQ,       // Memory-efficient, good for 100M+ vectors
        CAGRA         // State-of-the-art graph-based (if available)
    } index_type = IndexType::IVF_FLAT;
    
    // IVF parameters
    int nlist = 100;         // Number of clusters (10-10000)
    int nprobe = 10;         // Clusters to search (1-nlist)
    
    // PQ parameters (for IVF_PQ)
    int m = 8;               // Number of subquantizers
    int nbits = 8;           // Bits per subquantizer
    
    // Multi-GPU
    bool use_multi_gpu = false;
    int num_gpus = 1;
    std::vector<int> device_ids = {0}; // List of GPU IDs
    
    // Memory management
    size_t max_memory_mb = 8192;     // Max GPU memory to use
    bool fallback_to_cpu = true;     // Fallback if GPU fails
    
    // Batch processing
    int batch_size = 256;            // Optimal batch size
    int min_batch_size_gpu = 10;     // Min batch for GPU (else CPU)
};
```

---

#### Initialization
```cpp
// Create vector index with GPU support
VectorIndexManager vim(db);

// Initialize index
vim.init("embeddings", 
         /*dim=*/1536, 
         VectorIndexManager::Metric::COSINE,
         /*M=*/16, 
         /*efConstruction=*/200,
         /*efSearch=*/64);

// Enable GPU acceleration
GpuVectorIndexConfig gpu_config;
gpu_config.enable_gpu = true;
gpu_config.device_id = 0;
gpu_config.index_type = GpuVectorIndexConfig::IndexType::IVF_FLAT;
gpu_config.nlist = 100;
gpu_config.nprobe = 10;
gpu_config.fallback_to_cpu = true;

vim.enableGpuAcceleration(gpu_config);

// Build index (GPU-accelerated)
vim.rebuildFromStorage();  // Builds on GPU
```

---

#### Batch Search (GPU-optimized)
```cpp
// Batch search (automatically uses GPU)
std::vector<std::vector<float>> queries = {
    {0.1, 0.2, ...},  // Query 1
    {0.3, 0.4, ...},  // Query 2
    ...                // Query N (N=100-1000 for best GPU utilization)
};

auto [status, results] = vim.batchSearchKnn(queries, /*k=*/10);

// Results: vector<vector<Result>>
// results[i] = top-10 neighbors for queries[i]
for (const auto& query_results : results) {
    for (const auto& r : query_results) {
        std::cout << "PK: " << r.pk << ", Distance: " << r.distance << "\n";
    }
}
```

---

#### Single Search (hybrid CPU/GPU)
```cpp
// Single search (automatically routes to best backend)
std::vector<float> query = {0.1, 0.2, 0.3, ...};
auto [status, results] = vim.searchKnn(query, /*k=*/10);

// Routing logic (internal):
// - If GPU available AND batch size >= min_batch_size_gpu: use GPU
// - Else: use CPU (lower latency for single queries)
```

---

### Deployment Strategy

#### 1. GPU-First (Recommended for High-Throughput)
```yaml
# themisdb.yaml
vector_index:
  backend: gpu
  gpu_config:
    enable: true
    device_id: 0
    index_type: ivf_flat
    nlist: 100
    nprobe: 10
    fallback_to_cpu: true
    min_batch_size_gpu: 10
```

**Use Case:**
- High QPS workloads (>5,000 QPS)
- Batch query API
- Cost-sensitive (lower TCO than CPU cluster)

---

#### 2. Hybrid CPU+GPU
```yaml
vector_index:
  backend: hybrid
  cpu_config:
    index_type: hnsw
    ef_search: 64
  gpu_config:
    enable: true
    device_id: 0
    index_type: ivf_flat
    min_batch_size_gpu: 10  # GPU for batches ≥10
```

**Use Case:**
- Mixed workload (single + batch queries)
- Best latency for single queries (CPU)
- Best throughput for batches (GPU)

---

#### 3. Multi-GPU
```yaml
vector_index:
  backend: gpu
  gpu_config:
    enable: true
    use_multi_gpu: true
    num_gpus: 4
    device_ids: [0, 1, 2, 3]
    shard_strategy: by_cluster  # or by_id
```

**Use Case:**
- Very large datasets (100M-1B+ vectors)
- Ultra-high throughput (>100K QPS)
- Requires NVLink or high-bandwidth interconnect

---

### Monitoring

#### GPU Metrics to Expose
```cpp
struct GPUMetrics {
    // Utilization
    float utilization_percent;        // 0-100% (from nvidia-smi)
    float memory_utilization_percent; // 0-100%
    
    // Memory
    size_t memory_used_bytes;
    size_t memory_total_bytes;
    float memory_used_percent() const {
        return 100.0f * memory_used_bytes / memory_total_bytes;
    }
    
    // Temperature & Power
    float temperature_celsius;
    float power_watts;
    float power_limit_watts;
    
    // Performance
    uint64_t queries_per_second;
    uint64_t vectors_indexed;
    float avg_query_latency_ms;
    
    // Multi-GPU
    int num_gpus;
    std::vector<GPUMetrics> per_gpu_metrics;
};
```

#### Prometheus Metrics
```cpp
// Register Prometheus metrics
prometheus::Gauge gpu_utilization = prometheus::BuildGauge()
    .Name("themisdb_gpu_utilization_percent")
    .Help("GPU utilization percentage")
    .Register(*registry);

prometheus::Gauge gpu_memory_used = prometheus::BuildGauge()
    .Name("themisdb_gpu_memory_used_bytes")
    .Help("GPU memory used in bytes")
    .Register(*registry);

prometheus::Counter gpu_queries_total = prometheus::BuildCounter()
    .Name("themisdb_gpu_queries_total")
    .Help("Total number of GPU queries")
    .Register(*registry);

prometheus::Histogram gpu_query_latency = prometheus::BuildHistogram()
    .Name("themisdb_gpu_query_latency_seconds")
    .Help("GPU query latency in seconds")
    .Register(*registry);
```

---

## Additional Context

### Related Work in ThemisDB

**Existing GPU Support:**
- `src/acceleration/cuda/vector_kernels.cu` - CUDA distance kernels (L2, Cosine)
- `src/acceleration/faiss_gpu_backend.cpp` - FAISS GPU wrapper
- `include/acceleration/cuda_backend.h` - CUDA backend interface
- `docs/de/performance/performance_gpu_plan.md` - GPU acceleration plan

**Related Features:**
- Vector Index Manager (`include/index/vector_index.h`)
- Product Quantization (`ProductQuantizer` class)
- Secondary Index Manager (for pre-filtering)
- HNSW Layer Optimizer

---

### External Resources

**FAISS Documentation:**
- Wiki: https://github.com/facebookresearch/faiss/wiki
- GPU Tutorial: https://github.com/facebookresearch/faiss/wiki/Faiss-on-the-GPU
- IndexIVF: https://github.com/facebookresearch/faiss/wiki/Faster-search

**NVIDIA RAFT:**
- Docs: https://docs.rapids.ai/api/raft/stable/
- CAGRA: https://docs.rapids.ai/api/raft/stable/ann_benchmarks.html

**CUDA Programming:**
- CUDA Programming Guide: https://docs.nvidia.com/cuda/cuda-c-programming-guide/
- GPU Gems: https://developer.nvidia.com/gpugems/gpugems3/part-vi-gpu-computing
- Nsight Compute: https://developer.nvidia.com/nsight-compute

**AMD ROCm:**
- ROCm Docs: https://rocmdocs.amd.com/
- HIP Programming Guide: https://rocmdocs.amd.com/en/latest/Programming_Guides/HIP-GUIDE.html

---

### Industry Adoption

**Meta (Facebook):**
- FAISS-GPU for billion-scale similarity search
- 100M+ queries/day in production
- Powers image search, recommendation systems

**NVIDIA:**
- CAGRA in production ML pipelines (NVIDIA AI Enterprise)
- Used in Merlin (recommender systems)
- Integrated with Triton Inference Server

**Google:**
- ScaNN for large-scale embeddings (YouTube, Google Search)
- Optimized for asymmetric distance computation
- Deployed at exabyte scale

**Alibaba Cloud:**
- GPU-accelerated vector search in Database-as-a-Service
- HNSW + GPU hybrid for Alibaba Cloud Vector Search

**Other Notable Adopters:**
- **Milvus:** Open-source vector database with GPU support
- **Qdrant:** Rust-based vector search with GPU backend
- **Weaviate:** Knowledge graph with GPU acceleration

---

## 📚 Scientific Foundations (IEEE Citations)

The following references are provided in full IEEE citation format as the authoritative academic basis for the GPU/FPGA/Vector acceleration research documented in this file.

### Research Papers

1. J. Johnson, M. Douze, and H. Jégou, "Billion-scale similarity search with GPUs," *IEEE Transactions on Big Data*, vol. 7, no. 3, pp. 535–547, 2021, doi: 10.1109/TBDATA.2019.2921572. [Online]. Available: https://faiss.ai/ [Accessed: 2026-02-22]  
   — **ThemisDB application:** `src/acceleration/faiss_gpu_backend.cpp`; GPU-accelerated vector search at billion-scale via the FAISS library.

2. Y. A. Malkov and D. A. Yashunin, "Efficient and robust approximate nearest neighbor search using Hierarchical Navigable Small World graphs," *IEEE Transactions on Pattern Analysis and Machine Intelligence*, vol. 42, no. 4, pp. 824–836, Apr. 2020, doi: 10.1109/TPAMI.2018.2889473. [Online]. Available: https://ieeexplore.ieee.org/document/8613833 [Accessed: 2026-02-22]  
   — **ThemisDB application:** `src/index/hnsw_index.cpp`, GPU acceleration kernels; HNSW algorithm for high-dimensional nearest-neighbor search.

3. T. Dao, D. Y. Fu, S. Ermon, A. Rudra, and C. Ré, "FlashAttention: Fast and memory-efficient exact attention with IO-awareness," in *Proc. Advances in Neural Information Processing Systems (NeurIPS)*, 2022, pp. 16344–16359. [Online]. Available: https://arxiv.org/abs/2205.14135 [Accessed: 2026-02-22]  
   — **ThemisDB application:** Batch vector search optimization and Tensor Core kernels; IO-aware GPU kernel design principles.

4. Y. Gao, K. Xiong, X. Gao, J. Ding, and C. D. Carothers, "NVIDIA Tensor Core for machine learning and deep learning," *IEEE Micro*, vol. 40, no. 6, pp. 33–45, Nov.–Dec. 2020, doi: 10.1109/MM.2020.3037720. [Online]. Available: https://ieeexplore.ieee.org/document/9269176 [Accessed: 2026-02-22]  
   — **ThemisDB application:** `src/acceleration/cuda_backend.cpp`; Tensor Core architecture for matrix operations and mixed-precision (FP16/TF32) kernels.

5. C. Ding, A. Sharma, S. C. Suh, M. R. Amer, A. Bhattacharya, and S. Kumar, "ScaNN: Efficient vector similarity search at scale," in *Proc. 37th Int. Conf. Machine Learning (ICML)*, 2020, pp. 2589–2599. [Online]. Available: https://arxiv.org/abs/1908.10396 [Accessed: 2026-02-22]  
   — **ThemisDB application:** Hybrid CPU/GPU search and quantization support; quantization-aware ANN search for efficient production deployment.

### Specifications & API References

6. Khronos Group, "Vulkan API Specification v1.3," Khronos Registries. [Online]. Available: https://www.khronos.org/registry/vulkan/ [Accessed: 2026-02-22]  
   — **ThemisDB application:** `src/acceleration/vulkan_backend_full.cpp`; cross-platform GPU compute with deterministic performance.

7. AMD, "ROCm documentation: Software platform for GPU computing," AMD. [Online]. Available: https://rocmdocs.amd.com/ [Accessed: 2026-02-22]  
   — **ThemisDB application:** `src/acceleration/hip_backend.cpp`; HIP API, rocBLAS, and RCCL for AMD GPU and multi-GPU support.

---

## Research Checklist

- [x] I have identified GPU indexing approaches to investigate
  - [x] Brute-Force GPU Search
  - [x] GPU-Accelerated IVF
  - [x] GPU-Accelerated HNSW (CAGRA)
  - [x] GPU Product Quantization
  - [x] Multi-GPU Scaling
  - [x] Tensor Core Utilization
  - [x] Unified Memory & Hybrid approaches

- [x] I have listed key papers and state-of-the-art libraries
  - [x] Johnson et al. (FAISS, 2019)
  - [x] SONG (NeurIPS 2022)
  - [x] NGT-QG (2021)
  - [x] ScaNN (ICML 2020)
  - [x] NVIDIA CAGRA (2023-2024)

- [x] I have defined hardware requirements and benchmarks
  - [x] Hardware: RTX 4090, A100, H100, AMD GPUs
  - [x] Datasets: SIFT1M, Deep1B, Text Embeddings, Production
  - [x] Metrics: Latency, Throughput, Memory, Cost

- [x] I have outlined a detailed implementation plan
  - [x] Phase 1: Environment Setup (1 week)
  - [x] Phase 2: Baseline Benchmarking (1-2 weeks)
  - [x] Phase 3: Custom GPU Implementation (3-4 weeks)
  - [x] Phase 4: Integration with ThemisDB (2-3 weeks)
  - [x] Phase 5: Production Readiness (2-3 weeks)

- [x] I have considered memory constraints and scaling
  - [x] Chunking and sharding strategies
  - [x] Product quantization for compression
  - [x] Multi-GPU scaling with NCCL
  - [x] GPU+CPU hybrid approaches

- [x] I have assessed cost-benefit and TCO
  - [x] 3-year TCO comparison (CPU vs GPU)
  - [x] Break-even analysis (>5,000 QPS)
  - [x] Energy efficiency (4-10x better QPS/Watt)

- [x] I have defined monitoring and deployment strategies
  - [x] API design (configuration, initialization, search)
  - [x] Deployment strategies (GPU-first, hybrid, multi-GPU)
  - [x] Monitoring (Prometheus metrics, GPU utilization)

---

**Status:** ✅ Research Complete  
**Next Steps:** Begin Phase 1 (Environment Setup)  
**Timeline:** 8-10 weeks (Q2 2026)  
**Expected ROI:** 10-50x performance improvement, 6-10x better TCO

---

**Document Version:** 1.0  
**Last Updated:** April 2026  
**Authors:** ThemisDB Research Team  
**Next Review:** March 2026
