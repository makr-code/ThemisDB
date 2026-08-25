> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - AQL Module

All notable changes to the AQL module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Changed
- Documentation governance sync: README, ARCHITECTURE, SECURITY, ROADMAP, FUTURE_ENHANCEMENTS, AUDIT, and PERFORMANCE_EXPECTATIONS aligned to source-verifiable module behavior.
- Performance expectations updated to explicit existing benchmark symbols from current AQL benchmark sources.
- NL→AQL retry prompts now sanitize and delimit validation feedback before reinjection, reducing prompt-injection risk from model-generated retry diagnostics (`llm_aql_handler.cpp`).
- `AQLIngestionBridge` destructor is now explicitly `noexcept` to codify non-throwing teardown semantics.
- `LLMAQLEmbeddingBridge` degraded-mode failures are elevated from debug to warning logs, removing effectively silent production fallback behavior.
- Translation retry attempts in `translateNLToAQL*` now honor `validation_config.max_retries` instead of hardcoded retry defaults.
- `LLMValidationPipeline` now fails fast when the LLM client is unavailable and injects parser retry feedback into subsequent generation attempts.
- `LLMExtractiveCompressor` now uses real readiness checks, deterministic ranking fallback when LLM ranking is unavailable, and warning-level logging for persistence failures.

## [1.8.0] - 2026-03-22

### Added
- Multi-modal and async backend related AQL assistance additions.

## [1.7.0] - 2026-03-09

### Added
- token-stream, agent framework, streaming explain, and few-shot additions.

## [1.6.0] - 2026-01-15

### Added
- validator, query-builder/template, finetuner, migration, and optimizer-advisor additions.

## [1.5.0] - 2025-11-01

### Added
- highlighter, confidence, context, autocomplete, schema-aware and docs-assistant additions.

## [1.0.0] - 2024-06-01

### Added
- initial LLM-AQL command handling and NL-to-AQL assistance foundations.