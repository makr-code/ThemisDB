# Experiment Protocol: Transactional Wiki Knowledge Ingestion

**Manuscript**: `research/manuscripts/llm_runtime_training/LORA_WIKI_TRANSACTIONAL_KNOWLEDGE_INGESTION_PAPER_DRAFT.md`  
**Status**: PROTOCOL_DRAFT  
**Last Updated**: 2026-08-10

---

## Objective

Measure Phase A ingestion throughput, freshness lag, and adapter hot-swap latency for the LLM Wiki plugin under concurrent serving load.

---

## Prerequisite

Phase A (`JsonWikiIndexReader` + FNV hash) must be built from `plugins/private/themisdb_llm_wiki/`. Phase B (`THEMISDB_WIKI_PHASE_B`) is excluded from this protocol.

---

## Experiment Suite

### Suite W1 — Baseline Ingestion Throughput

- Input: JSON wiki index dumps at three sizes: 1K, 10K, 100K entries
- Metric: entries/sec per batch size
- Condition: no concurrent serving load

### Suite W2 — Ingestion Under Concurrent Serving

- Serving load: 100 req/sec, 500 req/sec, 1,000 req/sec (LLM query against current index)
- Input: 10K-entry ingestion batch
- Metric: ingestion throughput degradation (entries/sec) vs. baseline W1

### Suite W3 — Freshness Lag

- After ingestion batch commits: measure `WikiStatus.last_update_epoch` update latency
- Measure: time from ingestion start to `WikiStatus.is_ready == true && last_update_epoch > pre_ingestion_epoch`

### Suite W4 — Rollback Correctness

- Inject partial ingestion failure at 50% of batch
- Verify: serving path sees `last committed index state` (entries from before failed batch, not partial entries)
- Test ID mapping: extend LWP-01..LWP-08 with W4-01 rollback test

---

## Environment

- Build: `linux-release` with LLM Wiki plugin submodule initialized
- LLM: Ollama endpoint (serving path)
- Wiki source: Wikidata JSON dump sample (100K entities)

---

## Artifact Checklist

- [ ] W1 throughput table (entries/sec vs. batch size) committed
- [ ] W2 throughput degradation table committed
- [ ] W3 freshness lag p50/p95/p99 committed
- [ ] W4 rollback correctness verified (zero partial-entry exposure)
- [ ] Results at `research/experiments/llm_runtime_training/results/W_<timestamp>.json`
