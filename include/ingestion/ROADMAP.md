<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/ingestion/ROADMAP.md -->

# Roadmap — Ingestion Module (Public Headers)

> Implementation roadmap: `../../src/ingestion/ROADMAP.md`

## Current Status

v1.6.0 — Production-ready. 15 public headers. LLM adapter Phase 1 complete.

## Completed ✅

- [x] 9 source connectors (API, CDC, Kafka, DB, S3, filesystem, HuggingFace, web crawler, object storage)
- [x] Semantic and deontic validation
- [x] LLM enrichment adapter (Phase 1)
- [x] Agentic reference validator
- [x] Worker coordination with in-memory checkpoint store
- [x] Extended binary MIME detection XLSX/ODT/RTF (v1.6.0)

## Planned

- [ ] Phase 2 LLM pipeline: LoRA fine-tuning, SpaCy NLP, agentic verification (Target: v1.7.0)
- [ ] Distributed checkpoint store (etcd-backed) (Target: v1.7.0)

## Production Readiness Checklist

- [x] 9 source connectors stable
- [x] LLM adapter Phase 1 tested
- [x] Extended binary MIME detection (XLSX/ODT/RTF)
- [ ] Phase 2 LLM pipeline
- [ ] Distributed checkpointing
