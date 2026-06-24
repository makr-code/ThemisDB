# Boltzmann Observability Layer — Performance Expectations

**Status:** ACTIVE_DRAFT v0.1
**Date:** June 2026
**Related architecture draft:** [`docs/architecture/boltzmann_observability_draft.md`](../architecture/boltzmann_observability_draft.md)
**Related research paper:** [`research/boltzmann_flare_rag_monitoring.tex`](../../research/boltzmann_flare_rag_monitoring.tex)

> **Hypothesis labeling convention:**
> All quantitative claims in this document are labeled `[H]` (hypothesis)
> unless they are derived from verified ThemisDB benchmarks (labeled `[M]`).
> Hypotheses must be validated empirically before being used as SLOs in
> production configurations.

---

## Table of Contents

1. [Ingestion Throughput](#1-ingestion-throughput)
2. [Query Latency Targets](#2-query-latency-targets)
3. [Storage Overhead](#3-storage-overhead)
4. [Alert Precision and Recall](#4-alert-precision-and-recall)
5. [Computational Complexity](#5-computational-complexity)
6. [Benchmark Plan](#6-benchmark-plan)
7. [Known Caveats and Confounders](#7-known-caveats-and-confounders)

---

## 1. Ingestion Throughput

### 1.1 Per-Event Computation Cost

Signal computation for one event with $k$ retrieval candidates requires:

| Operation | Operations | Approx. cycles (x86-64) |
|---|---|---|
| Energy computation (Eq. 1) | $k$ FMA | $k \times 1$–$2$ |
| Boltzmann normalization (Eq. 2) | $k$ exp + 1 sum + $k$ div | $k \times 20$–$30$ |
| Entropy (Eq. 3) | $k$ mul + $k$ log + 1 sum | $k \times 20$–$30$ |
| N\_eff, energy gap, ln Z, Φ | $O(k)$ | $k \times 5$ |
| **Total per event** | $O(k)$ | $k \times 50$–$60$ |

At $k = 20$, typical CPU cost: **~1,000–1,200 cycles ≈ 0.4–0.5 µs** on a
3 GHz core.
This is negligible relative to I/O latency on the ingest path.

### 1.2 Throughput Target

`[H]` **The ingest endpoint sustains ≥ 5,000 events/s (single node) without
degrading primary ThemisDB query throughput by more than 2%.**

Basis:
- Signal computation: 0.5 µs/event × 5,000 events/s = 2.5 ms/s of CPU per
  core. Well within spare capacity.
- Write path: a single INSERT per event into a partitioned relational table.
  ThemisDB write throughput for small rows is known to exceed 50,000 inserts/s
  on spinning disk; observability events are ~400 bytes each.
- Async write path (Option A deployment): ingest acknowledgment before
  database write completes; bounded queue depth of 10,000 events.

`[H]` **Peak burst throughput of 20,000 events/s is achievable for 10-second
bursts with a bounded queue of 50,000 events, with graceful drop of oldest
events when queue is full.**

### 1.3 Scalability Considerations

- Horizontal scaling: observability events are partitioned by `date_bucket`;
  multiple ThemisDB nodes can accept writes for different time partitions.
- Vertical scaling: signal computation is CPU-bound and parallelisable;
  8-core node handles 40,000 events/s before I/O becomes the bottleneck.

---

## 2. Query Latency Targets

All latency targets assume:
- 30-day hot event store: ~10⁸ events (at 3,000 events/s average, ~1.3 × 10¹⁰
  events/day — revise downward to 10⁶–10⁷ events/day for typical RAG deployments).
- Appropriate columnar compression and partition pruning enabled.
- Composite indexes on `(timestamp, phi_scalar)`, `(query_hash)`,
  `(entropy_H, phi_scalar)`.

### 2.1 Primary Monitoring Queries

| Query | Description | p50 `[H]` | p95 `[H]` | p99 `[H]` |
|---|---|---|---|---|
| Hourly entropy drift (7d) | Time-bucketed aggregation, 168 buckets | < 50 ms | < 200 ms | < 500 ms |
| High-risk low-entropy lookup | Filter on (H, Φ) + feedback, no join | < 20 ms | < 100 ms | < 300 ms |
| Per-session trace (1 session) | session\_id index lookup | < 10 ms | < 50 ms | < 100 ms |
| Alert investigation join | AlertEvent ⋈ ObservabilityEvent | < 30 ms | < 150 ms | < 400 ms |
| Latency-entropy quartile | NTILE window + aggregation | < 100 ms | < 400 ms | < 1 s |

### 2.2 Analysis Queries (Longer Running)

| Query | Description | p50 `[H]` | p95 `[H]` |
|---|---|---|---|
| Domain-shift cluster join | ObservabilityEvent ⋈ QueryCluster (vector + relational) | < 200 ms | < 800 ms |
| Source attribution (graph) | Event → Document → Source path traversal, 24h window | < 150 ms | < 600 ms |
| 30-day correlation report | Pearson/Spearman of H vs. feedback across full month | < 5 s | < 15 s |

`[H]` **Analysis queries on the 30-day store complete within 15 s p99 when
run as scheduled jobs (not on-demand user-facing).**

### 2.3 Materialized View Refresh Latency

| View | Refresh trigger | Expected refresh time `[H]` |
|---|---|---|
| `RollingBaseline` (1h bucket) | After each ingest batch (every 30 s) | < 100 ms |
| `RollingBaseline` (24h/7d buckets) | Every 5 minutes | < 500 ms |
| `QueryCluster` | Every 6 hours (async) | < 60 s |

---

## 3. Storage Overhead

### 3.1 Per-Event Storage Estimate

| Field | Type | Bytes |
|---|---|---|
| event_id | UUID | 16 |
| timestamp | TIMESTAMP | 8 |
| query_hash | BYTEA(32) | 32 |
| session_id | UUID (nullable) | 17 |
| pipeline_version | VARCHAR(64) | ~20 avg |
| topk_scores (k=20) | FLOAT64[20] | 160 |
| topk_doc_ids (k=20) | UUID[20] | 320 |
| k_retrieved | SMALLINT | 2 |
| energy params (4×) | FLOAT64 | 32 |
| derived signals (5×) | FLOAT64 | 40 |
| optional signals (3) | FLOAT64+INT64 | ~24 |
| date_bucket | DATE | 4 |
| **Total (uncompressed)** | | **~675 bytes** |

With columnar compression (LZ4, typical compression ratio 4–6×):
**~110–170 bytes/event compressed.**

### 3.2 Storage Growth Projections

| Events/day | Uncompressed/day | Compressed/day | 30-day compressed |
|---|---|---|---|
| 100,000 | 67 MB | 11–17 MB | 330–510 MB |
| 1,000,000 | 675 MB | 110–170 MB | 3.3–5.1 GB |
| 10,000,000 | 6.75 GB | 1.1–1.7 GB | 33–51 GB |

`[H]` **For a typical RAG deployment processing 10⁶ queries/day with k=20,
the observability event store adds ≤ 5 GB to ThemisDB storage over 30 days
(compressed), representing ≤ 10% overhead for a 50+ GB primary database.**

`[H]` **Vector index for query embeddings (768d float32) adds ~3 MB per 1,000
unique query hashes. For 10⁶ queries/day with ~30% unique hashes, this is
~300 MB/day for the vector index, requiring periodic pruning or approximate
deduplication.**

### 3.3 Index Storage

| Index | Estimated size `[H]` |
|---|---|
| `(timestamp, phi_scalar)` B-tree | ~5% of event table |
| `(entropy_H, phi_scalar)` composite | ~5% of event table |
| `(session_id)` | ~3% of event table |
| `(query_hash)` hash index | ~3% of event table |
| **Total index overhead** | **~16% of event table** |

---

## 4. Alert Precision and Recall

All alert quality metrics are hypotheses pending empirical validation.
Ground truth is defined as hallucination-flagged responses with
FActScore < 0.5 or user feedback_label = -1.

### 4.1 Threshold Alerts on Φ

`[H]` **A threshold alert on `phi_scalar > p95_phi` (rolling 24h baseline)
achieves:**
- Precision ≥ 0.50 (at least half of alerted events are true positives)
- Recall ≥ 0.40 (at least 40% of true failures are captured)
- F1 ≥ 0.44

**Caveats:**
- Precision/recall are highly sensitive to the base rate of hallucinations,
  which varies from ~5% (well-grounded domain) to ~40% (out-of-distribution).
- The 0.50/0.40 targets are minimum acceptable thresholds, not expected
  central values. For well-calibrated models on in-domain queries, precision
  may be substantially higher.
- Ground-truth labels (FActScore, user feedback) have their own noise;
  FActScore is known to be ~85% accurate, introducing label noise.

### 4.2 Entropy Spike Alerts

`[H]` **An entropy spike alert (`|H_q - median_H| > 3 * MAD_H`) achieves:**
- False-positive rate ≤ 5% on anomaly-free 24h streams.
- True-positive rate ≥ 60% for injected domain-shift events (entropy
  increased by ≥ 1.5 standard deviations).

**Caveat:** the 3×MAD threshold assumes approximate normality of the entropy
distribution over the baseline window. Heavy-tailed distributions (common in
RAG deployments with diverse query mixes) may require a larger multiplier.

### 4.3 Combined Alert Strategy

`[H]` **Combining phi_scalar anomaly AND entropy_spike with AND logic reduces
false positives to ≤ 2% at the cost of recall reduction to ≥ 25%.**
Recommended for production alerting to avoid alert fatigue.

Using OR logic recovers recall at the cost of higher false-positive rate.
The trade-off should be tuned per deployment based on acceptable false-positive
rate and the cost of missed detections.

---

## 5. Computational Complexity

### 5.1 Per-Event Signal Computation

| Operation | Time | Space |
|---|---|---|
| Energy computation | O(k) | O(k) |
| Boltzmann normalization | O(k) | O(k) |
| Entropy H | O(k) | O(1) (streaming sum) |
| N\_eff, ΔE, ln Z, Φ | O(k) | O(1) |
| **Total** | **O(k)** | **O(k)** |

For k=20: ≤ 1,200 CPU cycles.
For k=100: ≤ 6,000 CPU cycles (~2 µs on 3 GHz core).

### 5.2 Rolling Window Baseline Updates

Naive recomputation: O(W) per event (W = window size).
With ring-buffer sliding sum: O(1) amortized per event for mean/sum.
For percentile updates: O(log W) with order statistics structure.

`[H]` **With ring-buffer implementation, baseline updates add ≤ 10 µs
per event for W ≤ 10,000 (approx. 3h at 1,000 events/s).**

### 5.3 Top-k Anomaly Detection

Maintaining the top-k highest-Φ events in a sliding window:
- Naive: O(W) per query.
- Priority queue (min-heap of size k): O(log k) per event insertion.
- For k=100 anomalies in W=10,000 window: O(100 × log 100) ≈ O(700) per update.

### 5.4 Vector Similarity for Query Clustering

ANN retrieval (HNSW):
- Query time: O(log n × ef) expected, where ef is the search beam width.
- For n=10⁶ query embeddings, d=768, ef=128: **~5–20 ms per query** `[M]`
  (consistent with ThemisDB HNSW benchmarks).
- Batch k-means refresh (every 6h): O(n × K × d × iterations) where K is
  cluster count. For n=10⁶, K=100, d=768, 10 iterations: ~1–5 minutes on
  CPU. GPU-accelerated: ~10–30 seconds.

### 5.5 Graph Traversal for Provenance

Event → Document → Source path traversal:
- For a 24h event window with 10⁵ events, each retrieving k=20 documents:
  2×10⁶ Event→Document edges.
- ThemisDB graph traversal throughput: 1.177 M ops/s `[M]`.
- Full traversal of 2×10⁶ edges: ~1.7 s (acceptable for scheduled jobs).
- Filtered traversal (top-100 high-Φ events): ~0.1 ms `[H]`.

---

## 6. Benchmark Plan

### 6.1 Synthetic Workload

**Purpose:** validate signal computation correctness and ingest throughput.

**Setup:**
- Generate N = {10⁴, 10⁵, 10⁶} synthetic ObservabilityEvent records.
- Score vectors drawn from controlled distributions:
  - `peaked`: one score = 0.95, rest = 0.05 → expected H ≈ 0.25
  - `flat`: all scores uniform → expected H = ln(k)
  - `bimodal`: two groups of 5 high scores, rest near zero → intermediate H
  - `random_gaussian`: Gaussian-distributed scores → variable H

**Measurements:**
1. Ingest throughput (events/s) at N=10⁴, 10⁵.
2. Signal computation accuracy: max absolute error vs. Python reference.
3. Write latency p50/p95/p99.
4. CPU and memory overhead during ingest burst.

**Pass criteria:**
- Throughput ≥ 5,000 events/s on standard ThemisDB hardware.
- Signal error ≤ 1e-6 absolute for H, N\_eff, Φ.
- Write latency p99 ≤ 10 ms (synchronous path) or ≤ 100 ms (async).

### 6.2 Replay Workload

**Purpose:** validate alert detection on realistic query patterns.

**Setup:**
- Replay a 30-day event log from a staging RAG deployment (anonymised, no raw queries).
- Inject synthetic anomalies at known timestamps:
  - Type A: entropy spike (5-minute window of H elevated by +2σ).
  - Type B: sustained high Φ (4-hour window of Φ > p90 baseline).
  - Type C: domain-shift cluster (new query cluster not in 30-day history).

**Measurements:**
1. Detection latency from anomaly injection to AlertEvent creation.
2. True-positive rate per anomaly type.
3. False-positive rate on anomaly-free windows (24h each).
4. Alert volume and suppression rate (combined AND vs. OR logic).

**Pass criteria:**
- Detection latency ≤ 60 s for entropy spike, ≤ 10 min for sustained risk.
- True-positive rate ≥ 0.60 for each anomaly type.
- False-positive rate ≤ 5% on anomaly-free periods.

### 6.3 End-to-End RAG Evaluation

**Purpose:** validate correlation between observability signals and ground-truth
quality metrics.

**Datasets:**
- Natural Questions (NQ) open-domain QA with DPR retrieval.
- TriviaQA with FAISS index retrieval.
- ThemisDB internal test set (if available with quality labels).

**Protocol:**
1. Run full RAG pipeline (retrieve + generate) on each dataset.
2. Collect ObservabilityEvent per query with topk_scores.
3. Compute FActScore and RAGAS context-precision as ground truth.
4. Correlate: H, N\_eff, ΔE, Φ vs. FActScore (continuous) and feedback_label (binary).

**Measurements:**
- Pearson r and Spearman ρ with 95% bootstrap confidence intervals (N=1,000).
- Partial correlations controlling for query length and k.
- ROC AUC for binary hallucination classification using Φ as score.
- Optimal threshold τ for Φ at desired precision/recall operating point.

**Reporting:**
- Results stratified by query domain and difficulty.
- Explicit statement of confounders observed.
- No overfitting: hyperparameters (T, α, β, γ) fixed before seeing labels.

### 6.4 Storage and Query Performance Benchmark

**Purpose:** validate latency targets from §2 under realistic load.

**Setup:**
- Populate ThemisDB with 10⁷ synthetic events (5-min fill, then benchmark).
- Run each AQL query from §7 of architecture draft 100 times.
- Measure p50/p95/p99 wall-clock latency.

**Pass criteria:**
- All queries meet targets from Table 2.1.
- Storage size within 20% of estimate from §3.

---

## 7. Known Caveats and Confounders

### 7.1 Score Distribution Assumptions

All derived signals assume that `topk_scores` are semantically meaningful
similarity values (e.g., cosine similarity in [−1, 1] or dot-product scores
from a calibrated model).

If scores are:
- **Rank-only** (no magnitude information): entropy is meaningless; use
  rank-weighted signals instead.
- **From different embedding models across time**: baseline drift may
  reflect model change, not query distribution change.
- **Truncated at a confidence threshold**: events with fewer than $k$
  candidates have different entropy semantics.

### 7.2 Temperature Sensitivity

All entropy-derived signals depend on the temperature parameter $T$.
Without proper calibration:
- $T$ too high → all $H \approx \ln k$; no discriminative power.
- $T$ too low → all $H \approx 0$; over-sensitive to score noise.

A calibration procedure is required before production use.
Recommended approach: fit $T$ to maximize correlation of $\Phi$ with
validation-set quality labels on a held-out calibration window.

### 7.3 Correlation vs. Causation

As stated throughout this document and in the research paper:
**all correlations between observability signals and quality metrics are
observational.** No causal claims are made. Confounders include:
- Query difficulty (hard questions have both low retrieval quality and
  low generation quality, regardless of entropy).
- Topic rarity (rare topics have out-of-distribution embeddings and
  also higher hallucination rates, creating spurious correlation with entropy).
- Model version changes affecting both scores and quality simultaneously.

### 7.4 Label Quality

FActScore is ~85% accurate. User feedback (thumbs up/down) is noisy,
biased by presentation order, and often missing for the majority of queries.
Precision/recall estimates using these labels underestimate true values
by an unknown amount.

### 7.5 Index Coverage Changes

Index updates (document ingestion, deletion, re-embedding) change the
score distribution independently of query distribution changes.
Without tracking index updates as events, entropy drift may be attributed
to query shift when it actually reflects index change.

**Recommendation:** emit `IndexUpdateEvent` records alongside
`ObservabilityEvent` and filter or segment analysis by index-stable periods.

---

*This document is an ACTIVE_DRAFT. All `[H]` values are hypotheses requiring
empirical validation. See the benchmark plan (§6) for the validation protocol.
Verified measured values will be updated from `[H]` to `[M]` with benchmark
citations upon completion of evaluation.*
