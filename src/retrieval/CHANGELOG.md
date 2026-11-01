---
Author: "ThemisDB Contributors"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "ThemisDB documentation orchestration assistant"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

> Historical changelog entries describe implementation state at the time they were recorded; they are not current feature status.

<!-- Historical entries retained; checked for current-status labeling on 2026-10-31. -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - Retrieval Module

All notable changes to the retrieval module are documented here.
The format is based on Keep a Changelog.

## [Unreleased]

### Changed
- Added an explicitly configured, synchronous RocksDB production path for the
  LoRA manifest store while retaining a non-production default constructor for
  source compatibility; that mode rejects all artifact writes and deployment.
- Removed arbitrary `setSignatureVerifier()` injection. Callers must provision
  pinned Ed25519 keys in retrieval YAML; see the API migration notes.
- Added canonical `config/ai_ml/retrieval.yaml` loading through
  `ConfigPathResolver` and yaml-cpp, with pinned-key validation using the shared
  `SignedPluginRepository` Ed25519 verifier.
- Production writes/imports and reopen loading now reject every unsigned,
  malformed, or invalidly signed artifact.
- Made artifact signature checks fail closed on missing signatures or pins and
  bound verification to ThemisDB's shared pinned-key Ed25519 primitive.
- Added quality-gated product deployment with an independent authorization
  callback boundary; authorization remains owned by the deployment service.

## Historical documentation updates (legacy Unreleased section; entry dates not recorded)

### Changed
- Added root governance docs (`ARCHITECTURE`, `SECURITY`, `PERFORMANCE_EXPECTATIONS`, `FUTURE_ENHANCEMENTS`, `ROADMAP`, `AUDIT`) aligned to the current EPIC 1 scaffold state.
- Kept roadmap/future planning and historical change tracking separated.

## [0.1.0] - 2026-06-01

### Added
- Initial EPIC 1 contract and scaffold documentation set (`README`, `include/README`, `src/README`).
