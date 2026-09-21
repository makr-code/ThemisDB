# Changelog — ai_working Module

<!-- Status: current | validated: 2026-09-21 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

All notable governance and structural changes to the `ai_working` module are documented here.
The format follows Keep a Changelog conventions.

## [Unreleased]

### Added
- `scripts/archive_ai_working.py` — archival automation script with `--archive-stale` and `--check-freshness` modes (2026-09-21).
- Freshness validation step (`--check-freshness`) integrated into `maintenance-ai-working.yml` as CI gate (2026-09-21).
- Archive-stale step (`--archive-stale`) integrated into `maintenance-ai-working.yml` maintenance job (2026-09-21).
- Full governance documentation set: README.md, ARCHITECTURE.md, AUDIT.md, CHANGELOG.md, FUTURE_ENHANCEMENTS.md, PERFORMANCE_EXPECTATIONS.md, PRODUCTION_REQUIREMENTS.md, ROADMAP.md, SECURITY.md (2026-09-21).
- Governance docs aligned to module-quick-rules.md and MODULE_DEVELOPER_DOCUMENTATION_GUIDELINES.md.

### Changed
- Replaced placeholder README.md, ROADMAP.md, and API.md in `docs/ai_working/` with substantive content.
- Updated MODULE_GAPS.md to reflect resolved documentation gap.

### Archived
- `src/ai_working/BATCH1_ANALYTICS_HARDENING.md` → moved to `docs/ARCHIVED/ai-working-history/` (analytics wave-specific artifact, no longer active in ai_working).
- `src/ai_working/BATCH1_STATUS.md` → moved to `docs/ARCHIVED/ai-working-history/` (analytics wave-specific artifact, no longer active in ai_working).

## [2026-09-04]

### Added
- BATCH1 analytics hardening implementation plan and status documents (now archived).

## [2026-09-02]

### Added
- Wave A/B execution artifacts in `ai_working/` root directory.
- Initial module scaffold for `src/ai_working/` (CMakeLists placeholder, MODULE_GAPS.md).

## [2026-08-26]

### Added
- Wave planning documents for Wave A Q3 modules.
