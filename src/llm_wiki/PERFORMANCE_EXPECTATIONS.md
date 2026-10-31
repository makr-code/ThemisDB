---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# LLM Wiki Performance Expectations

## Scope

This page distinguishes project targets from measured module performance. The source currently available does not establish a production query/ingest SLA. Root [`ROADMAP.md`](../../ROADMAP.md) is canonical for project-level targets and release posture.

## Current benchmark coverage

| Source | What it measures | Limitation |
|---|---|---|
| `benchmarks/llm_wiki/bench_llm_wiki_dedicated_gates.cpp` | In-process stub index query/sync and cache lookup timings | Uses `WikiIndexStub`/`CacheStub`, not the production plugin, retrieval index, or cache |
| `benchmarks/llm_wiki/bench_wikipedia_throughput.cpp` | Synthetic article string processing and checkpoint-string construction | Does not invoke the Wikipedia parser, plugin ingestion, embedding, or persistence pipeline |

These programs can compare their own synthetic operations across revisions, but their results do not demonstrate product throughput or query latency. The current CTest performance tests are tests, not representative hardware baselines.

## Project target and status

The root roadmap specifies a Phase B target of at least 2× query throughput over Phase A at 50,000 chunks and p95 below 100 ms on representative hardware (`../../ROADMAP.md`, RAG/WikiIndexStore Phase B). This is a target, not a validated result for the current plugin path. Representative hardware evidence is still required; no production baseline is recorded here.

| Metric | Target | Current evidence | Status |
|---|---|---|---|
| Phase B query p95 at 50K chunks | < 100 ms | No production-path benchmark evidence in the module benchmark sources | Pending |
| Phase B query throughput vs Phase A | ≥ 2× | No comparable production-path Phase A/B measurements recorded here | Pending |
| Wikipedia ingestion throughput | No validated module SLA | Synthetic corpus benchmark only | Not established |
| Workspace memory/resource ceiling | No source-backed operational limit | No enforced module-wide cap established | Not established |

## Measurement requirements

Before marking a target complete, record the code revision, build configuration, hardware, corpus size/revision, query set, number of samples, p50/p95/p99, throughput, memory use, and error/corruption outcomes. Compare the same corpus and workload across the relevant backends. Do not claim a target from a microbenchmark stub or from test timing alone.

## Failure and operational considerations

- Benchmark results from synthetic inputs can hide parsing, vectorization, storage, and I/O costs.
- A benchmark must fail or mark itself invalid if the intended backend is unavailable; do not silently substitute a stub and report it as production performance.
- Enforce deployment resource limits at the integrating service until the module exposes and validates explicit limits.

> Exact model backend not verifiable from final document text alone.
