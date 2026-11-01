> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | validated: 2026-07-17 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - Evaluation Module

> **Update provenance (2026-10-31):** AI_Used: true; AI_Models: unknown;
> AI_Contributor: OpenAI ChatGPT; Human_Review: false; Human_Author: ThemisDB
> Contributors. Exact model backend not verifiable from final document text alone.

All notable changes to the evaluation module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Changed
- Documentation-only synchronization for issue #7344: corrected module
  source/target, include, test and benchmark descriptions to match the checked-in
  headers, implementations and CMake registrations. No code or runtime behavior
  changed.
- Added root governance docs (`ARCHITECTURE`, `SECURITY`, `PERFORMANCE_EXPECTATIONS`, `FUTURE_ENHANCEMENTS`, `ROADMAP`, `AUDIT`) aligned to the shipped EPIC 2.5 planner state and current EPIC 2 mixed-stage delivery.
- Kept roadmap/future planning and historical change tracking separated.
- Added Phase 5 benchmark baselines in `benchmarks/epic2_evaluation/` for issue
  #5428 planner decision, scenario-matrix, and storage-strategy overhead checks.
- Updated evaluation-module docs to reflect the shipped EPIC 2.5 hybrid query planner
  contract, implementation, focused tests, and hardening benchmark surfaces tracked by
  issue #5441.
- Refreshed the evaluation status-governance docs for issue #5643, added
  `MODULE_EVIDENCE.md` and `PRODUCTION_REQUIREMENTS.md`, and recorded the current
  build-evidence blocker caused by missing RocksDB in the local validation environment.

## [0.1.0] - 2026-06-01

### Added
- Initial EPIC 2 contract and scaffold documentation set (`README`, `include/README`, `src/README`).
