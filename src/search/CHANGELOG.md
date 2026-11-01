> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | Last Updated: 2026-10-31 | Source_Level: level1 | SOT_Domain: release-versioning | Related_Milestone: DOC-WEEKLY-2026-W44 -->
<!-- Canonical References: include/search/*.h; cmake/ModularBuild.cmake; tests/search/CMakeLists.txt; tests/CMakeLists.txt; benchmarks/search/CMakeLists.txt -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - Search Module

All notable changes to the search module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Changed
- Documentation audit for issue #7378: corrected search API, result-stream buffering, build/test registration, and benchmark-evidence claims against current headers, CMake files, test sources, and benchmark definitions. No runtime test or benchmark pass is asserted.
- Canonical references: `include/search/*.h`, `cmake/ModularBuild.cmake`, `tests/search/CMakeLists.txt`, `tests/CMakeLists.txt`, and `benchmarks/search/CMakeLists.txt`.
- Documentation governance sync: README, ARCHITECTURE, SECURITY, ROADMAP, FUTURE_ENHANCEMENTS, AUDIT, and PERFORMANCE_EXPECTATIONS aligned to source-verifiable module behavior.
- Performance expectations updated to explicit verified benchmark symbols from hybrid, distributed-hybrid, and vector-search benchmark suites.
- README, ARCHITECTURE, ROADMAP, and FUTURE_ENHANCEMENTS now include explicit Wave B issue-scope traceability to `#5039` with upstream context links `#5040`/`#5038`.

## [2.1.x] - 2026

### Added
- hybrid/distributed merge reliability and search utility hardening improvements.

## [2.0.x] - 2025-2026

### Added
- expanded search capability surfaces across ranking, utility, and distributed paths.

## [1.x] - 2024-2025

### Added
- foundational lexical/vector search fusion infrastructure.

## Issue Scope Traceability

- Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
- dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
- follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
