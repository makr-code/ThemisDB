> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | validated: 2026-07-19 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · MODULE_EVIDENCE.md -->

# Changelog - Analytics Module

All notable changes to the analytics module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Changed
- Security hardening (2026-08-19): `ModelServingEngine::loadModel` now supports explicit SHA-256 integrity verification and optional fail-closed enforcement via `ModelServingConfig::require_model_integrity`.
- Security hardening (2026-08-19): `TFServingConfig` switched to secure defaults (`https://`, `verify_ssl=true`) and blocks plaintext HTTP unless `allow_insecure_transport=true` is explicitly configured.
- Reliability hardening (2026-08-19): `LLMProcessAnalyzer::validateResponse` now enforces stricter schema/type/range checks and bounded payload limits for fraud, 5R, and prediction tasks.
- Test coverage extended for integrity verification, insecure transport gating, and strict LLM response validation behavior.
- Module documentation synchronized and validated (2026-07-19): All roadmap artifacts, future enhancements, and API documentation aligned to source-verifiable module behavior.
- Documentation governance: ROADMAP.md, FUTURE_ENHANCEMENTS.md, README.md, ARCHITECTURE.md, SECURITY.md, PRODUCTION_REQUIREMENTS.md, and CHANGELOG.md all updated with current validation date.
- Added MODULE_EVIDENCE.md to document build/test evidence and closure criteria verification.
- Doxygen API documentation verified: 100% coverage across 24 analytics headers; all public APIs documented with @brief, @param, @return, and @code examples.
- Production readiness checklist: All core runtime surfaces and security/failure behavior documented at module level.

## [2.0.0] - 2026-04-12

### Added
- Multi-stream join engine and focused join test coverage for analytics streaming joins.

## [1.9.0] - 2026-03-28

### Added
- Forecasting batch and streaming-oriented enhancements.

## [1.7.0] - 2026-03-09

### Added
- AutoML, external ML integration, model serving, and forecasting integration surfaces.

## [1.6.0] - 2026-02-15

### Added
- CEP checkpointing/backpressure, streaming windows, incremental views, and anomaly detection additions.

## [1.5.0] - 2026-01-10

### Added
- CEP core, process mining integration surfaces, NLP analyzer, diff engine, and distributed analytics.

## [1.0.0] - 2024-01-01

### Added
- Initial OLAP and export foundations for analytics runtime.