---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-11-06"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Cache Research Notes

Exact model backend not verifiable from final document text alone.

## Purpose and evidence boundary

This note records the measurable questions and evidence sources for engineering work on `src/cache`. It is for cache maintainers evaluating cache policy, query reuse, embedding reuse, warmup, and distributed invalidation. It reports no performance or retrieval-quality result by itself.

The implementation evidence is in [`adaptive_query_cache.cpp`](adaptive_query_cache.cpp), [`embedding_cache.cpp`](embedding_cache.cpp), [`semantic_cache.cpp`](semantic_cache.cpp), and the public declarations under [`include/cache`](../../include/cache). Test evidence is in [`tests/cache`](../../tests/cache); release performance evidence is defined by [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) and the [cache benchmark runbook](../../benchmarks/cache/CACHE_BENCHMARK_RUNBOOK.md). The binding v1.x invariants are in [`cache_contract.h`](../../include/cache/cache_contract.h).

## External research and standards

These sources motivate evaluation questions; they are not evidence of ThemisDB cache performance, correctness, or release scope.

| Source | Finding relevant to the roadmap | Applicability limit |
|---|---|---|
| Megiddo and Modha, [“ARC: A Self-Tuning, Low Overhead Replacement Cache”](https://www.usenix.org/conference/fast-03/presentation/arc-self-tuning-low-overhead-replacement-cache), FAST 2003. | ARC adapts its recency/frequency balance and is designed to resist scan pollution; compare it with simpler policies under changing workloads. | Published trace results do not predict ThemisDB's hit ratio or runtime cost. |
| Einziger, Friedman, and Manes, [“TinyLFU: A Highly Efficient Cache Admission Policy”](https://doi.org/10.1145/3149371), ACM Transactions on Storage 13(4), 2017. | Approximate recent-frequency estimates can inform whether a new entry should displace an existing one; admission and eviction can be evaluated together. | Do not assume a TinyLFU implementation exists or performs better in this codebase; verify the implementation and compare against project workloads. |
| Cooper et al., [“Benchmarking Cloud Serving Systems with YCSB”](https://doi.org/10.1145/1807128.1807152), SoCC 2010. | A configurable workload suite supports repeatable comparisons across serving workloads instead of a single synthetic throughput case. | Adapt its read/write distributions to cache-specific capacity, warmup, hit/miss, and policy-overhead measurements; YCSB is not a cache gate by itself. |
| Fielding, Nottingham, and Reschke, [RFC 9111: HTTP Caching](https://www.rfc-editor.org/rfc/rfc9111.html), IETF, 2022. | Freshness, validation, and invalidation are distinct reuse controls and should be stated explicitly. | It standardizes HTTP cache behavior, not ThemisDB's internal cache consistency contract. |
| Misra et al., [“Lightweight Inter-transaction Caching with Precise Clocks and Dynamic Self-invalidation”](https://arxiv.org/abs/2003.04150), 2020. | KAIROS illustrates a lease/self-invalidation design with transaction validation, making the hit-rate versus stale-read/validation-cost trade-off explicit. | It is a transaction-cache design study, not a drop-in protocol recommendation for this module. |
| [OWASP Web Cache Security Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Web_Cache_Security_Cheat_Sheet.html). | Cache-key design, authorization context, sensitive-response reuse, and stale security state are security boundaries, not only performance choices. | Apply only the relevant guidance to ThemisDB's application-cache threat model; caller authorization remains outside the cache implementation. |
| Nishtala et al., [“Scaling Memcache at Facebook”](https://www.usenix.org/conference/nsdi13/technical-sessions/presentation/nishtala), NSDI 2013. | Large-scale cache deployments must reason about coordination, failure, stale sets, and request/miss amplification in addition to steady-state hit latency. | This production case study describes a different architecture and does not establish a required ThemisDB consistency model. |
| Zhang et al., [“Cross-Tenant Side-Channel Attacks in PaaS Clouds”](https://doi.org/10.1145/2660267.2660356), ACM CCS 2014. | Shared hardware caches can create cross-tenant side channels in some cloud threat models. | This is distinct from application-level cache key isolation; whether it is in scope requires a deployment threat-model decision. |

### Research-to-roadmap mapping

- Admission/replacement studies motivate Wave 4 policy comparisons; use measured hit ratio, latency, throughput, and memory on the same recorded workloads.
- Workload-benchmark methodology motivates Wave 1 reproducible runs; the current cache suite and manifest remain the local measurement authority after the multi-output validator gap is closed.
- Freshness, invalidation, and security sources motivate Waves 2, 3, 5, and 6; they do not justify serving stale or approximate results without explicit data-owner approval.
- Distributed-cache case studies motivate testing failure and miss-amplification paths, but project owners must choose the consistency and availability trade-offs.

## Questions and assumptions

1. **Tier placement:** Does selecting an L1/L2/L3 tier from entry size, access behavior, and configuration improve the target workload without exceeding its memory and latency budgets? The adaptive cache has bounded L1/L2 capacities and a RocksDB-backed L3; write-through is an explicit option with a different write cost.
2. **Approximate embedding reuse:** What similarity threshold gives acceptable reuse for a particular embedding model and query corpus? `EmbeddingCache` defaults to cosine similarity and threshold `0.95`, but that default is not evidence that different models, dimensions, or applications share a safe threshold.
3. **Prefetch value:** Do observed successive fingerprints predict future accesses often enough to offset extra cache work and memory? `PredictivePrefetcher` records bounded transition counts; prediction value must be measured against the workload rather than presumed.
4. **Distributed invalidation:** How often do asynchronous invalidation deliveries become partial or delayed, and do operators see the resulting degradation promptly? Local completion and successful remote delivery are separate outcomes.

These are evaluation questions, not claims that a workload-specific model has already been trained or that cache hits are always semantically safe.

## Methods and evaluation

- Compare cache-enabled and cache-disabled paths with the same deterministic query/result inputs and tenant configuration.
- For adaptive cache policy, measure hit/miss counts and p50/p95/p99 latency for L1, L2, L3, misses, tenant-aware operations, invalidation, and concurrent access.
- For embedding reuse, report embedding model/version, dimension, metric, threshold, corpus, duplicate/near-duplicate labeling method, hit rate, and false-reuse cases. A cache hit at or above the configured threshold is not a substitute for application-level correctness review.
- For prefetch, use a chronological workload split: learn transition counts on the earlier segment and measure candidate hit ratio, unnecessary prefetches, overhead bytes, and per-tenant behavior on a later segment. Keep A/B group assignment and time-of-day weighting settings fixed and recorded.
- For replication, inject peer failures and verify local operation behavior, failed-peer diagnostics, bounded retry/drop behavior, and tenant-scoped invalidation.

The existing focused suites include [`test_cache_tenant_isolation_hardening.cpp`](../../tests/cache/test_cache_tenant_isolation_hardening.cpp), [`test_cache_coordinator_degradation.cpp`](../../tests/cache/test_cache_coordinator_degradation.cpp), and [`test_cache_efficiency_multilevel.cpp`](../../tests/cache/test_cache_efficiency_multilevel.cpp). The release benchmark cases and allowed regression tolerance are listed in [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md); use the benchmark runbook for reproducible collection and manifest checks.

## Trade-offs and limits

- Larger capacities and longer TTLs can increase hit opportunities but retain more data and may extend stale-result exposure.
- Approximate similarity saves exact-match constraints but can return an unsuitable result when the threshold or embeddings are not calibrated for the workload.
- A vector index reduces search work when available; the embedding implementation can instead scan entries when no index is active, changing the cost profile with cache size.
- Replication and prefetch add work beyond the local lookup path. Queue limits and retry caps bound some work but do not guarantee every peer receives an event.
- Benchmark results are hardware-, compiler-, corpus-, and configuration-specific. No numeric latency or accuracy claim should be copied from these notes without the associated run artifacts.

## Reproducibility record

For any reported conclusion, preserve the code revision, build preset, hardware, cache configuration, dataset/workload description, benchmark JSON, manifest, and generated gate report under the existing benchmark evidence process. Mark unavailable L3 cases as skipped as the runbook requires; do not treat a skipped case as a passing measurement.

## Related documentation

- [Modeling assumptions](./MODELING.md)
- [Embedding cache behavior](./EMBEDDING.md)
- [Performance expectations](./PERFORMANCE_EXPECTATIONS.md)
- [Module guide](./MODULE_GUIDE.md)
