> WARNING: Historical changelog entries describe implementation state at the time they were recorded.

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog - Distributed Tensor Module

All notable changes to the distributed tensor module are documented here.
The format is based on Keep a Changelog.

<!--
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot Coding Agent
Human_Review: false
Human_Author: unknown
Exact model backend not verifiable from final document text alone.
-->

## [Unreleased]

### Added
- Added a bounded, versioned adapter-similarity graph projection codec and public planner validation path. Only graph-supported evidence can pass strict validation; the source and focused regression tests remain unbuilt and unrun.

### Changed
- Replaced the active integrity engine's FNV-1a demonstration hash with conditional OpenSSL EVP SHA-256, fail-closed behavior, canonical versioned receipt inputs, and rejection of unsupported or malformed digests. Legacy receipts are incompatible; hashing does not authenticate a producer. Build and test evidence remains pending.
- Reconciled module documentation with current source, public-header, test, benchmark, and CMake inventories; clarified the C++20 `std::span` API requirement and synchronous in-process training callbacks. Historical gate/pass claims remain marked as unverified current status (Issue #7341).
- Declared `cxx_std_20` as a public compile feature of `themis_distributed_tensor`, matching the C++20 `std::span` public-header requirement.
- Added root governance docs (`ARCHITECTURE`, `SECURITY`, `PERFORMANCE_EXPECTATIONS`, `FUTURE_ENHANCEMENTS`, `ROADMAP`, `AUDIT`) aligned to current EPIC 3 scaffold state.
- Kept roadmap/future planning and historical change tracking separated.

## [0.1.0] - 2026-06-01

### Added
- Initial EPIC 3 contract and scaffold documentation set (`README`, `include/README`, `src/README`).
