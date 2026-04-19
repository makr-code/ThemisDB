# Continuous Batching in Database-Native LLM Pipelines

**Status**: Draft  
**Version**: 0.1  
**Last Updated**: 2026-04-19  
**Target Venue**: MLSys, EuroSys, ICDE

---

## I. Abstract

Dieses Paper untersucht Continuous Batching als Kernmechanismus für datenbanknative LLM-Pipelines, in denen Query-Ausführung, Retrieval und Generation eng gekoppelt sind. Ziel ist, Throughput zu erhöhen ohne unkontrollierte Tail-Latency. Der Fokus liegt auf scheduler-seitigen Steuerparametern und KV-Cache-Verhalten.

## II. Problem Statement

Klassische per-request Inferenz verursacht geringe GPU-Auslastung und hohe Varianz bei Burst-Last. In einem DB-nativen Stack müssen zudem transaktionale und retrieval-bezogene Abhängigkeiten berücksichtigt werden. Es fehlt eine systematische Bewertung, wie Continuous Batching unter diesen Randbedingungen skaliert.

## III. Research Questions

1. Wie verändern Batchgröße, Tokenbudget und Parallelitätsgrenzen Throughput und p99?
2. Welche Rolle spielt KV-Cache-Management für Stabilität unter Langkontext-Last?
3. Ab wann kippt der Trade-off zwischen Effizienzgewinn und Tail-Latency?
4. Welche Guardrails (Timeouts, degrade paths) stabilisieren die Pipeline?

## IV. Repository Evidence Registry

- E1: `include/llm/continuous_batch_scheduler.h`
- E2: `include/llm/paged_kv_cache.h`
- E3: `include/llm/speculative_decoder.h`
- E4: `src/aql/llm_aql_handler.cpp`
- E5: `tests/test_llm_aql_handler.cpp`
- E6: `benchmarks/bench_llm_raid_pipeline.cpp`
- E7: `research/LLM_PROCESSING_OPTIMIZATION_PATTERNS.md`

## V. Measurement Plan

- Laststufen: 10, 50, 100, 250 gleichzeitige Requests.
- Scheduler Sweep: max_batch_size, max_tokens_per_batch, chunked prefill.
- Metriken:
  - tokens/s, req/s
  - TTFT, p95/p99 completion latency
  - cache hit/miss, queue depth
  - degraded-mode activation rate

## VI. Claim Boundaries

**Unterstützte Claims:**
- Kernkomponenten für batching und KV-cache sind im Repo vorhanden.
- LLM-AQL Integrationspunkt ist implementiert.

**Deferred Claims:**
- Vollständige Vergleichsdaten gegen alternative Scheduler.
- Hardwareübergreifende Generalisierung.

## VII. Next Milestones

- M1: Baseline vs batching mit festen Prompts
- M2: Parameter-Sensitivität und Stabilitätszonen
- M3: Guardrail-Auswertung bei Überlast
- M4: v0.2 mit reproduzierbarem Benchmark-Protokoll
