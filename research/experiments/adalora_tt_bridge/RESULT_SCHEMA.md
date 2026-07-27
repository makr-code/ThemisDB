# AdaLoRA↔TT Bridge — Result Schema

**Document type:** Schema specification (machine-readable + human-readable)  
**Version:** 1.0  
**Last updated:** 2026-07-27  
**JSON Schema:** `result_schema.json` (validates `summary.json` files in `results/`)  
**Protocol reference:** `../ADALORA_TT_BRIDGE_BENCHMARK_PROTOCOL.md`

---

## 1. Summary File Structure (`summary.json`)

Every completed benchmark run produces a `summary.json` in its result directory.
The JSON Schema in `result_schema.json` machine-validates this file.

### 1.1 Top-level fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `schema_version` | string | yes | `"1.0"` |
| `track_id` | string | yes | `"bt1"`, `"bt2"`, `"bt3"`, or `"bt4"` |
| `variant` | string | yes | Experiment variant identifier (e.g., `"cold_load_rank8"`) |
| `date` | string (ISO 8601) | yes | Run date (`YYYY-MM-DD`) |
| `env_sha256` | string | yes | First 8 chars of SHA-256 of `env.json` |
| `themisdb_git_sha` | string | yes | Full 40-char ThemisDB git commit SHA |
| `gate_status` | string | yes | `"cleared"`, `"blocked"`, or `"not_applicable"` |
| `corpus_type` | string | yes | `"synthetic"` or `"real"` |
| `measurements` | array | yes | Array of measurement records (see §1.2) |
| `validity_flags` | object | yes | Validity assessment (see §1.3) |
| `notes` | string | no | Free-text run notes, anomalies, deviations |

### 1.2 Measurement record

Each element of `measurements` describes one experimental cell from the experiment matrix.

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `experiment_id` | string | yes | E.g., `"BT1-A-cold-r8"` |
| `configuration` | object | yes | Rank, layers, method, threshold — see §1.2.1 |
| `n_runs` | integer | yes | Number of independent repetitions (must be ≥ 30) |
| `latency_us` | object | BT-1, BT-4 | `{mean, stddev, cv_pct, p50, p95, p99}` |
| `bytes_before` | integer | BT-2 | Raw bytes before deduplication |
| `bytes_after` | integer | BT-2 | Raw bytes after deduplication |
| `dedup_ratio` | float | BT-2 | `bytes_after / bytes_before` (0.0–1.0) |
| `dedup_hits` | integer | BT-2 | Number of layers that matched existing fingerprint |
| `fp_count` | integer | BT-2 (BT2-C) | False-positive collision count |
| `recon_error_mean` | float | BT-3 | Mean relative Frobenius reconstruction error across layers |
| `recon_error_max` | float | BT-3 | Max relative Frobenius error (worst-case layer) |
| `rank_before` | integer | BT-3 | Total active rank before pruning |
| `rank_after` | integer | BT-3 | Total active rank after pruning |
| `downstream_accuracy_delta` | float | BT-3-C | Accuracy delta vs. unpruned (null if skipped) |
| `downstream_task` | string | BT-3-C | Task name (null if skipped) |
| `cohens_d` | float | no | Effect size vs. baseline (where applicable) |
| `ci_95_low` | float | no | 95% bootstrap CI lower bound for p50 (BT-1/BT-4) |
| `ci_95_high` | float | no | 95% bootstrap CI upper bound for p50 (BT-1/BT-4) |

#### 1.2.1 Configuration sub-object

| Field | Type | Description |
|-------|------|-------------|
| `rank` | integer | Adapter rank (r) |
| `layers` | integer | Number of adapter layers |
| `in_dim` | integer | Layer input dimension (d) |
| `out_dim` | integer | Layer output dimension (k) |
| `method` | string | `"bridge"`, `"flat_baseline"`, `"adalora_greedy"`, `"tt_rounding"` |
| `dedup_threshold` | float | Similarity threshold (BT-2 only) |
| `tt_rounding_eps` | float | TT-rounding epsilon (BT-3-B only; null otherwise) |
| `cache_state` | string | `"cold"` or `"warm"` (BT-1 only) |
| `corpus_size` | integer | Number of adapters in corpus (BT-2 only) |
| `corpus_domain` | string | `"homogeneous"` or `"heterogeneous"` (BT-2 only) |

### 1.3 Validity flags

| Field | Type | Description |
|-------|------|-------------|
| `n_runs_sufficient` | boolean | `n_runs >= 30` for all cells |
| `cv_within_ceiling` | boolean | Max CV across cells is ≤ 15% |
| `cv_max_pct` | float | Actual maximum CV observed |
| `warmup_protocol_followed` | boolean | 3-phase warmup (50/100/200) applied |
| `bt4_gate_cleared` | boolean | BT-4 GGML bridge gate is cleared (must be true for BT-4 results) |
| `corpus_type_declared` | boolean | `corpus_type` is set to `"synthetic"` or `"real"` |
| `env_descriptor_present` | boolean | `env.json` is present in the run directory |

---

## 2. Summary Table (Human-Readable)

For papers and reports, results should be summarized as follows.

### BT-1: Load-Path Latency

| Experiment | Rank | Cache | p50 (µs) | p95 (µs) | p99 (µs) | CV (%) | n |
|------------|------|-------|----------|----------|----------|--------|---|
| BT1-A-cold-r4  | 4  | cold | TBD | TBD | TBD | TBD | ≥30 |
| BT1-A-cold-r16 | 16 | cold | TBD | TBD | TBD | TBD | ≥30 |
| BT1-A-cold-r64 | 64 | cold | TBD | TBD | TBD | TBD | ≥30 |
| BT1-B-warm-r4  | 4  | warm | TBD | TBD | TBD | TBD | ≥30 |
| BT1-B-warm-r16 | 16 | warm | TBD | TBD | TBD | TBD | ≥30 |
| BT1-B-warm-r64 | 64 | warm | TBD | TBD | TBD | TBD | ≥30 |
| BT1-C-baseline | 4–64 | cold | TBD | TBD | TBD | TBD | ≥30 |

### BT-2: Storage Deduplication

| Experiment | Corpus | Size | Bytes Before | Bytes After | Dedup Ratio | FP Count | n |
|------------|--------|------|-------------|-------------|-------------|----------|---|
| BT2-A-homo-10  | Homogeneous | 10 | TBD | TBD | TBD | — | ≥30 |
| BT2-A-homo-50  | Homogeneous | 50 | TBD | TBD | TBD | — | ≥30 |
| BT2-B-hetero-10 | Heterogeneous | 10 | TBD | TBD | TBD | — | ≥30 |
| BT2-B-hetero-50 | Heterogeneous | 50 | TBD | TBD | TBD | — | ≥30 |
| BT2-C-fp-check | Mixed | 1000 | — | — | — | TBD | ≥30 |

### BT-3: Rank Pruning / Reconstruction Quality

| Experiment | Method | Budget | Recon. Error (mean) | Recon. Error (max) | Rank Before | Rank After | n |
|------------|--------|--------|--------------------|--------------------|-------------|------------|---|
| BT3-A-greedy-10 | AdaLoRA greedy | 10% | TBD | TBD | TBD | TBD | ≥30 |
| BT3-A-greedy-30 | AdaLoRA greedy | 30% | TBD | TBD | TBD | TBD | ≥30 |
| BT3-A-greedy-50 | AdaLoRA greedy | 50% | TBD | TBD | TBD | TBD | ≥30 |
| BT3-B-tt-10     | TT-rounding | 10% equiv. | TBD | TBD | TBD | TBD | ≥30 |
| BT3-B-tt-30     | TT-rounding | 30% equiv. | TBD | TBD | TBD | TBD | ≥30 |
| BT3-B-tt-50     | TT-rounding | 50% equiv. | TBD | TBD | TBD | TBD | ≥30 |
| BT3-C-downstream | Both | 30% | — | — | — | — | ≥30 |

---

## 3. CSV Output Format

`collect_results.py` can export results as CSV with the following header:

```
experiment_id,track_id,variant,date,themisdb_git_sha,corpus_type,n_runs,
rank,layers,method,cache_state,
latency_p50_us,latency_p95_us,latency_p99_us,latency_cv_pct,
bytes_before,bytes_after,dedup_ratio,dedup_hits,fp_count,
recon_error_mean,recon_error_max,rank_before,rank_after,
downstream_accuracy_delta,downstream_task,
cohens_d,ci_95_low,ci_95_high,
n_runs_sufficient,cv_within_ceiling,bt4_gate_cleared
```

Empty / not-applicable fields are written as empty string (not `null` or `NaN`).

---

<!-- Status: schema-complete | validated: 2026-07-27 -->
<!-- Links: result_schema.json · collect_results.py · ../ADALORA_TT_BRIDGE_BENCHMARK_PROTOCOL.md -->
