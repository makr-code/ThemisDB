<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/ingestion/ROADMAP.md -->

# Roadmap — Ingestion Module (Public Headers)

> Implementation roadmap: `../../src/ingestion/ROADMAP.md`

## Current Status

v1.5.1 — Production-ready. 15 public headers. LLM adapter Phase 1 complete.

## Completed ✅

- [x] 9 source connectors (API, CDC, Kafka, DB, S3, filesystem, HuggingFace, web crawler, object storage)
- [x] Semantic and deontic validation
- [x] LLM enrichment adapter (Phase 1)
- [x] Agentic reference validator
- [x] Worker coordination with in-memory checkpoint store

## Planned

- [ ] Phase 2 LLM pipeline: LoRA fine-tuning, SpaCy NLP, agentic verification (Target: v1.6.0)
- [ ] Distributed checkpoint store (etcd-backed) (Target: v1.6.0)
- [ ] Extended binary MIME detection XLSX/ODT/RTF (Target: v1.6.0)
- [x] Extraction throughput + LLM-latency benchmark (`bench_ingestion_extraction.cpp`): DeonticExtractor regex throughput, LegalLlmAdapter fn call latency, MIME detection, checkpoint store overhead

## Production Readiness Checklist

- [x] 9 source connectors stable
- [x] LLM adapter Phase 1 tested
- [ ] Phase 2 LLM pipeline
- [ ] Distributed checkpointing
