<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Ingestion Module (Public Headers)

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
For implementation-level changes see `../../src/ingestion/CHANGELOG.md`.

## [Unreleased]
- Phase 2 LLM pipeline: LoRA fine-tuning integration, SpaCy NLP, agentic verification loop
- Distributed checkpoint store (etcd-backed)
- Extended binary MIME detection (XLSX, ODT, RTF)

## [1.5.1] — 2026-03-21
### Added
- CI workflow `ingestion-llm-adapter-ci.yml` for `IngestionLlmAdapterFocusedTests`

## [1.5.0] — 2026-03-12
### Added
- `llm_adapter.h`: LLM enrichment adapter (Phase 1 — legal document processing)
- `agentic_reference_validator.h`: agentic reference validation loop

## [1.0.0] — 2024-01-01
### Added
- All source connector headers (API, CDC, Kafka, database, S3, filesystem, HuggingFace, web crawler)
- `semantic_validator.h`, `deontic_extractor.h`, `ingestion_coordinator.h`
