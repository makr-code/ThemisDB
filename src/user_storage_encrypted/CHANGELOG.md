> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | source-reviewed: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->
<!--
Author: ThemisDB Contributors
Created: 2026-08-08
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
Exact model backend not verifiable from final document text alone.
-->

# Changelog - User Storage Encrypted Module

All notable changes to the user_storage_encrypted module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Changed
- Issue #7392 documentation sync: corrected module API/configuration, rotation and KDF integration boundaries, test registration, and benchmark evidence limits. No implementation changes.
- Documentation governance sync: README, ARCHITECTURE, SECURITY, ROADMAP, FUTURE_ENHANCEMENTS, AUDIT, and PERFORMANCE_EXPECTATIONS aligned to source-verifiable module behavior.
- Performance expectations updated from stale parent-module proxy mapping to explicit native benchmark symbols from the encrypted storage benchmark suite.

## [0.3.x] - 2026

### Added
- broader metrics, lifecycle hardening, and encrypted storage runtime refinements.

## [0.2.x] - 2026

### Added
- stale mount reconciliation and related encrypted storage lifecycle hardening.

## [0.1.x] - 2026

### Added
- foundational encrypted backend, key derivation, and rotation scheduling behavior.