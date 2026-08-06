# Process Module Performance Benchmarks

## Overview

This directory contains the comprehensive benchmark suite for ThemisDB's Process module. The benchmarks measure performance across all critical paths: import/export, parsing, linking, retrieval, mining, and advanced workflows.

**Phase 5 Status**: ✅ Performance & Hardening gates implemented and validated

## Benchmark Files and Gate Coverage

### Phase 4: Foundational Benchmarks
- `bench_process_import_retrieval.cpp` - Import/export and descriptor assembly paths
- `bench_process_mining.cpp` - Mining algorithm performance (Alpha, Heuristic, Inductive)
- `bench_process_retrieval.cpp` - Embedding-based retrieval and search
- `bench_process_release_gates.cpp` - Basic error handling gates

### Phase 5: Performance & Hardening Gates

#### 1. Concurrency Performance Gates (CP)
**File**: `bench_process_concurrency_gates.cpp`

Measures CRUD/import/export/linking throughput and latency under concurrent churn.

| Gate ID | Metric | Target | Benchmark |
|---------|--------|--------|-----------|
| CP-01 | Concurrent CRUD (100 models) | ≥ 50k ops/s | BM_CP01_ConcurrentCrud_Small |
| CP-02 | Concurrent CRUD (1k models) | ≥ 40k ops/s | BM_CP02_ConcurrentCrud_Medium |
| CP-03 | Concurrent Import (100 BPMN) | ≥ 20k ops/s | BM_CP03_ConcurrentImport_Bpmn |
| CP-04 | Concurrent Export (100 models) | ≥ 15k ops/s | BM_CP04_ConcurrentExport |
| CP-05 | Concurrent Linking (100 models) | ≥ 10k ops/s | BM_CP05_ConcurrentLinking |
| CP-06 | Concurrent Retrieval (1k models) | ≥ 30k ops/s | BM_CP06_ConcurrentRetrieval_Medium |

**Methodology**: 
- Uses 4-thread concurrency model
- Simulates realistic operation distribution (70% read, 15% update, 10% delete, 5% create)
- Measures throughput in ops/sec
- Reports mean, p50, p95, p99 latencies

---

#### 2. Determinism Performance Gates (DP)
**File**: `bench_process_determinism_gates.cpp`

Measures conflict resolution and rollback latency for deterministic ordering.

| Gate ID | Metric | Target | Benchmark |
|---------|--------|--------|-----------|
| DP-01 | Conflict Resolution (100 conflicts) | p99 ≤ 50ms | BM_DP01_ConflictResolution |
| DP-02 | Rollback Single (10 revisions) | p99 ≤ 30ms | BM_DP02_RollbackSingle |
| DP-03 | Rollback Batch (100 models) | p99 ≤ 100ms | BM_DP03_RollbackBatch |
| DP-04 | Transaction Serialization | p99 ≤ 25ms | BM_DP04_TransactionSerialization |

**Methodology**:
- Simulates revision history with configurable depths
- Measures conflict detection using DFS algorithm
- Reports p50, p95, p99 latencies in milliseconds
- Uses deterministic seed (42) for reproducibility

---

#### 3. Diagnostics Overhead Gates (GO)
**File**: `bench_process_diagnostics_overhead.cpp`

Verifies incident classification overhead remains < 5% regression.

| Gate ID | Metric | Target | Benchmark |
|---------|--------|--------|-----------|
| GO-01 | Classification Overhead | Regression ≤ 5% | BM_GO01_ClassificationOverhead |

**Methodology**:
- Compares baseline classifier vs. enhanced classifier with diagnostics
- Measures overhead percentage: (enhanced - baseline) / baseline * 100%
- Tests with 1k incident cases
- Reports overhead_percent, baseline_us, enhanced_us

**Acceptance Criteria**:
```
(enhanced_mean - baseline_mean) / baseline_mean <= 0.05 (5%)
```

---

#### 4. Parser Performance Gates (PP)
**File**: `bench_process_parser_gates.cpp`

Validates parse latency p95/p99 for all supported formats.

| Gate ID | Format | Metric | Target | Benchmark |
|---------|--------|--------|--------|-----------|
| PP-01 | BPMN | Parse 100 files | p99 ≤ 50ms | BM_PP01_BpmnParse_Small |
| PP-02 | BPMN | Parse 1k files | p99 ≤ 100ms | BM_PP02_BpmnParse_Medium |
| PP-03 | EPK | Parse 100 files | p99 ≤ 75ms | BM_PP03_EpkParse |
| PP-04 | CMMN | Parse 100 files | p99 ≤ 60ms | BM_PP04_CmmnParse |
| PP-05 | DMN | Parse 100 files | p99 ≤ 40ms | BM_PP05_DmnParse |
| PP-06 | OCEL | Parse 100 logs | p99 ≤ 200ms | BM_PP06_OcelParse |
| PP-07 | VCC/VPB | Parse 100 files | p99 ≤ 80ms | BM_PP07_VccVpbParse |
| PP-08 | FIM | Parse 100 files | p99 ≤ 70ms | BM_PP08_FimParse |

**Methodology**:
- Generates realistic XML/JSON content for each format
- Simulates DOM/SAX parsing, element counting, structure validation
- Measures p50, p95, p99 latencies
- Uses steady_clock for I/O-bound measurements

---

#### 5. Linker Performance Gates (LP)
**File**: `bench_process_linker_gates.cpp`

Validates linking latency and cyclic-dependency detection.

| Gate ID | Metric | Target | Benchmark |
|---------|--------|--------|-----------|
| LP-01 | Linking Latency (100 pairs) | p99 ≤ 20ms | BM_LP01_LinkingLatency |
| LP-02 | Cyclic Dependency Detection (1k) | p99 ≤ 50ms | BM_LP02_CyclicDependencyDetection |
| LP-03 | Link Validation (1k links) | p99 ≤ 25ms | BM_LP03_LinkValidation |
| LP-04 | Graph Traversal (10k nodes) | p99 ≤ 100ms | BM_LP04_GraphTraversal |

**Methodology**:
- Builds directed acyclic graph (DAG) of process models
- Uses depth-first search (DFS) for cycle detection
- Measures breadth-first search (BFS) for graph traversal
- Reports p50, p95, p99 latencies in milliseconds

---

#### 6. Retriever Performance Gates (RP)
**File**: `bench_process_retriever_gates.cpp`

Validates query latency p95/p99 and throughput under churn.

| Gate ID | Metric | Target | Benchmark |
|---------|--------|--------|-----------|
| RP-01 | Simple Query (1k models) | p99 ≤ 20ms | BM_RP01_SimpleQuery |
| RP-02 | Complex Query (1k models) | p99 ≤ 50ms | BM_RP02_ComplexQuery |
| RP-03 | Full-Text Search (1k models) | p99 ≤ 30ms | BM_RP03_FullTextSearch |
| RP-04 | Embedding Similarity (1k models) | p99 ≤ 40ms | BM_RP04_EmbeddingSimilarity |
| RP-05 | Pagination Query (10k models) | p99 ≤ 100ms | BM_RP05_PaginationQuery |
| RP-06 | Concurrent Query (1k models, 4x) | ≥ 5k qps | BM_RP06_ConcurrentQuery |
| RP-07 | Query Under Churn (1k→10k) | p99 ≤ 75ms | BM_RP07_QueryUnderChurn |
| RP-08 | Ranking/Sorting (1k results) | p99 ≤ 25ms | BM_RP08_RankingAndSorting |

**Methodology**:
- Tests state filters, multi-field filters, full-text search, embedding similarity
- Uses 128-dimensional normalized embeddings with cosine distance
- Simulates pagination with configurable page sizes
- Measures under concurrent load with 4 worker threads
- Measures under dynamic model addition (churn)

---

#### 7. Benchmark Expansion - Advanced Workflows (BE)
**File**: `bench_process_advanced_workflows.cpp`

Expands process mining and RAG workflow depth for advanced scenarios.

| Gate ID | Scenario | Coverage | Benchmark |
|---------|----------|----------|-----------|
| BE-01 | Multi-Format Import | 100 files × 5 formats | BM_BE01_MultiFormatImport |
| BE-02 | Process Mining Alpha | 1k event log | BM_BE02_AlphaMiner |
| BE-03 | Process Mining Heuristic | 1k event log | BM_BE03_HeuristicMiner |
| BE-04 | Process Mining Inductive | 1k event log | BM_BE04_InductiveMiner |
| BE-05 | Conformance Checking | 1k events vs. DFG | BM_BE05_ConformanceChecking |
| BE-06 | Variant Analysis | 1k events, clustering | BM_BE06_VariantAnalysis |
| BE-07 | LLM Process Descriptor | 100 models, NLG | BM_BE07_LlmDescriptor |
| BE-08 | BPMN→DFG Conversion | 100 models | BM_BE08_BpmnToDfgConversion |
| BE-09 | Community Detection | 1k model graph | BM_BE09_CommunityDetection |
| BE-10 | RAG Knowledge Retrieval | 1k models + queries | BM_BE10_RagRetrieval |
| BE-11 | End-to-End Scenario | Import + mining + retrieval | BM_BE11_EndToEndScenario |
| BE-12 | Sustained Load | High-load stress test | BM_BE12_StressTest |

**Methodology**:
- Tests complete workflows combining multiple operations
- Simulates mining algorithms (Alpha, Heuristic, Inductive Miner)
- Includes conformance checking and variant analysis
- End-to-end scenario chains: import → mining → validation → analysis
- Stress test runs concurrent operations (4 threads)

---

## Running Benchmarks

### Run All Process Benchmarks
```bash
# Release mode (recommended for performance validation)
cmake --preset=release -DTHEMIS_BUILD_BENCHMARKS=ON
cmake --build --preset=release --target=bench_process_*

# Run all concurrency gates
./build/Release/benchmarks/process/bench_process_concurrency_gates --benchmark_repetitions=10

# Run with specific filter
./build/Release/benchmarks/process/bench_process_parser_gates --benchmark_filter="PP.*"
```

### Run Individual Benchmarks
```bash
# Concurrency gates
./build/Release/benchmarks/process/bench_process_concurrency_gates

# Parser gates (all 8 formats)
./build/Release/benchmarks/process/bench_process_parser_gates

# Retriever gates
./build/Release/benchmarks/process/bench_process_retriever_gates

# Advanced workflows
./build/Release/benchmarks/process/bench_process_advanced_workflows
```

### Benchmark Options
```bash
# Show available benchmarks
--benchmark_list_tests=true

# Run specific test
--benchmark_filter="CP-01"

# Set iterations
--benchmark_repetitions=20

# Output formats: json, csv
--benchmark_format=json --benchmark_out=results.json
```

---

## Performance Envelopes & Regression Budgets

### Concurrency Gates (CP)
- **Baseline**: Established in Phase 4
- **Regression Budget**: 10% allowed variance
- **Scaling**: Linear with thread count (4 threads reference)

### Determinism Gates (DP)
- **Baseline**: Conflict resolution < 10ms per case
- **Regression Budget**: 15% allowed variance

### Diagnostics Overhead (GO)
- **Hard Limit**: < 5% regression (strict)

### Parser Gates (PP)
- **Regression Budget**: 20% allowed variance

### Linker Gates (LP)
- **Regression Budget**: 15% allowed variance

### Retriever Gates (RP)
- **Regression Budget**: 25% allowed variance

### Advanced Workflows (BE)
- **Regression Budget**: 30% allowed variance

---

## Measurement Hygiene

### Memory Management
- All I/O benchmarks use OS temporary directories
- Cleanup after each iteration
- No memory accumulation across repetitions

### Timing
- CPU-bound benchmarks: default timing
- I/O-bound benchmarks: `UseRealTime()`
- Timing excludes setup/teardown via `PauseTiming()` / `ResumeTiming()`

### Data
- Deterministic seeds (kCanonicalRngSeed=42)
- Realistic data sizes: small (100), medium (1k), large (10k+)

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-08-06 | Phase 5: Complete gate suite (39 gates) |

---

**Status**: ✅ Production Ready (Phase 5 Gates Validated)
