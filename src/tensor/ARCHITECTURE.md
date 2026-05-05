# Tensor Module — Architecture

<!-- Status: current | validated: 2026-05-05 | updated: 2026-05-05 aligned with paper -->

> **Scientific Basis:** This architecture is aligned with
> *"Advanced Architectural Paradigms for Multi-Model AI Databases: Integrating Tensor Graph
> Networks and Zero-Copy Inference in ThemisDB"* — ThemisDB Research Group, 2026.
> Internal research report (pre-print); venue and DOI pending publication.
> Companion docs: `docs/research/best_practices/tensor_train_storage.md`,
> `docs/research/HNSW_FAISS_TT_BOUNDARY_ANALYSIS.md`.

## Module Position in ThemisDB

```
┌────────────────────────────────────────────────────────────────────────────────┐
│ ThemisDB Core Engine                                                           │
│                                                                                │
│  Query Layer                                                                   │
│  ┌──────────────────────────────────────────────────────────────────────────┐  │
│  │ AQL Query Engine                                                         │  │
│  │  ├── TensorAwareQueryOptimizer  (Phase 3)                                │  │
│  │  │    └── TENSOR_CONTRACTION plan-node                                   │  │
│  │  ├── TensorContractionEngine   (src/query/)                              │  │
│  │  └── AQL operators: CONTRACT, PROJECT, DECOMPOSE  (Phase 3)              │  │
│  └──────────────────────────────────────────────────────────────────────────┘  │
│           │                        │                                            │
│           ▼                        ▼                                           │
│  ┌──────────────────┐   ┌──────────────────────────────────────────────────┐   │
│  │  src/index/      │   │  src/tensor/  ◄── this module                    │   │
│  │  (HNSW, FAISS,   │   │                                                  │   │
│  │   ScaNN, DiskANN)│   │  TensorIndexManager                              │   │
│  │                  │   │   ├── FlatTensorIndex (Phase 1, TT)              │   │
│  │  HNSW nav layer  │◄──┼── HnswTTBridge (Phase 2 HYBRID)                 │   │
│  │  (sketch-based)  │   │   ├── HierarchicalTuckerIndex (Phase 5, HT)      │   │
│  │                  │   │   ├── AdapterRepository (Phase 3, LoRA sovereign) │   │
│  └──────────────────┘   │   └── [GPU-TT cuSOLVER index Phase 4]            │   │
│                          └──────────────────────────────────────────────────┘   │
│                                    │                                            │
│                                    ▼                                            │
│  ┌──────────────────────────────────────────────────────────────────────────┐   │
│  │  src/storage/                                                            │   │
│  │   ├── TensorTrainDecomposer       — TT-SVD (Oseledets 2011) + LAPACK    │   │
│  │   ├── HierarchicalTuckerDecomposer — HT binary tree (Phase 5)           │   │
│  │   ├── QuanticsTTDecomposer         — QTT log-scaling (Phase 5)          │   │
│  │   ├── TensorNetworkStorageEngine   — RocksDB persistence + TensorCompactionFilter│
│  │   ├── TTQuantizer                  — INT8/NF4 quantization of TT cores  │   │
│  │   ├── TensorRouter                 — routing decision (HNSW/TT/HT/HYBRID)│  │
│  │   ├── TensorButterflyOperator      — O(n·d) oscillatory integrals (Phase 3)│ │
│  │   └── GgmlTensorBridge             — zero-copy mmap + GGUF v3 (Phase 3) │   │
│  └──────────────────────────────────────────────────────────────────────────┘   │
│                                    │                                            │
│                                    ▼                                            │
│  ┌──────────────────────────────────────────────────────────────────────────┐   │
│  │  src/tensor/ — Adaptive Structural Layer (Phases 6-8)                   │   │
│  │   ├── HissStructuralSearchEngine  — TN-SS + entropy clustering          │   │
│  │   ├── TensorNetworkStructuralRounding (TNSR) — background topology opt  │   │
│  │   ├── TemplateCatalog              — domain template graph registry      │   │
│  │   ├── UTRConverter                 — multi-modal UTR encoding            │   │
│  │   └── VlasovMaxwellSolver          — physics-informed spectral solver    │   │
│  └──────────────────────────────────────────────────────────────────────────┘   │
│                                    │                                            │
│                                    ▼                                            │
│  ┌──────────────────────────────────────────────────────────────────────────┐   │
│  │  src/graph/                                                              │   │
│  │   ├── TensorFingerprintGraph — LSH+MinHash similarity graph              │   │
│  │   └── TensorDeduplicationManager — single-instance TT storage           │   │
│  └──────────────────────────────────────────────────────────────────────────┘   │
│                                    │                                            │
│                                    ▼                                            │
│  ┌──────────────────────────────────────────────────────────────────────────┐   │
│  │  RAG / Inference Layer                                                   │   │
│  │   ├── FLARERetriever   — mid-generation iterative retrieval (Phase 3)    │   │
│  │   ├── TARGGate         — logit-gap gating (Phase 3)                      │   │
│  │   └── SnapshotLearner  — scientific RAG operator inference (Phase 8)     │   │
│  └──────────────────────────────────────────────────────────────────────────┘   │
└────────────────────────────────────────────────────────────────────────────────┘
```

## SOC Boundary with src/index

The `src/tensor/` module maintains a clean separation from `src/index/`:

| Concern | src/index | src/tensor |
|---------|-----------|------------|
| Graph structure | HNSW adjacency graph | — |
| Distance computation | Exact float32 L2/cosine | TT-domain O(d·r²), HT-domain O(d·n·r²+d·r⁴) |
| Storage format | Flat float32 arrays | TT/HT/QTT core chains |
| Persistence | Flat file / RocksDB | TT-core RocksDB schema + rank-adaptive compaction |
| GPU acceleration | CUDA/HIP/Vulkan vector ops | cuSOLVER TT-SVD (Phase 4); HT tensor cores (Phase 5) |
| GGML integration | — | Zero-copy mmap bridge (Phase 3) + GGUF v3 metadata |
| Structural optimization | — | Hiss/TNSR adaptive rounding (Phase 6) |
| Multi-modal encoding | — | UTR converter (Phase 7) |
| Physics simulation | — | Vlasov-Maxwell spectral solver (Phase 8) |

**Interface between modules (HYBRID mode):**
`HnswTTBridge` calls `src/index` HNSW for graph navigation over sketches, then
`src/tensor` TT arithmetic for re-ranking.  The dependency direction is:
`src/tensor → src/index` (tensor depends on index, not vice versa).

## Tensor Format Comparison (from Paper)

| Complexity | Traditional Matrix | Tensor Train (TT) | Hierarchical Tucker (HT) |
|---|---|---|---|
| Storage Scaling | O(n^d) | O(d·n·r²) | O(d·n·r + d·r³) |
| Contraction Complexity | O(n^d) | O(d·n·r³) | O(d·n·r² + d·r⁴) |
| Parallelization | Moderate | Low (Sequential Chain) | High (Tree Structure) |
| Multi-scale Capture | Low | Moderate | High |

- **TT** is preferred for sequential/chain-structured data (LLM embeddings, 1D fields)
- **HT** is preferred for multi-scale data and when parallel tree contractions are needed
- **QTT** is preferred for oscillatory integrals and multi-scale phenomena (wave equations)
- Routing decision made by `TensorRouter::route()` based on compressibility κ and data class

## Data Flow — addFlat()

```
addFlat(id, float* vec, dim)
        │
        ▼
TensorTrainDecomposer::decompose(vec, shape={dim}, cfg)
        │ TTTrain (chain of TT-cores Gₖ)
        ▼
FlatTensorIndex::add(id, train)     OR     HnswTTBridge::add(id, train)
        │                                           │              │
        │ store in hash-map                  extractSketch    store TTTrain
        │                               (first-core mean)         │
        │                                           │              │
        │                                  HNSW::insert(id, sketch)
        ▼                                           ▼
   [in-memory store]                   [HNSW graph + TT store]
```

**Phase 2 addition:** `TensorCompactionFilter` may re-compress stored cores during
background compaction if local entropy < threshold and ε budget allows.

## Data Flow — searchFlat()

```
searchFlat(float* query, dim, k)
        │
        ▼
Decompose query → TTTrain_q
        │
        ├─── FlatTensorIndex (pure TT, Phase 1)
        │    └── Linear scan: cosine(TTTrain_q, TTTrain_i) for all i
        │        O(n · d · r²)
        │
        └─── HnswTTBridge (HYBRID, Phase 2)
             ├── Extract sketch from TTTrain_q (first core)
             ├── HNSW::search(sketch, ef=rerank_candidates) → candidate IDs
             └── TT re-rank: cosine(TTTrain_q, TTTrain_cand) O(C · d · r²)
                 where C = rerank_candidates << n
                 Paper accuracy: 91% hybrid-TT vs. 73% dense baseline
```

## Zero-Copy GGML Bridge (Phase 3)

```
ThemisDB                           llama.cpp / llama-server
─────────                          ────────────────────────
TensorIndexManager
  ggmlCorePtrs(tenant, coll,
               field, id)
        │
        │ raw float* pointers
        │ (TT-core data, already
        │  resident in RAM/VRAM)
        ▼
GgmlTensorBridge::inject(ptrs)
        │ ggml_tensor* (GGML_TYPE_TT)
        │ pointing to ThemisDB memory
        │ NO COPY — shared address space
        │ Apple Silicon: unified memory pool
        │ (CPU, GPU, ANE share same RAM)
        ▼
ggml_build_forward() injects
TT-core as graph input node
        │
        ▼
Forward pass: O(d·r²) contraction
with model weights uses TT-cores
directly from DB address space.
Paper TTFT: 40-90ms vs 150-400ms standard.

GGUF v3 metadata attached to each core:
  source.filename, source.page, source.line,
  source.doc_id, source.tenant_id,
  source.ingest_timestamp
  (required for regulated-industry traceability)
```

## FLARE + TARG Retrieval Paradigms (Phase 3)

```
                    ┌─────────────────────────────────────────────┐
                    │  Retrieval Strategy Selection                │
                    │                                             │
          ┌─────────┤  TARG: compute logit gap (top-1 − top-2)   │
          │         │  if gap < threshold → retrieve              │
          │         │  else → continue generation (Never-RAG)     │
          │         │  Eliminates 70-90% unnecessary calls        │
          │         └─────────────────────────────────────────────┘
          │
          ▼ (retrieval triggered)
  TensorIndexManager::search(query_tt, k, tenant_id)
          │
          ▼
  Zero-copy inject via GgmlTensorBridge
          │
          ▼
  Generation continues with retrieved TT-context

  FLARE path (mid-generation):
    monitor per-token log-prob during generation
    → low confidence → pseudo-sentence query
    → retrieve from TT index (≤ 90ms round-trip per paper)
    → regenerate uncertain passage with new context
```

## Hiss / TNSR Adaptive Structural Rounding (Phase 6)

```
HissStructuralSearchEngine
  ├── stochastic sub-network sampling (entropy-guided)
  ├── hierarchical local refinement
  └── targeted index reshaping to expose QTT structures
         │
         ▼
  TensorNetworkGraph (optimized topology for data class)
         │
         ├── registered in TemplateCatalog by domain_tag
         │   (re-use within 10% perf on similar instances)
         │
         └── executed by TensorNetworkStructuralRounding (TNSR)
                as background maintenance task in RocksDB
                compaction thread pool:
                - adjusts bond dimensions (like rank-adaptive filter)
                - reconfigures tree topology (unique to TNSR)
                - storage reduction ≥ 15% over 24h
                - cosine accuracy δ < 0.001
```

## UTR Multi-Modal Architecture (Phase 7)

```
Heterogeneous Data Sources
         │
         ▼
UTRConverter (unified entry point)
  ├── fromGeospatial()   → TTTrain   (topological proximity preserved)
  ├── fromTabular()      → HyperIndexTensor  (latent join discovery)
  ├── fromImage()        → TTTrain   (structural similarity in freq space)
  └── fromDocument()     → HTTrain   (hierarchical paragraph → child-to-parent)
         │
         ▼
TensorIndexManager::add(id, tensor_native_repr)
         │
         ▼
All downstream query, search, GGML-bridge paths unchanged
(UTR output is a valid TTTrain or HTTrain)
```

## Key Design Decisions

1. **Phase-1 `FlatTensorIndex` is a known O(n) linear scan.**  This is
   intentional: the interface is stable, the algorithm is correct, and
   Phase-2 will swap the backend without changing the public API.

2. **`HnswTTBridge` depends on `src/index` for navigation, not vice versa.**
   This preserves the independence of the existing HNSW module.

3. **TT arithmetic (inner-product, norm) is duplicated between
   `FlatTensorIndex` and `HnswTTBridge`.**  A shared `TTArithmetic` utility
   will be extracted in Phase 2 to eliminate the duplication.

4. **Tenant isolation** follows the same `"<prefix>:<tenant_id>:<name>"`
   convention as `src/index/IndexManager`.

5. **HT is NOT a replacement for TT** — both coexist.  `TensorRouter` selects
   the format based on compressibility κ, data class, and workload type
   (sequential chain vs. tree parallelism).

6. **Hiss/TNSR are background-only** — they never block foreground queries.
   The TNSR result is committed atomically via RocksDB WriteBatch only if
   `bytes_saved > min_bytes_saved_to_commit` (configurable).

7. **Zero-copy mmap bridge assumes page-aligned core storage.**  `TensorNetworkStorageEngine`
   must allocate core buffers aligned to `sysconf(_SC_PAGESIZE)` when
   `THEMIS_ENABLE_GGML_BRIDGE` is defined.

8. **GGUF v3 metadata provenance** is a hard requirement for regulated-industry
   deployments.  All code paths that write TT-cores to storage MUST attach
   `ProvenanceRecord` metadata.
