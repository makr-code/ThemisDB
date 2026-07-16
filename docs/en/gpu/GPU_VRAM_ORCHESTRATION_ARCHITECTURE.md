# GPU/VRAM Orchestration Architecture

**Issue:** [#5383 — GPU/VRAM Architekturdiagramm und Refactoring-Konzept dokumentieren](https://github.com/makr-code/ThemisDB/issues/5383)  
**Status:** Current as of v2.4 (Multi-GPU phase)  
**Last Updated:** 2026-06-30  
**Scope:** Full GPU/VRAM stack — backend selection, memory management, kernel dispatch, and multi-GPU coordination

---

## Overview

ThemisDB provides a multi-backend GPU acceleration layer that spans vector indexing, graph traversal, geospatial operations, LLM inference, and LoRA fine-tuning.  
All GPU resources are managed through a unified VRAM orchestration stack that enforces edition limits, tenant isolation, memory pressure thresholds, and graceful CPU fallback.

---

## System Architecture Diagram

```mermaid
flowchart TD
    %% ── Entry Points ────────────────────────────────────────────────────────
    subgraph Callers["Entry Points"]
        VQ["Vector Query\n(GPUVectorIndex)"]
        GQ["Graph Query\n(GPUTraversal)"]
        GEO["Geo Query\n(GPUGeoBackend)"]
        LLM["LLM / LoRA\n(LoRATrainingService)"]
        IDX["Index Manager\n(IndexManager)"]
    end

    %% ── Dispatch Layer ──────────────────────────────────────────────────────
    subgraph Dispatch["Kernel Dispatch Layer"]
        AHD["AiHardwareDispatcher\n(NPU priority chain)"]
        KFD["KernelFallbackDispatcher\n(GPU → CPU fallback)"]
        MGR["BackendRegistry\n(capability matching)"]
    end

    %% ── Backend Implementations ─────────────────────────────────────────────
    subgraph Backends["Compute Backends  (IComputeBackend)"]
        CUDA["CUDABackend\nNVIDIA · CC ≥ 7.0"]
        VK["VulkanBackend\nCross-platform"]
        HIP["HIPBackend\nAMD ROCm"]
        DX["DirectXBackend\nWindows only"]
        MTL["MetalBackend\nmacOS / iOS"]
        CL["OpenCLBackend\nLegacy"]
        MGPU["MultiGPUBackend\nNCCL / RCCL sharding"]
        CPU["CPUBackend\nSIMD fallback\n(always available)"]
    end

    %% ── Kernel Libraries ────────────────────────────────────────────────────
    subgraph Kernels["GPU Kernel Libraries"]
        VK_C["vector_kernels.cu\nL2 · Cosine · InnerProduct\nBitonic top-k"]
        GK_C["graph_kernels.cu\nHNSW traversal\nBFS / random walk"]
        TM_C["tensor_core_matmul.cu\nTensor Core GEMM"]
        ANN_C["ann_kernels.cu\nANN search"]
        GEO_C["geo_kernels.cu\nGeospatial distance\n& containment"]
        LK_C["lora/cuda_kernels.cu\nBF16 · FP16\nFused · Flash kernels\nQuantization"]
        HLSL["HLSL Shaders\n(DirectX)\nmatmul · gradient\nembedding_lookup"]
    end

    %% ── VRAM Memory Stack ───────────────────────────────────────────────────
    subgraph Memory["VRAM Memory Stack"]
        GMM["GPUMemoryManager\n(singleton · edition limits\ntenant isolation · stats)"]
        MP["MemoryPool\n(slab allocator\nfragmentation control)"]
        VA["VRAMAllocator\n(LoRA per-layer budgets)"]
        UM["UnifiedMemory\n(CUDA managed memory)"]
        MO["GPUMemoryOversubscription\n(index eviction policy)"]
        VSC["VRAMSecureClear\n(security: zeroise on free)"]
    end

    %% ── Observability ───────────────────────────────────────────────────────
    subgraph Observe["Observability & Safety"]
        KV["KernelValidator\n(integrity check)"]
        MP2["MemoryPressureMonitor\n(OOM threshold alerts)"]
        SF["GPUSafeFail\n(OOM recovery · eviction)"]
        PROF["GPUProfiler\n(event timing · utilization)"]
        AUDIT["VRAMAuditLog\n(tenant usage accounting)"]
    end

    %% ── Multi-GPU Coordination ──────────────────────────────────────────────
    subgraph MultiGPU["Multi-GPU Coordination"]
        MIG["MIGManager\n(A100 MIG slices)"]
        P2P["P2PTransfer\n(NVLink / PCIe)"]
        SCH["TimeSliceScheduler\n(QoS priority queues)"]
        LB["GPULoadBalancer\n(least-loaded dispatch)"]
    end

    %% ── Connections ─────────────────────────────────────────────────────────
    Callers --> Dispatch

    AHD -->|"NPU available"| CUDA
    AHD -->|"NPU unavailable"| KFD
    KFD -->|"GPU available"| MGR
    KFD -->|"GPU OOM / absent"| CPU
    MGR --> CUDA & VK & HIP & DX & MTL & CL & MGPU

    CUDA --> VK_C & GK_C & TM_C & ANN_C & GEO_C & LK_C
    VK --> VK_C & GK_C
    HIP --> VK_C & ANN_C
    DX --> HLSL
    MGPU --> CUDA & HIP

    CUDA & VK & HIP & MGPU --> Memory

    GMM --> MP & VA & UM & MO & VSC
    GMM --> Observe

    MP2 --> SF
    SF -->|"evict / degrade"| CPU
    KV -->|"fail-closed"| SF

    MGPU --> MultiGPU
    LB --> CUDA & HIP
    SCH --> LB
    P2P --> CUDA & HIP
    MIG --> CUDA
```

---

## Layer Descriptions

### Entry Points

| Caller | Primary Operation | GPU Backend Used |
|--------|------------------|-----------------|
| `GPUVectorIndex` | ANN / k-NN vector search | CUDA, Vulkan, HIP, CPU |
| `GPUTraversal` | HNSW / BFS graph traversal | CUDA (graph_kernels) |
| `GPUGeoBackend` | Geospatial distance & containment | CUDA (geo_kernels) |
| `LoRATrainingService` | Fine-tuning weight updates | CUDA (lora kernels) |
| `IndexManager` | Index build / rebuild under memory pressure | CUDA + oversubscription |

### Dispatch Layer

| Component | Responsibility |
|-----------|---------------|
| `AiHardwareDispatcher` | Tries NPU (Apple Neural Engine, Intel NPU, Qualcomm QNN, ARM Ethos) first; falls back to GPU chain |
| `KernelFallbackDispatcher` | GPU→CPU degradation on OOM or device absence |
| `BackendRegistry` | Matches `CapabilityRequirements` (GEMM, ANN, graph, geo) to registered backends |

### Compute Backends

All backends implement `IComputeBackend` (contract version 1.0).  
Binary compatibility is verified via `BACKEND_CONTRACT_VERSION` at runtime.

| Backend | Platform | Compute Capability |
|---------|----------|--------------------|
| `CUDABackend` | NVIDIA (CC ≥ 7.0) | Tensor Cores, CUDA Graphs, cuBLAS |
| `VulkanBackend` | Cross-platform | Compute shaders, SPIR-V |
| `HIPBackend` | AMD (ROCm) | RCCL, rocBLAS |
| `DirectXBackend` | Windows | HLSL Compute Shaders (CS 5.1 / 6.0) |
| `MetalBackend` | macOS / iOS | Metal Performance Shaders |
| `OpenCLBackend` | Legacy | OpenCL 2.x |
| `MultiGPUBackend` | Multi-device | NCCL / RCCL all-reduce |
| `CPUBackend` | Always | AVX-512, AVX2, NEON (SIMD) |

### VRAM Memory Stack

```
┌─────────────────────────────────────────────────────────────┐
│  GPUMemoryManager (singleton)                               │
│  ├── Edition VRAM limit (compile-time, -DTHEMIS_EDITION)    │
│  ├── Per-tenant quota registry (SetTenantQuota)             │
│  ├── Allocation ledger (AllocationRecord per tag)           │
│  └── Stats: allocated / peak / count / dealloc count        │
│                                                             │
│  MemoryPool (slab-based)                                    │
│  ├── Size-class buckets (power-of-two slabs)                │
│  └── Fragmentation monitoring                               │
│                                                             │
│  VRAMAllocator (LoRA-specific)                              │
│  └── Per-layer weight budgets for fine-tuning               │
│                                                             │
│  UnifiedMemory (CUDA managed)                               │
│  └── Automatic host↔device migration                       │
│                                                             │
│  GPUMemoryOversubscription (index eviction)                 │
│  └── Eviction policy: LRU / priority                        │
│                                                             │
│  VRAMSecureClear                                            │
│  └── Zero-fill on dealloc (security hardening)             │
└─────────────────────────────────────────────────────────────┘
```

### Observability & Safety

| Component | Function |
|-----------|----------|
| `KernelValidator` | Validates kernel integrity before execution; fail-closed on mismatch |
| `MemoryPressureMonitor` | Monitors VRAM usage against configurable OOM thresholds |
| `GPUSafeFail` | Executes eviction and graceful CPU degradation on OOM |
| `GPUProfiler` | Records kernel execution time, SM occupancy, memory bandwidth |
| `VRAMAuditLog` | Tracks per-tenant VRAM accounting for billing/compliance |

### Multi-GPU Coordination

| Component | Function |
|-----------|----------|
| `MIGManager` | Manages NVIDIA A100 MIG slices for hard tenant isolation |
| `P2PTransfer` | Peer-to-peer device memory copies via NVLink or PCIe |
| `TimeSliceScheduler` | Priority-queue based GPU time-slice scheduling (QoS) |
| `GPULoadBalancer` | Routes queries to least-loaded GPU device |

---

## Data Flow: Vector Query Path

```mermaid
sequenceDiagram
    participant App as Application
    participant GVI as GPUVectorIndex
    participant KFD as KernelFallbackDispatcher
    participant MEM as GPUMemoryManager
    participant CUDA as CUDABackend
    participant K as vector_kernels.cu

    App->>GVI: search(query, topK)
    GVI->>KFD: dispatch(VectorSearch, capability)
    KFD->>MEM: TryAllocateGPU(scratch_bytes, "vector_search")
    alt VRAM available
        MEM-->>KFD: handle
        KFD->>CUDA: ExecuteKnnQuery(query, index, topK)
        CUDA->>K: l2_distance_kernel<<<grid, block>>>()
        CUDA->>K: bitonic_topk_kernel<<<grid, block>>>()
        K-->>CUDA: device results
        CUDA->>MEM: DeallocateGPU(handle)
        CUDA-->>GVI: KnnQueryResult[]
    else VRAM exhausted / OOM
        MEM-->>KFD: AllocationError
        KFD->>KFD: fallback to CPUBackend
        KFD-->>GVI: KnnQueryResult[] (CPU path)
    end
    GVI-->>App: results
```

---

## Edition VRAM Limits

| Edition | Max VRAM | Tenant Isolation | MIG Support |
|---------|----------|-----------------|-------------|
| Minimal | 4 GB | No | No |
| Community | 16 GB | Soft quotas | No |
| Enterprise | 80 GB | Hard quotas | Yes (A100) |
| Hyperscaler | Unlimited | Per-MIG-slice | Yes (full) |
| Military | Configurable | Air-gap enforced | Yes |

---

## Related Documentation

- [GPU_MASTER_TRACKING.md](GPU_MASTER_TRACKING.md) — Implementation status across all GPU backends
- [GPU_VECTOR_INDEXING_ARCHITECTURE.md](GPU_VECTOR_INDEXING_ARCHITECTURE.md) — Detailed vector indexing design
- [GPU_VRAM_REFACTORING_DESIGN.md](GPU_VRAM_REFACTORING_DESIGN.md) — Planned refactoring: Memory Manager consolidation, Kernel/Shader unification, OOP/SoC improvements
- [MULTI_GPU_IMPLEMENTATION_SUMMARY_V2.md](MULTI_GPU_IMPLEMENTATION_SUMMARY_V2.md) — Multi-GPU coordination details
- [../../architecture/GPU_ARCHITECTURE_REVIEW_TEMPLATE.md](../../architecture/GPU_ARCHITECTURE_REVIEW_TEMPLATE.md) — Architecture review template
- **Main Issue:** [#5383](https://github.com/makr-code/ThemisDB/issues/5383)
