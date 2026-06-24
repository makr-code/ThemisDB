# Boltzmann-Inspired Observability Layer — Architecture Draft

**Status:** ACTIVE_DRAFT v0.1
**Date:** June 2026
**Related research paper:** [`research/boltzmann_flare_rag_monitoring.tex`](../../research/boltzmann_flare_rag_monitoring.tex)
**Related performance doc:** [`docs/performance/boltzmann_observability_expectations.md`](../performance/boltzmann_observability_expectations.md)

---

## Table of Contents

1. [Goals and Non-Goals](#1-goals-and-non-goals)
2. [Terminology](#2-terminology)
3. [Data Model — Event Schema](#3-data-model--event-schema)
4. [Derived Signal Definitions](#4-derived-signal-definitions)
5. [Pipeline Stages](#5-pipeline-stages)
6. [ThemisDB Hybrid Model Mapping](#6-themisdb-hybrid-model-mapping)
7. [AQL Query Examples](#7-aql-query-examples)
8. [Deployment Options](#8-deployment-options)
9. [Failure Modes and Mitigations](#9-failure-modes-and-mitigations)
10. [FLARE Parallel and Complementarity](#10-flare-parallel-and-complementarity)
11. [Iterative Roadmap](#11-iterative-roadmap)
12. [Open Questions](#12-open-questions)

---

## 1. Goals and Non-Goals

### Goals

- Provide a **principled, cheap-to-compute observability signal set** for RAG
  retrieval quality derived from Boltzmann/Gibbs statistics.
- Enable **longitudinal drift detection** across query batches without
  requiring ground-truth labels.
- Integrate with **ThemisDB's hybrid storage** (relational, vector, graph,
  file) using native AQL queries.
- Define a **schema-first event model** that any RAG adapter or ThemisDB
  client can emit, with consistent signal semantics.
- Provide **alerting hooks** for elevated retrieval risk at the system level.

### Non-Goals

- **Not a replacement for per-request generation quality monitoring.**
  FLARE and similar within-loop techniques remain the appropriate tool for
  per-query generation-level uncertainty.
- **Not a causal attribution system.**
  Observed correlations between entropy/energy signals and quality metrics
  are observational; no causal claims are made.
- **Not a physics simulation.**
  The term "Boltzmann-inspired" denotes mathematical form, not physical
  equation solving in production.
- **Not a real-time annotation system.**
  Signal computation may be asynchronous; alerting latency of seconds to
  minutes is acceptable for the use cases addressed.
- **Not a replacement for RAGAS or FActScore evaluation.**
  This layer provides cheap proxies, not ground-truth accuracy measurement.

---

## 2. Terminology

| Term | Definition |
|---|---|
| **Observability event** | A single structured record emitted per RAG inference call, containing retrieval scores and derived signals. |
| **Energy** $E_i$ | A scalar assigned to retrieval candidate $i$ combining similarity, recency, and risk factors. Lower = better candidate. |
| **Boltzmann/Gibbs weights** $p_i$ | Probabilities derived via softmax over negated energies (Eq. 1 in the research paper). |
| **Entropy** $H$ | Shannon entropy of the Boltzmann weight distribution. High = diffuse retrieval; low = concentrated. |
| **Effective candidate count** $N_\text{eff}$ | $\exp(H)$; interpretable number of candidates effectively considered. |
| **Energy gap** $\Delta E$ | Difference between 2nd-best and best candidate energy; small gap = ambiguous top choice. |
| **Free-energy proxy** $\Phi$ | $\bar{E} - T \cdot H$; combines expected energy and entropy into a single risk scalar. |
| **Rolling baseline** | Per-time-bucket statistics ($\mu$, $\sigma$, MAD) of signals over past $W$ queries. |
| **FLARE** | Forward-Looking Active REtrieval (Jiang et al. 2023); within-loop re-retrieval trigger. |

---

## 3. Data Model — Event Schema

### 3.1 `ObservabilityEvent` Table

Stored in ThemisDB relational store. All derived signals computed
server-side on ingest to guarantee consistency.

```sql
CREATE TABLE ObservabilityEvent (
    event_id          UUID         NOT NULL PRIMARY KEY,
    timestamp         TIMESTAMP    NOT NULL,          -- UTC wall clock
    query_hash        BYTEA(32)    NOT NULL,           -- SHA-256(query_embedding)
    session_id        UUID,                            -- nullable; user/agent session
    pipeline_version  VARCHAR(64)  NOT NULL DEFAULT 'unknown',

    -- Raw retrieval outputs
    topk_scores       FLOAT64[]    NOT NULL,           -- s_1 .. s_k, descending
    topk_doc_ids      UUID[]       NOT NULL,           -- matched document IDs
    k_retrieved       SMALLINT     NOT NULL,           -- actual k (may be < max k)

    -- Energy model parameters used (for reproducibility)
    energy_alpha      FLOAT64      NOT NULL DEFAULT 1.0,
    energy_beta       FLOAT64      NOT NULL DEFAULT 0.0,
    energy_gamma      FLOAT64      NOT NULL DEFAULT 0.0,
    energy_temperature FLOAT64     NOT NULL DEFAULT 1.0,

    -- Derived observability signals (computed on ingest)
    entropy_H         FLOAT64      NOT NULL,
    n_eff             FLOAT64      NOT NULL,
    energy_gap        FLOAT64      NOT NULL,
    ln_Z              FLOAT64      NOT NULL,
    phi_scalar        FLOAT64      NOT NULL,

    -- Optional downstream signals (nullable until available)
    answer_confidence FLOAT64,                         -- surrogate LLM confidence
    latency_ms        INT64,                           -- end-to-end request latency
    feedback_label    SMALLINT,                        -- -1/0/+1 user feedback

    -- Partition key for retention management
    date_bucket       DATE         NOT NULL            -- GENERATED AS DATE(timestamp)
)
PARTITION BY RANGE (date_bucket);
```

### 3.2 `AlertEvent` Table

```sql
CREATE TABLE AlertEvent (
    alert_id          UUID         NOT NULL PRIMARY KEY,
    created_at        TIMESTAMP    NOT NULL,
    alert_type        VARCHAR(64)  NOT NULL,  -- 'entropy_spike' | 'phi_anomaly' | 'sustained_risk'
    severity          VARCHAR(16)  NOT NULL,  -- 'warning' | 'critical'
    trigger_event_id  UUID         REFERENCES ObservabilityEvent(event_id),
    window_start      TIMESTAMP    NOT NULL,
    window_end        TIMESTAMP    NOT NULL,
    metric_value      FLOAT64      NOT NULL,  -- value that triggered alert
    baseline_value    FLOAT64      NOT NULL,  -- baseline at alert time
    payload           JSON                    -- additional context
);
```

### 3.3 `QueryCluster` Materialized View

Maintained by periodic k-means clustering on query embeddings stored in
the vector index. Used for domain-shift detection queries.

```sql
-- Updated by async background job (see Stage 3.4)
CREATE MATERIALIZED VIEW QueryCluster AS
SELECT
    query_hash,
    cluster_id,
    cluster_centroid_distance,
    snapshot_date
FROM VectorIndex.QueryEmbeddingClusters;
```

### 3.4 `RollingBaseline` Materialized View

```sql
CREATE MATERIALIZED VIEW RollingBaseline AS
SELECT
    DATE_TRUNC(timestamp, 'hour')   AS bucket_1h,
    DATE_TRUNC(timestamp, 'day')    AS bucket_1d,
    COUNT(*)                         AS n,
    AVG(entropy_H)                   AS mean_H,
    STDDEV(entropy_H)                AS std_H,
    PERCENTILE_CONT(0.5)  WITHIN GROUP (ORDER BY entropy_H) AS median_H,
    PERCENTILE_CONT(0.95) WITHIN GROUP (ORDER BY entropy_H) AS p95_H,
    AVG(phi_scalar)                  AS mean_phi,
    PERCENTILE_CONT(0.95) WITHIN GROUP (ORDER BY phi_scalar) AS p95_phi
FROM ObservabilityEvent
GROUP BY bucket_1h, bucket_1d;
```

---

## 4. Derived Signal Definitions

All signals are computed deterministically from `topk_scores` and energy
parameters. Reference implementation (pseudocode):

```python
def compute_signals(scores: list[float], alpha=1.0, beta=0.0, gamma=0.0,
                    T=1.0, recency=None, risk=None) -> dict:
    k = len(scores)
    recency = recency or [0.0] * k
    risk = risk or [0.0] * k

    # Energy per candidate (Eq. 1 in paper)
    energies = [-alpha * s - beta * r + gamma * rho
                for s, r, rho in zip(scores, recency, risk)]

    # Boltzmann weights (Eq. 2)
    beta_T = 1.0 / T
    exp_neg_E = [math.exp(-beta_T * e) for e in energies]
    Z = sum(exp_neg_E)
    p = [x / Z for x in exp_neg_E]

    # Entropy H (Eq. 3)
    H = -sum(pi * math.log(pi) for pi in p if pi > 1e-15)

    # N_eff (Eq. 4)
    N_eff = math.exp(H)

    # Energy gap (Eq. 5)
    sorted_E = sorted(energies)
    delta_E = sorted_E[1] - sorted_E[0] if len(sorted_E) >= 2 else 0.0

    # Free-energy proxy (Eq. 6)
    E_bar = sum(pi * ei for pi, ei in zip(p, energies))
    phi = E_bar - T * H

    return dict(entropy_H=H, n_eff=N_eff, energy_gap=delta_E,
                ln_Z=math.log(Z), phi_scalar=phi)
```

**Important:** the reference implementation is for validation only.
Production computation runs inside the ThemisDB ingest handler in C++.

---

## 5. Pipeline Stages

```
RAG Pipeline                    ThemisDB Observability Layer
─────────────────────           ────────────────────────────────────────────────
  [Query]                           │
     │                              │
  [Retriever]──topk_scores─────────►│  Stage 1: INGEST
     │         topk_doc_ids         │   - Receive ObservabilityEvent (gRPC/HTTP)
  [Generator]──answer_conf─────────►│   - Compute H, N_eff, ΔE, ln Z, Φ
     │         latency_ms           │   - Write to ObservabilityEvent table
  [Response]──feedback_label───────►│   - Append query embedding to vector index
                                    │
                                    │  Stage 2: ROLLING BASELINE UPDATE
                                    │   - Sliding-window aggregation (1h / 24h / 7d)
                                    │   - Update RollingBaseline materialized view
                                    │   - Trigger async cluster refresh if Δt > 6h
                                    │
                                    │  Stage 3: ALERTING
                                    │   - Evaluate threshold rules against baseline
                                    │   - Write AlertEvent on trigger
                                    │   - Publish to alert bus (Prometheus/webhook)
                                    │
                                    │  Stage 4: ANALYSIS (on-demand / scheduled)
                                    │   - AQL correlation queries (see §7)
                                    │   - Drift reports (hourly/daily)
                                    │   - Domain-shift cluster analysis
                                    │
                                    │  Stage 5: RETENTION
                                    │   - Roll daily partitions to cold storage
                                    │   - Downsample old events (keep aggregates)
                                    │   - Purge per data-retention policy
```

### Stage Details

#### Stage 1 — Ingest

- **Interface:** `POST /observability/events` (JSON) or gRPC
  `ObservabilityService.Ingest(ObservabilityEvent)`.
- **Computation:** server-side signal derivation using the C++ implementation
  (avoids client-side parameter inconsistencies).
- **Write path:** single INSERT into partitioned `ObservabilityEvent` table.
- **Latency budget:** ≤ 5 ms synchronous acknowledgement; actual signal
  computation may be async (eventual consistency within 100 ms).

#### Stage 2 — Rolling Baseline Update

- Implemented as an incremental materialized view refresh triggered by
  ingest batch completion.
- Rolling window sizes: 1h, 24h, 7d. Each window maintained independently.
- Baseline statistics: mean, stddev, median, MAD, p95 of $H$ and $\Phi$.

#### Stage 3 — Alerting

Alert conditions (configurable thresholds):

| Rule | Condition | Severity |
|---|---|---|
| `entropy_spike` | `\|H_q - median_H\| > 3 * MAD_H` | warning |
| `phi_anomaly` | `phi_scalar > p95_phi` (rolling 24h) | warning |
| `sustained_risk` | fraction of events with `phi_scalar > θ` > 0.1 in 1h window | critical |
| `low_z` | `ln_Z < percentile_5_ln_Z` (baseline) | warning |
| `zero_effective` | `n_eff < 1.1` (single-candidate collapse) | critical |

Alerts include: alert_type, trigger event_id, window boundaries,
metric/baseline values, and a structured payload for operator investigation.

#### Stage 4 — Analysis

See AQL examples in §7.

#### Stage 5 — Retention

- **Hot tier (0–7 days):** full event granularity, all signals.
- **Warm tier (7–30 days):** hourly aggregates + sampled raw events (5%).
- **Cold tier (30+ days):** daily aggregates only.
- Data retention respects applicable regulations (GDPR where applicable:
  `query_hash` is a pseudonym; raw queries must not be stored without consent).

---

## 6. ThemisDB Hybrid Model Mapping

| Storage tier | Data | Access pattern |
|---|---|---|
| **Relational** | `ObservabilityEvent`, `AlertEvent`, `RollingBaseline`, `QueryCluster` | SQL/AQL aggregations, time-range scans, alert lookups |
| **Vector** | Query embedding index (per `query_hash`) | Similarity search: "find queries most similar to this anomalous one" |
| **Graph** | Provenance graph: Event → Document → Source nodes | Path queries: "which sources most often appear in high-Φ events?" |
| **File / Blob** | Long-term raw event archives (compressed Parquet), evaluation snapshots | Bulk export, offline analysis, replay benchmarks |

### Graph Provenance Schema

```
ObservabilityEvent ──[retrieved]──► Document ──[originates_from]──► Source
                   ──[generated]──► AnswerNode
Document           ──[authored_by]──► Author (optional)
```

Graph query example — identify sources correlated with risky retrievals:

```aql
MATCH (e:ObservabilityEvent)-[:retrieved]->(d:Document)-[:originates_from]->(s:Source)
WHERE e.phi_scalar > :phi_threshold
  AND e.timestamp >= :window_start
GROUP BY s.source_id, s.name
ORDER BY AVG(e.phi_scalar) DESC
LIMIT 20
```

---

## 7. AQL Query Examples

### 7.1 Entropy Drift (Time-Series View)

```sql
-- Hourly entropy trend over the last 7 days
SELECT
    DATE_TRUNC(timestamp, 'hour')                                 AS bucket,
    COUNT(*)                                                       AS n_events,
    AVG(entropy_H)                                                 AS mean_H,
    PERCENTILE_CONT(0.50) WITHIN GROUP (ORDER BY entropy_H)       AS median_H,
    PERCENTILE_CONT(0.95) WITHIN GROUP (ORDER BY entropy_H)       AS p95_H,
    AVG(phi_scalar)                                                AS mean_phi
FROM ObservabilityEvent
WHERE timestamp >= NOW() - INTERVAL '7 days'
GROUP BY bucket
ORDER BY bucket;
```

Use case: plot time series of entropy to detect gradual or sudden drift.
A sustained upward trend in `mean_H` suggests the query distribution is
diverging from index coverage.

### 7.2 High-Risk Low-Entropy Events (Over-Confident Failures)

```sql
-- Events with concentrated retrieval (low entropy) but high free-energy risk
-- Cross-referenced with negative user feedback
SELECT
    event_id,
    timestamp,
    entropy_H,
    n_eff,
    phi_scalar,
    energy_gap,
    feedback_label
FROM ObservabilityEvent
WHERE entropy_H   < :entropy_low_threshold    -- default: 0.5
  AND phi_scalar  > :phi_high_threshold       -- default: p95 from RollingBaseline
  AND feedback_label IS NOT NULL
ORDER BY phi_scalar DESC, timestamp DESC
LIMIT 100;
```

Use case: identify cases where the system was over-confident in a specific
candidate but the answer was wrong (negative feedback). These are the most
dangerous failure mode for RAG systems.

### 7.3 Domain-Shift Cluster Candidates

```sql
-- Queries from clusters not seen in the historical baseline
SELECT
    o.event_id,
    o.timestamp,
    o.entropy_H,
    o.phi_scalar,
    qc.cluster_id,
    qc.cluster_centroid_distance
FROM ObservabilityEvent o
JOIN QueryCluster qc ON qc.query_hash = o.query_hash
WHERE o.timestamp >= NOW() - INTERVAL '24 hours'
  AND qc.cluster_id NOT IN (
      SELECT DISTINCT cluster_id
      FROM QueryCluster
      WHERE snapshot_date < NOW() - INTERVAL '30 days'
  )
ORDER BY o.phi_scalar DESC, qc.cluster_centroid_distance DESC;
```

Use case: surface queries that belong to novel clusters (no historical
precedent), which may indicate out-of-distribution queries or emerging topics.

### 7.4 Latency-Entropy Correlation Check

```sql
-- Pearson correlation proxy: bucket by entropy quartile, show mean latency
SELECT
    NTILE(4) OVER (ORDER BY entropy_H) AS entropy_quartile,
    MIN(entropy_H)                      AS H_min,
    MAX(entropy_H)                      AS H_max,
    AVG(latency_ms)                     AS mean_latency_ms,
    PERCENTILE_CONT(0.95) WITHIN GROUP (ORDER BY latency_ms) AS p95_latency_ms,
    COUNT(*)                            AS n
FROM ObservabilityEvent
WHERE latency_ms IS NOT NULL
  AND timestamp >= NOW() - INTERVAL '24 hours'
GROUP BY entropy_quartile
ORDER BY entropy_quartile;
```

Use case: test Hypothesis 3 from the research paper — whether high entropy
events are associated with elevated latency.

### 7.5 Alert Investigation — Recent Critical Alerts with Context

```sql
SELECT
    a.alert_id,
    a.created_at,
    a.alert_type,
    a.metric_value,
    a.baseline_value,
    o.entropy_H,
    o.phi_scalar,
    o.n_eff,
    o.query_hash,
    o.k_retrieved
FROM AlertEvent a
JOIN ObservabilityEvent o ON o.event_id = a.trigger_event_id
WHERE a.severity = 'critical'
  AND a.created_at >= NOW() - INTERVAL '1 hour'
ORDER BY a.created_at DESC;
```

---

## 8. Deployment Options

### Option A — Sidecar (Recommended for initial deployment)

The RAG service emits observability events asynchronously to a dedicated
ThemisDB collection endpoint. No impact on critical RAG path.

```
RAG Service ──async POST──► ObservabilityIngestService ──► ThemisDB
                                     │
                         (signal computation + write)
```

**Pros:** zero latency impact on RAG, easy to disable.
**Cons:** events may arrive out of order; small risk of event loss under load.

### Option B — Inline (Higher fidelity)

The ThemisDB RAG module computes signals inline before returning results.
Events are written synchronously.

```
RAG Request ──► ThemisDB RAG Module
                   │
             compute signals (< 1 ms for k ≤ 100)
                   │
             write ObservabilityEvent
                   │
             return RAG result
```

**Pros:** guaranteed event ordering, no loss.
**Cons:** adds ~1–2 ms to synchronous RAG path; couples observability
to RAG availability.

### Option C — Replay-Based (Evaluation Only)

Post-hoc computation of signals from stored RAG logs. No production integration.
Used for initial evaluation of signal quality before committing to Option A or B.

---

## 9. Failure Modes and Mitigations

| Failure mode | Symptom | Mitigation |
|---|---|---|
| **Ingest overflow** | Write queue backlog under high load | Drop oldest events with `WARN` log; circuit-breaker on queue depth |
| **Signal NaN/Inf** | `entropy_H = NaN` due to degenerate scores (all identical, all zero) | Clamp scores before normalization; add epsilon regularization; log degenerate events separately |
| **Baseline staleness** | Rolling baseline not updated; all alerts suppressed or all fired | Heartbeat check on `RollingBaseline` freshness; alert on stale baseline |
| **Clock skew** | Events arrive with future timestamps; time-series corrupted | Server-side timestamp override; reject events > 60 s in the future |
| **k=0 edge case** | Retriever returns empty result set; no scores | Skip signal computation; emit special `empty_retrieval` flag; increment counter |
| **Temperature=0** | Boltzmann weights degenerate (divide by zero) | Enforce `T ≥ 0.01` at ingest; reject configuration with `T < min_T` |
| **Query hash collision** | Different queries hash to same `query_hash` | Use full 256-bit SHA-256; collision risk negligible for practical query volumes |
| **Partitioning failure** | Old partitions not pruned; storage grows unbounded | Automated partition management job with monitoring; alert on partition count |

---

## 10. FLARE Parallel and Complementarity

FLARE (Jiang et al. 2023) monitors token-level probabilities during
generation and triggers re-retrieval when confidence drops below a threshold.

The Boltzmann observability layer is **complementary, not competitive**:

| Aspect | FLARE | Boltzmann Layer |
|---|---|---|
| Timing | Real-time, within generation | Async, post-hoc or parallel |
| Granularity | Per token | Per query/retrieval call |
| Signal | Generator token probability | Retrieval score distribution |
| Action | Proactive: prevents bad generation | Reactive: detects systemic patterns |
| Scope | Single request | Aggregated across many requests |
| Storage | None (stateless) | Persistent event store |

**Recommended integration:**
- Deploy FLARE (or equivalent) for per-request quality correction.
- Deploy Boltzmann layer for system-level drift monitoring and alerting.
- Cross-reference: when FLARE re-retrieval rate spikes in a time window,
  the Boltzmann layer should show a corresponding $H$ or $\Phi$ anomaly.
  Consistency between the two is a useful diagnostic signal.

---

## 11. Iterative Roadmap

### MVP (Phase 1) — Signal Pipeline

**Goal:** basic signal computation and storage, no alerting.

- [ ] Define and create `ObservabilityEvent` table in ThemisDB schema.
- [ ] Implement server-side signal computation (C++: `BoltzmannSignals` struct).
- [ ] Implement `POST /observability/events` ingest endpoint.
- [ ] Unit tests: signal computation vs. Python reference implementation.
- [ ] Integration test: ingest 1,000 synthetic events, verify signal correctness.

**Acceptance criteria:**
- Signal computation matches reference Python within `1e-6` absolute error.
- Ingest endpoint handles 5,000 events/s without queue backlog.

### v1 — Rolling Baseline and Alerting

**Goal:** operational alerting on drift and anomalies.

- [ ] Implement `RollingBaseline` materialized view with 1h/24h/7d windows.
- [ ] Implement alert rule engine (entropy spike, phi anomaly, sustained risk).
- [ ] Write `AlertEvent` on trigger; expose via `/alerts` API.
- [ ] AQL views for entropy drift, high-risk low-entropy, latency correlation.
- [ ] Integration tests: inject synthetic anomalies, verify alert trigger latency
      and false-positive rate on anomaly-free periods.

**Acceptance criteria:**
- Alert trigger latency ≤ 30 s from event ingest.
- False-positive rate ≤ 5% on 24h anomaly-free synthetic stream.

### v2 — Correlation Analysis and Graph Provenance

**Goal:** end-to-end correlation analysis and source attribution.

- [ ] Vector index for query embeddings (`query_hash` → embedding).
- [ ] `QueryCluster` materialized view with async k-means refresh.
- [ ] Graph provenance schema: Event → Document → Source.
- [ ] Domain-shift cluster AQL query.
- [ ] Source-attribution graph query.
- [ ] Correlation reports (Pearson/Spearman of $H$, $\Phi$ vs. quality labels).
- [ ] Partition retention management (hot/warm/cold).

**Acceptance criteria:**
- Domain-shift query returns results within 400 ms p95 on 30-day event store.
- Correlation reports match offline Python analysis within ±0.02 Pearson $r$.

---

## 12. Open Questions

1. **Temperature calibration:** what is the correct $T$ for a given embedding
   model and index? Should $T$ be adaptive (learned from feedback)?
2. **Score normalization:** similarity scores from different embedding models
   (dot product vs. cosine, different scales) are not directly comparable.
   Should normalization happen at ingest or at query time?
3. **Event schema versioning:** as signals evolve, how do we handle schema
   migrations without breaking existing AQL queries?
4. **GDPR and privacy:** `query_hash` pseudonymises queries, but timing +
   cluster membership may re-identify users. Requires data-protection review
   before production deployment.
5. **Multi-retriever pipelines:** RAG pipelines with multiple retrieval stages
   (hybrid dense+sparse) produce multiple score vectors. Which to use? Or
   should signals be computed per-stage?

---

*This document is an ACTIVE_DRAFT. All design decisions are subject to
revision. See [research/boltzmann_flare_rag_monitoring.tex](../../research/boltzmann_flare_rag_monitoring.tex)
for the scientific foundations and evaluation protocol.*
