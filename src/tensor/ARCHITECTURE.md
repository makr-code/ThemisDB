# Tensor Module — Architecture

<!-- Status: current | validated: 2026-05-05 -->

## Module Position in ThemisDB

```
┌────────────────────────────────────────────────────────────────────────┐
│ ThemisDB Core Engine                                                   │
│                                                                        │
│  Query Layer                                                           │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │ AQL Query Engine                                                 │  │
│  │  ├── TensorAwareQueryOptimizer  (Phase 3)                        │  │
│  │  │    └── TENSOR_CONTRACTION plan-node                           │  │
│  │  └── TensorContractionEngine   (src/query/)                      │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│           │                        │                                    │
│           ▼                        ▼                                   │
│  ┌──────────────────┐   ┌──────────────────────────────────────────┐   │
│  │  src/index/      │   │  src/tensor/  ◄── this module            │   │
│  │  (HNSW, FAISS,   │   │                                          │   │
│  │   ScaNN, DiskANN)│   │  TensorIndexManager                      │   │
│  │                  │   │   ├── FlatTensorIndex (Phase 1)           │   │
│  │  HNSW nav layer  │◄──┼── HnswTTBridge (Phase 2 HYBRID)          │   │
│  │  (sketch-based)  │   │   └── [future: GPU-TT index Phase 4]      │   │
│  └──────────────────┘   └──────────────────────────────────────────┘   │
│                                    │                                    │
│                                    ▼                                    │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │  src/storage/                                                    │   │
│  │   ├── TensorTrainDecomposer  — TT-SVD (Oseledets 2011)          │   │
│  │   ├── TensorNetworkStorageEngine  — RocksDB persistence          │   │
│  │   ├── TTQuantizer  — INT8/NF4 quantization of TT cores           │   │
│  │   ├── TensorRouter — routing decision (HNSW/TT/HYBRID)           │   │
│  │   └── GgmlTensorBridge — zero-copy mmap to llama.cpp (Phase 3)  │   │
│  └──────────────────────────────────────────────────────────────────┘   │
│                                    │                                    │
│                                    ▼                                    │
│  ┌──────────────────────────────────────────────────────────────────┐   │
│  │  src/graph/                                                      │   │
│  │   ├── TensorFingerprintGraph — LSH+MinHash similarity graph      │   │
│  │   └── TensorDeduplicationManager — single-instance TT storage   │   │
│  └──────────────────────────────────────────────────────────────────┘   │
└────────────────────────────────────────────────────────────────────────┘
```

## SOC Boundary with src/index

The `src/tensor/` module maintains a clean separation from `src/index/`:

| Concern | src/index | src/tensor |
|---------|-----------|------------|
| Graph structure | HNSW adjacency graph | — |
| Distance computation | Exact float32 L2/cosine | TT-domain O(d·r²) |
| Storage format | Flat float32 arrays | TT core chains |
| Persistence | Flat file / RocksDB | TT-core RocksDB schema |
| GPU acceleration | CUDA/HIP/Vulkan vector ops | cuSOLVER TT-SVD (Phase 4) |
| GGML integration | — | Zero-copy mmap bridge (Phase 3) |

**Interface between modules (HYBRID mode):**
`HnswTTBridge` calls `src/index` HNSW for graph navigation over sketches, then
`src/tensor` TT arithmetic for re-ranking.  The dependency direction is:
`src/tensor → src/index` (tensor depends on index, not vice versa).

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
```

## Zero-Copy GGML Bridge (Phase 3)

```
ThemisDB                           llama.cpp
─────────                          ──────────
TensorIndexManager
  ggmlCorePtrs(tenant, coll,
               field, id)
        │
        │ raw float* pointers
        │ (TT-core data, already
        │  resident in RAM/VRAM)
        ▼
GgmlTensorBridge::inject(ptrs)
        │
        │ ggml_tensor* (GGML_TYPE_TT)
        │ pointing to ThemisDB memory
        │ NO COPY — shared address space
        ▼
ggml_build_forward() injects
TT-core as graph input node
        │
        ▼
Forward pass: contraction with
model weights uses TT-cores
directly from DB address space
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
