<!-- Status: current | validated: 2026-04-15 -->
<!-- Links: README.md · ARCHITECTURE.md · ../../src/ingestion/ROADMAP.md -->

# Roadmap — Ingestion Module (Public Headers)

> Implementation roadmap: `../../src/ingestion/ROADMAP.md`

## Current Status

v1.6.0 — Production-ready. 15 public headers. LLM adapter Phase 1 complete.
Infrastructure now exposed globally via `toolbox::IngestionToolbox`.

## Completed ✅

- [x] 9 source connectors (API, CDC, Kafka, DB, S3, filesystem, HuggingFace, web crawler, object storage)
- [x] Semantic and deontic validation
- [x] LLM enrichment adapter (Phase 1)
- [x] Agentic reference validator
- [x] Worker coordination with in-memory checkpoint store

## Planned

- [x] SoC refactoring: `ITextGenerationBackend` interface decouples ingestion from `llm/` headers — `LegalLlmAdapter` now uses injected backend; `LlmIngestionBridge` in `llm/` is the only cross-module binding (Target: v1.6.0)
- [x] SoC Phase 2: `SemanticValidator::setExtractor(DeonticExtractor)` — injection point for LLM-backed extractor (no `llm/` includes in `semantic_validator`) (Target: v1.6.0)
- [x] SoC Phase 3: `IngestionManager::setTextGenerationBackend(shared_ptr<ITextGenerationBackend>)` — pipeline-level backend injection; `runLegalExtraction()` wires backend into `SemanticValidator` (Target: v1.6.0)
- [x] Global toolbox: `WorkflowEngine`, `StepRegistry`, and `ITextGenerationBackend` exposed system-wide via `toolbox::IngestionToolbox` (see `include/toolbox/ROADMAP.md`) (Target: v1.6.0)
- [ ] Phase 2 LLM pipeline: LoRA fine-tuning, SpaCy NLP, agentic verification (Target: v1.7.0)
- [ ] Distributed checkpoint store (etcd-backed) (Target: v1.7.0)
- [ ] Extended binary MIME detection XLSX/ODT/RTF (Target: v1.7.0)
- [x] Extraction throughput + LLM-latency benchmark (`bench_ingestion_extraction.cpp`): DeonticExtractor regex throughput, LegalLlmAdapter fn call latency, MIME detection, checkpoint store overhead

## Production Readiness Checklist

- [x] 9 source connectors stable
- [x] LLM adapter Phase 1 tested
- [x] `ITextGenerationBackend` is the shared LLM abstraction across `ingestion/`, `llm/`, and `aql/` (via `toolbox/`)
- [x] `WorkflowEngine` + `StepRegistry` accessible globally via `toolbox::IngestionToolbox`
- [ ] Phase 2 LLM pipeline
- [ ] Distributed checkpointing
