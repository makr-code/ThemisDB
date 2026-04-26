# Product Quantization in ThemisDB: Compression-Accuracy-Throughput Trade-offs

**Status**: Draft  
**Version**: 0.2 (migrated from docs/research)  
**Last Updated**: 2026-04-19  
**Target Venue**: arXiv (cs.DB / cs.IR)

---

## Abstract

This paper analyzes Product Quantization (PQ) in ThemisDB as a production mechanism to reduce vector memory footprint while preserving retrieval quality. It covers standard PQ, residual quantization, and binary quantization, and identifies next-step opportunities such as OPQ and SIMD/GPU acceleration for ADC. The contribution is a repository-grounded evaluation framework that makes compression-quality-speed trade-offs explicit for database operators.

## I. Introduction

Vector search at scale is constrained by memory bandwidth and index size. PQ-based methods offer large compression gains but introduce approximation error and implementation complexity.

### Contributions

1. A structured comparison of currently implemented quantization methods in ThemisDB.
2. Evidence-backed migration path toward higher-recall variants (e.g., OPQ).
3. Reproducibility-focused metric set for production decision-making.

## II. Related Work

- Product Quantization and asymmetric distance computation.
- Residual and additive quantization variants.
- Practical ANN pipelines combining quantization and graph/index structures.

## III. System Model / Architecture

- Encoding path: training codebooks -> vector encoding -> index persistence.
- Query path: query-side distance tables -> ADC over compressed codes.
- Variant support: standard PQ, residual PQ, binary quantization.

## IV. Method / Design

- Compare variants on recall@k, latency, compression ratio, build time.
- Separate offline training cost from online query cost.
- Identify engineering opportunities (SIMD ADC, GPU-assisted PQ).

## V. Implementation Evidence (Repository-Grounded)

| Evidence ID | File | Scope | What It Proves | Status |
|-------------|------|-------|----------------|--------|
| E1 | `include/index/product_quantizer.h` | API | Standard PQ interfaces are defined | Ready |
| E2 | `src/index/product_quantizer.cpp` | implementation | Codebook training + ADC path exist | Ready |
| E3 | `include/index/residual_quantizer.h` | API | Residual quantization interface exists | Ready |
| E4 | `src/index/residual_quantizer.cpp` | implementation | Multi-stage residual quantization path exists | Ready |
| E5 | `include/index/binary_quantizer.h` | API | Binary quantization interface exists | Ready |
| E6 | `src/index/binary_quantizer.cpp` | implementation | Binary encoding/search path exists | Ready |
| E7 | `tests/test_product_quantizer.cpp` | tests | Standard PQ correctness coverage exists | Ready |
| E8 | `tests/test_residual_quantizer.cpp` | tests | Residual quantization test coverage exists | Ready |
| E9 | `benchmarks/bench_product_quantization.cpp` | benchmarks | PQ performance measurement harness exists | Ready |

## VI. Experimental Methodology

### A. Setup
- Dimension sets: 384/768/1536
- Candidate datasets: synthetic + corpus-derived embeddings
- Fixed hardware profile per run and repeated measurements

### B. Workloads
- W1: standard PQ (baseline compression)
- W2: residual PQ (accuracy-oriented)
- W3: binary quantization (maximum compression)

### C. Metrics
- Recall@10 / Recall@k
- p50/p95/p99 query latency
- Memory per vector and global footprint
- Offline training/build time

## VII. Results (Planned Consolidation)

Repository-level implementation and tests are available. Consolidated comparative results across all dataset scales and parameter sweeps are the next publication step.

## VIII. Discussion

PQ variants should be selected by operational objective: memory minimization, quality retention, or throughput. No single variant dominates across all constraints.

### Claim Boundaries

**Supported claims:**
- Standard, residual, and binary quantization are implemented and test-covered.
- Benchmark harness exists for systematic evaluation.

**Deferred claims:**
- Universal recall gains from OPQ without dataset-specific validation.
- Stable speedup claims across all CPU/GPU architectures.

## IX. Reproducibility & Artifact

- Artifact anchors: quantizer implementations, tests, benchmark harness.
- Reproducibility target: publish standard parameter grid and seed policy.
- Open action: add unified report table for all quantizer variants.

## X. Limitations, Risk, Ethics

- Compression can hide quality degradation if recall is under-reported.
- Dataset bias can overstate transferability.
- Operational settings must document acceptable quality floors.

## XI. Conclusion

ThemisDB already provides a strong PQ baseline with residual and binary extensions. The immediate research value is a rigorous, reproducible quantization frontier analysis and guided production tuning.

## References

- Source analysis and bibliography: `docs/research/PRODUCT_QUANTIZATION_RESEARCH.md`.
