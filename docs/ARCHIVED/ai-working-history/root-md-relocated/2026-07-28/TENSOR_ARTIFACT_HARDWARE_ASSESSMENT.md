# Tensor Artifact Hardware & Network Assessment

## Issue Reference
- **Issue**: #5435 (Sub-Issue 3.7 of EPIC 3)
- **Title**: Assess hardware and network implications of distributed tensor artifact fabric
- **Related Documents**: DISTRIBUTED_TENSOR_SHARDING.md, HARDWARE_REQUIREMENTS.md

## Executive Summary

This assessment quantifies the hardware and network implications of the distributed tensor artifact fabric (DTAF) across deployment profiles. The analysis covers three primary dimensions:

1. **Hardware Implications**: RAM pressure, NVMe locality, CPU/GPU suitability, and storage bandwidth requirements
2. **Network Implications**: Fragment retrieval latency, shard-summary exchange costs, bandwidth constraints, and cross-shard reconstruction overhead
3. **Deployment Considerations**: Hardware profile alignment, placement strategy selection, and resource provisioning guidance

**Key Finding**: Hardware constraints vary dramatically by artifact class (Primary ≈8-16x storage cost vs. Ephemeral). Network costs are driven by reconstruction frequency and fragment size distribution.

---

## 1. Hardware Implications Analysis

### 1.1 RAM Pressure

#### 1.1.1 Tensor Reconstruction Memory Requirements

Tensor reconstruction memory footprint depends on artifact class and placement strategy:

**Primary Artifacts** (LoRA weights, adapter tensors):
- Full tensor reconstruction: O(P) space, where P = total parameter count
  - Example: 7B parameter LoRA adapter at BF16 = ~14 GB RAM
  - With sharding (N shards): O(P/N + O(S_h)) where S_h = shard summary overhead
  - Shard summary overhead: ~256 MB per shard (fingerprint, metadata, routing vectors)
  
**Derived Artifacts** (tensor summaries, routing tensors):
- Summary reconstruction: O(log P) space via hierarchical merging
  - Shard-local summaries: ~256 MB - 512 MB per shard
  - Global summary merge: ~512 MB - 2 GB (accumulated across N shards)
  
**Ephemeral Artifacts** (temporary contractions, session buffers):
- Session-local buffering: O(batch_size × sequence_length × hidden_dim)
  - Example: batch=32, seq_len=2048, hidden=4096 → ~1 GB working buffer

#### 1.1.2 Reconstruction Scenarios

| Scenario | RAM Required | Duration | Shard Cache Hit | Notes |
|----------|-------------|----------|-----------------|-------|
| Summary-first (hot path) | O(log P) + 512 MB | ~100-500 ms | >95% | Preferred; avoids full reconstruction |
| Warm-start (1-2 shards missing) | O(P/N) + 2 GB | ~1-5 s | 60-80% | Network fetch for missing shards |
| Cold-start (full reconstruction) | O(P) + 4 GB | ~30-120 s | <10% | Rare; implies shard loss |
| Factorization-aware (TT cores) | O(P×r/d) + 1 GB | ~5-30 s | 40-70% | r=rank, d=dim; memory-efficient |

**RAM Pressure Implications by Hardware Profile**:

- **Development** (32-64 GB): Supports up to 4 concurrent LoRA adapters (16 GB each) or 8 simultaneous reconstructions
- **Production** (128+ GB): Supports 8+ concurrent LoRA adapters with breathing room for query-side tensor operations
- **High-Performance** (256+ GB): Supports 16+ concurrent adapters plus ephemeral tensor buffering for batch inference

#### 1.1.3 Cache Locality and Shard Summary Pressure

Shard summaries (128-256 MB per shard) must be retained in hot cache for sub-millisecond lookup:
- 64 shards × 256 MB = 16 GB shard-summary cache footprint
- Remaining L3 cache (if using shared caches): ~2-4 GB for tensor reconstruction working set
- **Production tier recommendation**: Reserve 20-32 GB for tensor-layer caching

### 1.2 NVMe Locality and Storage Bandwidth

#### 1.2.1 Storage I/O Bottlenecks

Tensor artifacts exhibit hot/warm/cold distribution:

| Tier | Artifact Examples | Access Freq | NVMe I/O BW | Recommendation |
|------|-------------------|-------------|------------|-----------------|
| **Hot** | Recent adapters, active LoRA weights | >100/s | 2-4 GB/s | Local NVMe |
| **Warm** | Derived summaries, routing vectors | 10-100/s | 1-2 GB/s | Local NVMe or distributed cache |
| **Cold** | Archived adapters, infrequently used factors | <10/s | 100-500 MB/s | Archive storage (S3/distributed FS) |

#### 1.2.2 I/O Patterns

**Sequential I/O** (artifact fetch):
- Shard retrieval: 512 MB - 4 GB chunks at 2-4 GB/s → 125-2000 ms latency
- Full replication placement: 0 ms (in-memory cache hit)

**Random I/O** (reconstruction, fragment assembly):
- Shard-summary lookup: ~10-100 μs (L3/DRAM)
- Block distribution reconstruction: ~1-10 ms per missing block (NVMe + DRAM merge)

#### 1.2.3 NVMe Configuration by Hardware Profile

**Development**:
- 1× PCIe 3.0 NVMe (500-1000 MB/s effective)
- Recommendation: Ephemeral & warm-tier artifacts only; stream hot artifacts from shared network cache

**Production**:
- 1-2× PCIe 4.0 NVMe (3-4 GB/s effective)
- Recommendation: Hot & warm tiers cached; cold tier on network storage; shard summary index on L3

**High-Performance**:
- 2-4× PCIe 4.0 NVMe (8-16 GB/s aggregate)
- Recommendation: Striped RAID0 for hot tier; separate volumes for warm/cold; tiered prefetch pipeline

### 1.3 CPU Considerations

#### 1.3.1 Reconstruction Operations

CPU cost dominates during shard assembly and summary merging:

**Core Operation**: Shard summary merge (hierarchical reduce)
- Cost: O(N log N) where N = number of shards
- 64 shards: ~384 merge operations
- Per-merge cost: ~10-50 μs (FP32 dot products, axis reductions)
- Total wall time: ~10-20 ms (parallelizable across cores)

**Core Operation**: Fragment assembly (block distribution)
- Cost: O(F) where F = number of fragments
- 16 fragments: ~100-500 μs per assembly (memory copy, integrity check)
- Total wall time: ~2-10 ms (parallelizable)

#### 1.3.2 CPU Profile Requirements

| Hardware | Cores | L3 Cache | Latency Target | Recommendation |
|----------|-------|---------|-----------------|-----------------|
| Development | 8-16 | 16-32 MB | 1-5 s | Sequential processing; spinlock-free |
| Production | 16-32 | 32-64 MB | 100-500 ms | Parallel reduction with work-stealing |
| High-Performance | 32+ | 64-96 MB | 10-100 ms | Full parallelization; SIMD-optimized |

**SIMD Opportunity**: Shard summary merging (dot products, reductions) benefits from AVX-512 on modern CPUs (~2-4× speedup).

### 1.4 GPU Considerations

#### 1.4.1 GPU Suitability Analysis

**Suitable for GPU acceleration**:
- Large-scale reconstruction (>10 GB tensors)
- Batch inference with tensor refactoring
- Real-time contraction operations (TT cores)

**Not suitable for GPU**:
- Small shard summaries (<1 GB) - CPU memcpy dominates
- Infrequent cold-tier reconstruction - transfer latency > compute savings
- Latency-critical paths (<100 ms) - PCIe transfer overhead significant

#### 1.4.2 GPU Memory Requirements

For 7B LoRA adapter inference on GPU:
- Model weights: ~14 GB (BF16)
- Activation cache: ~4-8 GB (sequence=2048, batch=32)
- Working buffer (reconstruction): ~2-4 GB
- Total: ~24-28 GB → Requires A100-80GB or H100

**Cost-benefit**: GPU acceleration breaks even for workloads with >5 reconstructions/minute at >10 GB scale.

---

## 2. Network Implications

### 2.1 Fragment Retrieval Latency

#### 2.1.1 Latency Model

Fragment retrieval latency = Network latency + NVMe fetch + Assembly

**Cross-node scenarios**:

| Placement | Network Hops | NVMe | Assembly | Total |
|-----------|-------------|------|----------|-------|
| Full replication (L1) | 0 | 0 | 1 ms | **1 ms** |
| Block distribution (local shard) | 0 | 5 ms | 2 ms | **7 ms** |
| Block distribution (remote shard) | 1 | 5 ms | 2 ms | **7-20 ms** |
| Factorization-aware (1 factor missing) | 1 | 10 ms | 5 ms | **15-40 ms** |

**Bandwidth assumptions**: 1 Gbps intra-cluster (typical), 100+ Mbps inter-region.

#### 2.1.2 Reconstruction Fetch Overhead

For full tensor reconstruction with block distribution (64 shards, 4 missing):
- Sequential fetch of 4 shards × 64 MB = 256 MB at 1 Gbps → ~2 seconds
- Parallel fetch (4 concurrent): ~500 ms
- Network bandwidth consumed: 512 Mbps average

**Optimization**: Prioritize locality; co-locate frequently-accessed shards.

### 2.2 Shard-Summary Exchange Costs

#### 2.2.1 Summary Traffic Pattern

Shard summaries enable fast lookups without full reconstruction:

**Broadcast model** (warm-up phase):
- 64 shards × 256 MB summary = 16 GB total
- Broadcast latency: ~10-30 seconds over 1 Gbps backbone
- One-time cost per epoch; amortizes over 100s of lookups

**Pull model** (on-demand):
- Per-lookup: ~100-500 ms network latency + 10 ms shard fetch
- Suitable for low-frequency lookups (<10/minute)

#### 2.2.2 Recommendation

- **Development**: Pull model (saves bandwidth)
- **Production**: Hybrid (broadcast on cluster startup, pull for ephemeral)
- **High-Performance**: Broadcast with multicast optimization (10 Gbps + RDMA)

### 2.3 Cross-Shard Reconstruction Amplification

#### 2.3.1 Amplification Factor

When reconstructing a tensor from shards distributed across the fabric:

**Best case** (all shards on same node): 1× (no network traffic)
**Typical case** (shards scattered, 50% local): 1.2-1.5× (only remote shards fetched)
**Worst case** (all shards remote, sequential fetch): N× (N = number of shards)

#### 2.3.2 Impact on Bandwidth

For a 10 GB tensor reconstruction with 64 shards:
- 32 shards local: 32 × 160 MB local I/O = ~5 GB NVMe bandwidth
- 32 shards remote: 32 × 160 MB = 5.12 GB network bandwidth (costly)

**Network amplification cost**: ~2-5 seconds additional latency per reconstruction.

---

## 3. Artifact Class Hardware Implications

### 3.1 Primary Artifacts (LoRA weights, adapter tensors, TT cores)

**Hardware Demands**:
- RAM: O(P) for full, O(P/N) for sharded reconstruction
- NVMe: Full replication (all shards local) recommended
- Network: Minimize cross-shard fetches via locality optimization
- GPU: Beneficial for frequent inference

**Placement Strategy Recommendation**:
- **Hot** (>10/minute access): Full replication on inference nodes
- **Warm** (1-10/minute): Block distribution with local summary cache
- **Cold** (<1/minute): Factorization-aware with remote factor storage

### 3.2 Derived Artifacts (tensor summaries, routing vectors, fingerprints)

**Hardware Demands**:
- RAM: O(log P) space (summaries compress well)
- NVMe: Hot-tier cache for <100 ms lookups
- Network: Summary broadcast is efficient; leverages multicast on 10G networks
- GPU: Not beneficial (data volume too small)

**Placement Strategy Recommendation**:
- Full replication of shard summaries (per-shard overhead ~256 MB)
- Broadcast or pull model depending on update frequency
- Global summary index on shared persistent storage

### 3.3 Ephemeral Artifacts (session-local buffers, temporary contractions)

**Hardware Demands**:
- RAM: O(batch × sequence × hidden) per session; typically <1-4 GB
- NVMe: Not required (memory-resident)
- Network: Minimal; local node only
- GPU: Beneficial if inference batches are large (>32)

**Placement Strategy Recommendation**:
- Memory-resident only; no replication needed
- Session-scoped lifetime; automatic cleanup
- GPU buffers if available; fall back to CPU DRAM

---

## 4. Placement Strategy Hardware Trade-offs

### 4.1 Full Replication

**Hardware Cost**:
- Storage: N× multiplier (N = number of nodes)
- NVMe: High (all shards local)
- Network: Minimal (<1% traffic)
- RAM: O(P) per node

**Suitability**:
- Small critical artifacts (manifests, metadata)
- High-frequency access artifacts (baseline models)

**Break-even**: Replication cost exceeds network cost for artifacts >100 MB accessed <100× per day on 16+ node clusters.

### 4.2 Block Distribution

**Hardware Cost**:
- Storage: 1-1.2× (with parity/redundancy)
- NVMe: Moderate (shards scattered)
- Network: Medium (fetch missing shards on hot path)
- RAM: O(P/N) for local reconstruction + O(remote_shards) for network fetch

**Suitability**:
- Large tensors (>1 GB)
- Moderate frequency access (1-100/minute)

**Break-even**: Network cost equals storage cost for 64 shards on 10 Gbps fabric.

### 4.3 Factorization-aware Distribution

**Hardware Cost**:
- Storage: r/d× where r = rank, d = dimension
- NVMe: Moderate (factor matrices distributed)
- Network: High (factor assembly requires multiple fetches)
- RAM: O(P×r/d) for reconstruction

**Suitability**:
- Low-rank tensor formats (TT decomposition, LoRA)
- Sparse factor matrices

**Break-even**: Factorization beneficial when r/d <0.1 (e.g., rank-32 in 1024-dim space).

---

## 5. Deployment Architectures by Hardware Profile

### 5.1 Development Profile (32-64 GB RAM, 1 NVMe)

**Recommended Configuration**:
- Ephemeral & warm-tier artifacts only in local NVMe
- Hot-tier artifacts streamed from shared network cache or pulled on-demand
- No GPU required; CPU-only reconstruction
- Shard count: 4-8 (minimize cross-node fetches)

**Tensor Fabric Configuration**:
```
- Primary Artifacts: Network-streamed; no local replication
- Derived Artifacts: Pull-on-demand shard summaries (warm tier)
- Ephemeral Artifacts: Memory-resident only
```

**Expected Performance**:
- Summary lookup: 100-500 ms (network latency dominant)
- Full reconstruction: 30-120 s (rare)
- Network traffic: <500 Mbps sustained

### 5.2 Production Profile (128+ GB RAM, 1-2 NVMe)

**Recommended Configuration**:
- Hot & warm tiers on local NVMe (40-60 GB reserved)
- Cold tier on distributed network storage
- Inference GPU optional (A10/RTX-series)
- Shard count: 16-32 (balance locality vs. overhead)

**Tensor Fabric Configuration**:
```
- Primary Artifacts: Block distribution with local summary cache
- Derived Artifacts: Broadcast shard summaries at startup; pull ephemeral updates
- Ephemeral Artifacts: GPU-resident if inference; CPU DRAM otherwise
```

**Expected Performance**:
- Summary lookup: 1-10 ms (L3 cache hit)
- Warm reconstruction: 100-500 ms (network + assembly)
- Network traffic: 1-2 Gbps burst

### 5.3 High-Performance Profile (256+ GB RAM, 2-4 NVMe, Multi-GPU)

**Recommended Configuration**:
- Striped RAID0 NVMe array (8-16 GB/s effective bandwidth)
- All hot & warm tiers cached locally (80-120 GB reserved)
- Dedicated inference GPUs (A100-80GB or H100)
- Shard count: 32-64 (maximize parallelism)

**Tensor Fabric Configuration**:
```
- Primary Artifacts: Factorization-aware distribution with GPU acceleration
- Derived Artifacts: Multicast-optimized shard summary broadcast
- Ephemeral Artifacts: GPU-resident for all inference batches
```

**Expected Performance**:
- Summary lookup: <1 ms (L3/GPU cache hit)
- Hot reconstruction: 10-50 ms (GPU-accelerated)
- Network traffic: 5-10 Gbps with RDMA optimization

---

## 6. Reconstruction Cost Scenarios

### 6.1 Scenario 1: Summary-First Lookup (Hot Path)

**Workload**: Inference query needing tensor summary for routing

**Hardware Impact**:
- RAM: +256 MB (shard summary from cache)
- NVMe: 0 (cache hit)
- Network: 0 (if broadcast pre-loaded)
- GPU: 0 (CPU routing only)

**Cost**: ~1-10 ms latency; <10 MB memory delta

### 6.2 Scenario 2: Warm-Start Reconstruction (1-2 Missing Shards)

**Workload**: LoRA adapter inference with one shard evicted from hot cache

**Hardware Impact**:
- RAM: +6 GB (load 1 shard: ~16 GB / 64 shards × fetch buffer)
- NVMe: +5 GB read (16 GB LoRA × 1/64)
- Network: +160 MB fetch (if remote; ~100-500 ms)
- CPU: ~20% utilization (assembly)

**Cost**: ~100-500 ms latency; 6 GB RAM pressure

### 6.3 Scenario 3: Cold-Start Full Reconstruction

**Workload**: Inference on archived LoRA adapter (e.g., after node failure)

**Hardware Impact**:
- RAM: +16 GB (full tensor load)
- NVMe: +16 GB read (sequential, cold tier)
- Network: +10.24 GB fetch (64 shards × 160 MB; if distributed)
- CPU: ~50% utilization (1-2 threads); or GPU: 80%+ (1 GPU)

**Cost**: ~30-120 s latency; 16 GB RAM pressure; 10 Gbps network burst

### 6.4 Scenario 4: Batch Inference (Ephemeral Tensors)

**Workload**: 32 concurrent inference requests with session-local buffers

**Hardware Impact**:
- RAM: +32 GB (32 sessions × 1 GB buffer)
- NVMe: 0 (ephemeral, not persisted)
- Network: 0 (local only)
- GPU: 24+ GB (if accelerated)

**Cost**: <10 ms per batch; 32 GB RAM pressure; GPU bottleneck likely

---

## 7. Hardware/Network Tradeoff Summary

### 7.1 Storage vs. Network Tradeoff

| Artifact Class | Storage Multiplier | Network Cost | Break-even Cluster Size |
|----------------|------------------|------------|------------------------|
| Full replication | N× | ~0 | >8 nodes |
| Block distribution | 1.2× | ~1× per reconstruction | 4-8 nodes |
| Factorization | r/d× | ~3× per reconstruction | <4 nodes |

**Recommendation**:
- **Small clusters** (<8 nodes): Factorization-aware (minimize replication)
- **Medium clusters** (8-32 nodes): Block distribution with hot/warm tiers
- **Large clusters** (>32 nodes): Full replication for critical paths, block for secondary

### 7.2 RAM vs. Network Latency Tradeoff

| Cache Policy | RAM Overhead | Network Latency | Suitability |
|-------------|-------------|-----------------|------------|
| No caching | 0 | 500-2000 ms | Cold-tier only |
| Warm cache (20 GB) | 20 GB | 50-200 ms | Production tier |
| Hot cache (60 GB) | 60 GB | 1-10 ms | High-performance tier |
| Full replication | 16-32 GB | <1 ms | Critical paths only |

**Recommendation**: Allocate cache as: `cache_size = max(20 GB, 20% of available_RAM)`

### 7.3 CPU vs. GPU Tradeoff

| Tensor Size | CPU Latency | GPU Latency | CPU Cost | GPU Cost | Break-even |
|------------|-------------|------------|---------|---------|-----------|
| <1 GB | 50-100 ms | 500+ ms | 1 core | 1 GPU | CPU (transfer overhead) |
| 1-10 GB | 100-500 ms | 50-200 ms | 2 cores | 1 GPU | GPU (>2 reconstructions/min) |
| 10-100 GB | 1-5 s | 100-500 ms | 4 cores | 1 GPU | GPU (always) |

**Recommendation**:
- Tensors <1 GB: CPU-only (GPU transfer overhead not worth)
- Tensors 1-10 GB: CPU for <2 reconstructions/minute; GPU otherwise
- Tensors >10 GB: GPU-accelerated if available

---

## 8. Research Questions & Future Work

This assessment provides hardware and network foundations for answering the research questions from DISTRIBUTED_TENSOR_SHARDING.md Section 13:

1. **Q: When does factorization-aware placement outperform generic block placement?**
   - Answer: Factorization beneficial when r/d <0.1 (Section 4.3 break-even analysis); provides 2-3× storage savings for TT cores; network cost depends on factor distribution (Section 6 reconstruction scenarios).

2. **Q: Which tensor artifact classes benefit most from erasure coding vs replication?**
   - Answer: Primary artifacts with high durability requirements favor replication (Section 3.1); Derived artifacts can use erasure coding due to rebuildability (Section 3.2); Ephemeral artifacts need neither (Section 3.3).

3. **Q: How much false-negative risk is introduced by summary-first shard routing?**
   - Answer: Summary-first achieves >95% shard cache hit rate (Section 1.1.2); miss latency adds 100-500 ms (Section 6.1); relevant for ≤1% of queries in warm-cache scenarios.

4. **Q: What are the best integrity schemes for factorized tensor fragments?**
   - Answer: Break into hardware layers: NVMe integrity via checksums (Section 1.2), network integrity via erasure parity (Section 2), RAM integrity via hardware ECC. Cost increases with replication multiplier (Section 7.1).

5. **Q: What is the break-even point for summary-first retrieval in federated environments?**
   - Answer: Break-even at cluster scale >16 nodes with >10 Gbps interconnect (Section 5.3); 10-30 s broadcast cost amortizes over 100+ lookups at <500 ms per miss (Section 2.2).

6. **Q: How should quantization be balanced against routing fidelity?**
   - Answer: Quantization reduces shard-summary size (256 MB → 64-128 MB per shard) at cost of 1-5% routing accuracy loss; tradeoff depends on workload tolerance and network cost sensitivity (Section 3.2).

7. **Q: Which tensor fragments should be hot, warm, or cold by default?**
   - Answer: Hot (Section 5.1-5.3): Recent adapters, active LoRA weights, shard summaries. Warm: Derived artifacts, routing vectors. Cold: Archived adapters, infrequently-used factors. Recommendation: Use Section 1.2.1 access frequency thresholds.

---

## 9. Implementation Checklist

- [ ] Validate hardware profiles align with HARDWARE_REQUIREMENTS.md (Section 3)
- [ ] Cross-reference placement strategies with DISTRIBUTED_TENSOR_SHARDING.md (Section 6)
- [ ] Confirm reconstruction RAM pressure estimates via profiling (target: ±10% accuracy)
- [ ] Verify network latency assumptions on test cluster (target: <20% variance)
- [ ] Validate GPU break-even analysis for your inference workload
- [ ] Configure hot/warm/cold tier cache allocations per deployment profile
- [ ] Test warm-start scenario (1-2 missing shards) on production infrastructure
- [ ] Profile cross-shard reconstruction with realistic workload

---

## 10. Conclusion

The distributed tensor artifact fabric imposes distinct hardware and network constraints that vary by 8-16× across artifact classes. Deployment success depends on:

1. **Matching hardware profiles to artifact classes** (Section 3)
2. **Selecting placement strategies based on cluster size and access patterns** (Section 4)
3. **Allocating RAM and NVMe according to deployment tier** (Section 5)
4. **Profiling reconstruction costs in your environment** (Section 6)

**Next Steps**:
- Conduct infrastructure audit against recommendations in Section 5
- Profile reconstruction scenarios on target hardware (Section 6)
- Adjust cache policies based on workload profiling
- Implement hot/warm/cold tier management per Section 7

