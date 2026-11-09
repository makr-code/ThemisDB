---
Author: "ThemisDB Team"
Created: "2026-11-09"
Last Updated: "2026-11-09"
Status: "review"
AI_Used: true
AI_Models: "GitHub Copilot (exact model backend not recorded)"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Benchmark Function-Path Coverage — Implementation Plan

## Scope and current evidence

Target branch: `develop`.

This plan follows the benchmark standards in
`benchmarks/BENCHMARK_STANDARDS.md` and the benchmark roadmap. It addresses
measurement validity and production-path coverage, not broad source-file
counts. The current inventory maps benchmark-named sources to most `src/`
directories, but source registration is not evidence that the benchmark calls
the production path, validates results, or has a current performance baseline.

Known examples to remediate:

- `benchmarks/mmdb/bench_mmdb.cpp` labels simplified in-memory map operations,
  brute-force vector scans, text-encoded embeddings, and a simulated RAG
  generation phase as multi-model database work.
- `benchmarks/llm/bench_llm_inference_performance.cpp` contains simulated token
  generation and synthetic context pointers; those cases must not be reported
  as model inference.
- Process retrieval benchmarks use local synthetic stores, while the
  Process-specific expectation comparator currently has no configured
  expectation rows. Stubbed process-manager cases must not be reported as OS
  process performance.
- The stored module coverage matrix is tied to a source revision different
  from the current `develop` HEAD and must be regenerated before it is treated
  as current evidence.

## Acceptance criteria

- Every published case maps to a named production function/API path and is
  labeled `micro`, `component`, or `end_to_end`.
- Each data-path case checks operation success and workload-specific result
  correctness; vector search reports recall against exact ground truth.
- Random inputs use fixed seeds; database/artifact paths are unique and under
  the OS temporary directory; setup, warmup, measurement, and teardown are
  distinct.
- I/O and request-path workloads use wall-clock measurement. Request p50/p95/
  p99 are computed from request-level samples, never inferred from Google
  Benchmark repetitions or aggregate means.
- Synthetic data may be used for deterministic inputs, but synthetic
  algorithms, mocks, or simulated inference are excluded from production
  performance claims.
- Every benchmark target has CTest smoke registration and a result expectation
  mapping. Smoke success is reported separately from performance-gate success.
- No throughput/latency target becomes release-blocking until a representative
  baseline, noise policy, and maintainer-approved threshold are recorded.

## Implementation phases

### Phase 1 — Correct the multi-model database workload

Replace or rename `bench_mmdb` cases that use synthetic / non-production-path
workloads; relabel them as synthetic and exclude them from production
performance reporting until they call real ThemisDB query, storage,
vector-index, and graph APIs. Cover:

- A seeded YCSB-inspired A/B/C/F read/update mix over typed records.
- Filtered document queries and a combined document + vector + graph result
  path.
- ANN top-k search with exact-search ground truth and recall@k.
- Graph 1-hop and 2-hop traversals over stored edges.
- A mixed read/write run with concurrent clients and transaction correctness.

Initial corpus profiles: 100K and 1M records; vector dimensions 384, 768, and
1536; top-k 10 and 50; at least two filter-selectivity levels. Keep load/index
build time outside steady-state query latency and report both separately.

Required metrics: successful operations, errors, throughput, request-level
p50/p95/p99, recall@k, load/index-build time, dataset/index size, and
server-scoped CPU/memory/I/O where available.

### Phase 2 — Separate real LLM inference from component microbenchmarks

Keep adapter/cache/dispatch microbenchmarks distinct from model execution.
Model execution must use an available, pinned GGUF or ONNX model and record its
hash, tokenizer/runtime versions, quantization, hardware, and configuration.
Missing model or accelerator artifacts are reported as skipped, not as zero
latency or a successful inference run.

Measure model load/cold start separately from warmed inference. Exercise
prompt lengths 128, 2K, and 8K tokens; output lengths 64, 256, and 512; batch
sizes 1, 4, and 16; and concurrency 1, 8, and 32 where hardware permits.
Measure offline and interactive arrival profiles separately.

Required metrics: TTFT, inter-token latency/TPOT, request p50/p95/p99,
requests/s, input/output tokens/s, errors, output-quality validation, and
CPU/GPU memory. A real RAG path additionally reports retrieval recall/NDCG and
separate embedding, retrieval, reranking, context-build, and generation
durations.

### Phase 3 — Measure Process module runtime paths

Use the module APIs and persistent data path for:

- BPMN import, parse/validation, persistence, and indexing.
- Keyword/vector retrieval and process-context assembly.
- Process links, graph traversal, and state-change persistence.
- The RAG adapter path, with retrieval output correctness checked separately
  from generated-answer quality.

Use 1K, 10K, and 100K process records, with concurrency 1, 8, and 32 where
supported. Add explicit Process expectations only for cases whose measured
path and metrics are verified. If OS process spawn/termination is a desired
measurement, define it as a separate operating-system benchmark using real OS
processes, not as Process module performance.

### Phase 4 — Reproducibility, comparison, and gates

- Refresh the source-to-CMake-to-CTest inventory from the current commit.
- Retain raw JSON, run manifest, source SHA, build/preset/compiler details,
  hardware profile, dataset/config hashes, workload mix, and failed/skipped
  cases.
- Keep PR checks to compile and short correctness/smoke runs. Run scale,
  concurrency, model inference, and GPU cases on suitable scheduled or
  representative hardware.
- Compare external systems only with pinned reference drivers and validated
  workload semantics; label local YCSB-inspired runs as non-conformant until
  then.
- Establish baseline distributions before introducing regression thresholds;
  use the existing suite-specific variance policy rather than one arbitrary
  threshold for all modules.

## Priority and dependencies

1. Phase 1 depends on confirming usable production APIs and available local
   benchmark fixtures for storage, query, vector, and graph.
2. Phase 2 depends on model-artifact availability and a runner that can report
   hardware-specific metrics. CPU-only runs must be labeled separately.
3. Phase 3 depends on verifying Process API behavior and wiring the Process
   expectation comparator to actual output names.
4. Phase 4 follows successful focused builds and live correctness runs for
   each new path.

## Out of scope for the first implementation batch

- Claiming TPC, MLPerf, YCSB, VectorDBBench, or LDBC compliance without the
  corresponding pinned reference driver and protocol.
- Publishing competitor rankings from microbenchmarks or synthetic paths.
- Adding per-module benchmark targets where no meaningful performance claim
  or production workload exists.
- Making unmeasured performance estimates release-blocking.

