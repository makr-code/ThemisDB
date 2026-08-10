# Experiment Protocol: Database-Native Observability for RAG and AI Inference

**Manuscript**: `research/manuscripts/systems/DB_NATIVE_OBSERVABILITY_RAG_AI_INFERENCE_PAPER_DRAFT.md`  
**Status**: PROTOCOL_DRAFT  
**Last Updated**: 2026-08-10

---

## Objective

Validate that ThemisDB's observability runtime surfaces (metrics, tracing, SLO reporting, anomaly detection) can detect retrieval quality drift and inference degradation with measurable time-to-detect and alert precision.

---

## Experiment Suite

### Suite O1 — Retrieval Entropy Baseline

Capture score-distribution entropy for a stable RAG workload at rest:
- 1,000 queries against a fixed vector index
- Record: per-query top-K score distribution, entropy H, candidate concentration (ratio of top-1 score to top-K mean)
- Establish baseline μ(H) and σ(H)

### Suite O2 — Drift Detection

Inject embedding distribution shift (replace 30% of index with off-distribution embeddings):
- Measure: time from injection to anomaly alert (time-to-detect, TTD)
- Measure: alert precision = (true-positive drifted queries detected) / (total alerts)
- Compare: entropy-threshold detector vs. sliding-window concentration change detector

### Suite O3 — Inference Degradation Under KV-Cache Pressure

Inject KV-cache pressure (reduce available memory by 50%):
- Measure: p95/p99 TTFT regression latency
- Measure: time from TTFT regression onset to observability alert

### Suite O4 — G-Eval Correlation

For W2 (drift) workload: measure G-Eval faithfulness score before and after drift injection. Correlate with entropy signal. Establish Pearson correlation coefficient.

---

## Environment

- Build: `linux-release`
- LLM: Ollama endpoint (`qwen2.5-coder:14b` or `gemma4:latest`)
- Vector index: HNSW, 50K entries (768-dim)
- Observability: `src/observability/` runtime surfaces enabled

---

## Artifact Checklist

- [ ] O1 entropy baseline distribution histogram committed
- [ ] O2 TTD and precision table committed
- [ ] O3 TTFT regression detection latency committed
- [ ] O4 G-Eval / entropy correlation coefficient committed
- [ ] Results at `research/experiments/systems/results/O_<timestamp>.json`
