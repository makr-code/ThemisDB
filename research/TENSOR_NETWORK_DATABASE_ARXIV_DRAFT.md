# Tensor-Native Database Patterns in ThemisDB: A Code-Verified Research Draft

**Status:** Internal research draft (review-ready)  
**Repository:** https://github.com/makr-code/ThemisDB

---

## Abstract

This draft reviews tensor-network-oriented capabilities in ThemisDB against the current repository state, with emphasis on what is already implemented versus what remains experimental. The core verified surfaces are: (i) Tensor-Train (TT) decomposition and TT-domain arithmetic, (ii) tensor index lifecycle management, (iii) AQL tensor function registration and execution, (iv) tensor routing decisions for multi-model workloads, and (v) benchmarked hot-path guardrails for tensor runtime behavior. Instead of claiming end-to-end production outcomes for all planned research directions, this version separates verified implementation artifacts from roadmap items and open risks. The resulting argument chain is: problem context -> implemented approach -> evidence from code/tests/benchmarks -> known limitations -> next steps.

---

## 1. Introduction (Einleitung)

Modern multi-model database systems must process vectors, documents, tabular records, graph structures, and model-adjacent numerical artifacts under one operational envelope. Flat-vector-only retrieval paths are often insufficient when data naturally exhibits high-dimensional structure and strong cross-mode correlations. Tensor methods are therefore relevant not only for compression but also for keeping selected computations in compressed form.

In ThemisDB, tensor support is implemented as a module family spanning storage, query, graph, and runtime integration components. However, maturity differs across components. This draft addresses that reality explicitly: it documents verified tensor capabilities, avoids unsupported claims, and highlights where planned research (e.g., deeper HT/QTT expansion or advanced retrieval loops) still needs empirical closure.

---

## 2. System Context in ThemisDB

### 2.1 Architectural placement

Tensor-related functionality is distributed across repository modules rather than a single isolated package:

- `src/storage/` and `include/storage/`: decomposition, storage engine, routing.
- `src/tensor/` and `include/tensor/`: index/manager/bridge/graph and tensor runtime abstractions.
- `src/query/` and `include/query/`: compressed-domain operations and AQL tensor functions.
- `src/rag/` and `include/rag/`: retrieval-gating primitives (FLARE/TARG style interfaces).

This matches ThemisDB's multi-model positioning: tensor flows are integrated into storage/query lifecycles, not bolted on as an external sidecar.

### 2.2 Terminology normalization used in this draft

- **AQL tensor functions**: built-ins registered through `registerTensorFunctions(...)`.
- **Multi-model routing**:
  - static route enum: `TENSOR_TRAIN`, `HYBRID`, `HNSW`.
  - policy decision enum: `LIFT`, `HYBRID`, `KEEP`.
- **Consistency/contract model** (tensor runtime): explicit rules for shape invariants, dtype compatibility, ownership, and CPU/GPU tolerance bounds from `tensor_api_contract.h`.
- **Bridge terminology**: `mapCores()` is the preferred API for mmap-backed tensor-core access; legacy raw-pointer accessors are deprecated.

---

## 3. Methodology / Ansatz

### 3.1 Verification method

This review uses three evidence classes:

1. **Code-level verification** of public headers and implementation files.
2. **Behavioral verification surfaces** from focused tests and roadmap evidence.
3. **Performance envelope artifacts** from benchmark gate definitions.

Claims in this draft are limited to artifacts visible in the current repository snapshot.

### 3.2 Core technical approach (verified)

#### A) TT decomposition and compressed-domain math

- `TensorTrainDecomposer` defines TT decomposition/rounding interfaces and reports decomposition statistics.
- `TTTrain` and `TTCore` provide explicit serialized tensor-train structures for persistence and reuse.
- `TensorContractionEngine` is used by AQL tensor functions for operations such as cosine similarity and norms in TT domain.

#### B) Tensor index management and routing

- `ITensorIndex` defines add/search/norm/inner-product/persistence/statistics contract.
- `TensorIndexManager` owns index lifecycle, route selection, and bridge integration points.
- `TensorRouter` encodes heuristic decisions for TT vs HYBRID vs HNSW based on compressibility indicators and profile metadata.

#### C) AQL integration

- `registerTensorFunctions(...)` registers `TENSOR_*` built-ins.
- `tensor_functions.cpp` resolves tensor arguments from inline JSON objects or document field paths and executes tensor operations via decomposer/contraction engine.

#### D) Retrieval gating interfaces

- `FlareRetrieval` and `TARGRetrieval` provide retrieval-gating logic around uncertainty signals/logit gaps.
- Current implementation exposes integration hooks and runtime logic, while empirical quality claims (hallucination reduction, recall uplift, latency distributions under production load) still require controlled benchmark publication.

### 3.3 Methodological constraint

This draft intentionally separates:

- **implemented interfaces and guards** (code-verifiable), from
- **forward-looking research outcomes** (roadmap/pre-print dependent).

---

## 4. Evaluation / Experiments

### 4.1 Implementation evidence matrix

| Surface | Verification status | Evidence |
|---|---|---|
| Tensor contract semantics | Implemented | `include/tensor/tensor_api_contract.h` |
| TT decomposition APIs | Implemented | `include/storage/tensor_train_decomposer.h`, `src/storage/tensor_train_decomposer.cpp` |
| Tensor index contract | Implemented | `include/tensor/tensor_index.h`, `src/tensor/tensor_index.cpp` |
| Tensor index manager | Implemented | `include/tensor/tensor_index_manager.h`, `src/tensor/tensor_index_manager.cpp` |
| Router heuristics | Implemented | `include/storage/tensor_router.h`, `src/storage/tensor_router.cpp` |
| AQL tensor registration | Implemented | `include/query/functions/tensor_functions.h`, `src/query/functions/tensor_functions.cpp` |
| Compressed-domain contraction engine | Implemented | `include/query/tensor_contraction_engine.h`, `src/query/tensor_contraction_engine.cpp` |
| Retrieval gating primitives | Implemented (module-level) | `include/rag/flare_retrieval.h`, `src/rag/flare_retrieval.cpp`, `include/rag/targ_retrieval.h`, `src/rag/targ_retrieval.cpp` |

### 4.2 Test evidence

Repository-level tensor roadmap evidence reports substantial focused coverage and hardening completion for index/bridge/graph paths (including focused regression and stress suites), with explicit production-readiness checklist items marked complete for recent phases.

Primary evidence:

- `src/tensor/ROADMAP.md` (validated 2026-08-07; phased completion and checklist evidence).
- `tests/tensor/` focused suites such as index manager concurrency, bridge edge cases, contract hardening, and stress coverage.
- `tests/query/test_tensor_contraction_engine.cpp` for query-layer tensor contraction behavior.

### 4.3 Benchmark evidence and measurable gates

This draft distinguishes benchmark **gate definitions** from measured run outputs. Current repository evidence confirms defined release gates for tensor hot paths:

- `benchmarks/tensor/bench_tensor_release_gates.cpp` defines TRNRG-01..TRNRG-06 and associated gate thresholds.
- `src/tensor/PERFORMANCE_EXPECTATIONS.md` defines module-level expectations and regression constraints.

These artifacts are sufficient to claim benchmark governance exists; they are not by themselves sufficient to claim globally valid hardware-independent speedups.

### 4.4 Reproducibility notes

To keep this draft review-safe, all performance claims are constrained to repository-defined thresholds or explicitly marked as future empirical work. No unverified absolute latency/speedup claims are presented as established fact.

---

## 5. Limitations / Known Issues

1. **Bridge zero-copy maturity is partial:** `mapCores()` includes documented stub behavior for true SST-backed `MAP_SHARED` mapping (see notes in `tensor_index_manager.h` and tensor header documentation).
2. **Persistence maturity varies by backend path:** some save/load paths are documented as phased or stubbed in module docs.
3. **Heterogeneous maturity across tensor surfaces:** roadmap and header docs mark several advanced capabilities as experimental or planned despite core APIs being available.
4. **Evaluation gap:** benchmark gate definitions exist, but externally reproducible publication-grade experimental tables (multi-hardware, multi-dataset, confidence intervals) are not fully consolidated in this draft.
5. **RAG integration claims must remain conservative:** retrieval gating components exist, but end-to-end quality improvements require broader controlled experiments before being stated as established outcomes.

---

## 6. Conclusion

ThemisDB already contains a substantial tensor-oriented implementation baseline: TT decomposition contracts, tensor index lifecycle APIs, AQL tensor functions, routing heuristics, and benchmark-gate governance are present and verifiable in code. At the same time, the repository itself documents that parts of the advanced stack remain staged, experimental, or roadmap-bound.

Accordingly, this revised draft reframes the contribution as a **code-verified architecture and capability review** rather than a finished empirical claim set. The next publication step is to pair this verified architectural baseline with fully reproducible experimental evidence across datasets/hardware and to close documented stub/phase gaps in bridge and advanced structural paths.

---

## References

### External scientific references

1. Oseledets, I. V. (2011). *Tensor-Train Decomposition*. SIAM J. Sci. Comput. https://doi.org/10.1137/090752142
2. Holtz, S., Rohwedder, T., & Schneider, R. (2012). *The Alternating Linear Scheme for Tensor Optimization in the Tensor-Train Format*. SIAM J. Sci. Comput. https://doi.org/10.1137/100818893
3. Grasedyck, L. (2010). *Hierarchical Singular Value Decomposition of Tensors*. SIAM J. Matrix Anal. Appl. https://doi.org/10.1137/090764189
4. Hackbusch, W., & Kühn, S. (2009). *A New Scheme for the Tensor Representation*. Journal of Fourier Analysis and Applications. https://doi.org/10.1007/s10820-009-9122-0
5. Khoromskij, B. N. (2011). *O(d log n)-Quantics Approximation of n^d Tensors*. Constructive Approximation. https://doi.org/10.1007/s00365-011-9131-1
6. Dettmers, T. et al. (2023). *QLoRA: Efficient Finetuning of Quantized LLMs*. NeurIPS. https://arxiv.org/abs/2305.14314
7. Jiang, Z. et al. (2023). *Active Retrieval Augmented Generation (FLARE)*. EMNLP. https://arxiv.org/abs/2305.06983
8. Li, Y. et al. (2015). *Butterfly Factorization*. SIAM J. Sci. Comput. https://doi.org/10.1137/15M1007173

### Repository artifacts (ThemisDB)

- Tensor roadmap and phase evidence: `/home/runner/work/ThemisDB/ThemisDB/src/tensor/ROADMAP.md`
- Tensor architecture notes: `/home/runner/work/ThemisDB/ThemisDB/src/tensor/ARCHITECTURE.md`
- Tensor future enhancements: `/home/runner/work/ThemisDB/ThemisDB/src/tensor/FUTURE_ENHANCEMENTS.md`
- Public tensor headers overview: `/home/runner/work/ThemisDB/ThemisDB/include/tensor/README.md`
- API contracts and interfaces:
  - `/home/runner/work/ThemisDB/ThemisDB/include/tensor/tensor_api_contract.h`
  - `/home/runner/work/ThemisDB/ThemisDB/include/tensor/tensor_index.h`
  - `/home/runner/work/ThemisDB/ThemisDB/include/tensor/tensor_index_manager.h`
  - `/home/runner/work/ThemisDB/ThemisDB/include/storage/tensor_train_decomposer.h`
  - `/home/runner/work/ThemisDB/ThemisDB/include/query/functions/tensor_functions.h`
- Implementations:
  - `/home/runner/work/ThemisDB/ThemisDB/src/storage/tensor_router.cpp`
  - `/home/runner/work/ThemisDB/ThemisDB/src/query/functions/tensor_functions.cpp`
  - `/home/runner/work/ThemisDB/ThemisDB/src/rag/flare_retrieval.cpp`
  - `/home/runner/work/ThemisDB/ThemisDB/src/rag/targ_retrieval.cpp`
- Benchmarks and expectations:
  - `/home/runner/work/ThemisDB/ThemisDB/benchmarks/tensor/bench_tensor_release_gates.cpp`
  - `/home/runner/work/ThemisDB/ThemisDB/src/tensor/PERFORMANCE_EXPECTATIONS.md`

---

**Draft Version:** 0.2.0  
**Language:** English (terminology normalized to current codebase)  
**Review state:** Ready for technical review
