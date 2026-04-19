> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
# Changelog — Themis Module
Based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.7.0] — 2026-03-12
### Added
- `wire_protocol_server.cpp` — `themis::wire` namespace Phase 3 deliverable
- `module_dependency_resolver.cpp` — `ModuleDependencyResolver` (registerModule, resolve, topologicalSort)
- `module_hash_verifier.cpp` — SHA-256 module integrity verification (Issue #2471)
- `module_signature_verifier.cpp` — Authenticode/GPG signature verification (Issue #2473)
- `edition_manager.cpp` — dynamic feature-flag override API (Issue #2469)
- `build_info.cpp` — migrated from `src/utils/`; 9 free functions with `THEMIS_BASE_API`
- `license_info.cpp` — migrated from `src/utils/`; 6 free functions + `LicenseClient`
- Windows CI job `windows-compile-check` added to `themis-core-ci.yml`
- `RegistryConfig::pinned_public_key` field for CURLOPT_PINNEDPUBLICKEY certificate pinning
- THEMIS_BASE_API export macros on all 5 public API headers

### Changed
- Module loader split from `src/base/module_loader.cpp` into 4 files in `src/themis/`

## [1.0.0] — 2024-01-01
### Added
- Edition management (Community, Professional, Enterprise)
- License validation and enforcement
- Module loading and lifecycle management
