> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — AI Module

All notable changes to the AI module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

- Phase-2 LLM endpoint wiring in `AIPluginGenerator::generatePlugin()` (Target: v1.6.0, Q3 2026)
- Security sandbox pipeline for generated code artefacts (Target: Q3 2026)
- Extended prompt validation for `required_capabilities` and `dependencies` (Target: Q3 2026)

## [1.9.1] — 2026-05-13

### Changed
- `src/ai/ROADMAP.md`: documentation-consolidation task moved from `[~]` to `[x]` — all Phase-1 docs complete.
- `src/ai/ROADMAP.md`: Phase 6 checklist updated with `ARCHITECTURE.md`, `AUDIT.md`, `SECURITY.md`, `CHANGELOG.md`, `PERFORMANCE_EXPECTATIONS.md` confirmation.
- All module docs validated: 0 lint errors, 0 broken links (`docs-lint.py` + `link-check.py --internal-only`).

## [1.9.0] — 2026-05-11

### Added
- Module documentation consolidated: `src/ai/README.md`, `ROADMAP.md`,
  `FUTURE_ENHANCEMENTS.md`, `PERFORMANCE_EXPECTATIONS.md`.
- Public header API documentation: `include/ai/README.md`.
- Research-based ML enhancement catalogue added to `FUTURE_ENHANCEMENTS.md`
  (Wave A/B/C items: speculative decoding, DPR, Self-RAG, KG completion, federated learning).

## [1.0.0] — 2024-06-01

### Added
- `AIPluginGenerator` class in `include/ai/ai_plugin_generator.h` and
  `src/ai/ai_plugin_generator.cpp`:
  - `Config` struct — `llm_endpoint`, `sandbox_dir`, `output_dir`
  - `validatePrompt(const PluginGenerationPrompt&)` — validates description:
    non-empty, ≤ 8192 characters; returns `Result<void>`
  - `generatePlugin(const PluginGenerationPrompt&)` — Phase-1 implementation:
    validates prompt, logs entry at DEBUG level, returns structured
    `ERR_PLUGIN_LOAD_FAILED` error (LLM endpoint not yet wired)
- `PluginGenerationPrompt` and `GeneratedPlugin` types defined in public header.
- 6 focused unit tests (APG-01..APG-06) covering construction, validation, and
  Phase-1 error paths (`tests/test_ai_plugin_generator.cpp`).
