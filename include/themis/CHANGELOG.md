<!-- Status: current | validated: 2026-04-06 -->

# Changelog — include/themis/

All notable changes to the **public themis core headers** are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
For implementation-level changes see [`../../src/themis/CHANGELOG.md`](../../src/themis/CHANGELOG.md).

---

## [Unreleased]

### Planned
- `runtime_license_gate.h` — add `LicenseDenialReason` enum and `GateResult::message()` accessor

---

## [1.7.0] — 2026-03-12

### Added
- `module_signature_verifier.h` — Ed25519 / RSA-PSS signature verification of
  loaded modules; `SignatureVerificationResult` typed return value
- `module_hash_verifier.h` — SHA-256 hash verification of module binaries;
  `HashVerificationResult` typed return value; `[[nodiscard]]` enforced
- `runtime_license_gate.h` — `FeatureFlag` enum extended with `GPU_ACCELERATION`,
  `DISTRIBUTED_QUERY`, `TEMPORAL_CDC_PERSISTENT` flags for new v1.7 features
- `license_info.h` — `LicenseConstraint::MAX_NODES` and
  `LicenseConstraint::MAX_STORAGE_GB` fields

### Changed
- `edition_manager.h` — `EditionCapabilities` now carries a `std::bitset<64>`
  capability mask replacing the previous `std::vector<std::string>` list (ABI-breaking; soname bumped to `.7`)
- `build_info.h` — `BuildInfo::features()` returns `uint64_t` bitmask aligned
  with `EditionCapabilities`

### Fixed
- `export.h` — `THEMISDB_DEPRECATED` macro was missing the deprecation message
  parameter on MSVC; corrected

---

## [1.6.0] — 2025-12-01

### Added
- `license_info.h` — `LicenseStatus::GRACE_PERIOD` status for expired-but-within-
  grace-window licenses
- `edition.h` — `Edition::DEVELOPER` tier for local development without a
  commercial license

### Changed
- `runtime_license_gate.h` — `RuntimeLicenseGate::check()` now marked
  `[[nodiscard]]`; callers ignoring the result trigger a compiler warning

---

## [1.5.0] — 2025-08-20

### Added
- `module_hash_verifier.h` — initial release (SHA-256 only)
- `runtime_license_gate.h` — initial release with `FeatureFlag` enum and
  `GateResult` struct

### Changed
- `edition_manager.h` — `EditionManager` singleton now initialised lazily on
  first access (thread-safe via `std::call_once`)

---

## [1.4.0] — 2025-04-10

### Added
- `edition_manager.h` — `EditionCapabilities` capability query interface
- `license_info.h` — initial `LicenseInfo` struct and `LicenseStatus` enum

---

## [1.3.0] — 2025-01-15

### Added
- `export.h` — centralised `THEMISDB_API` visibility macro; replaces ad-hoc
  per-header `__declspec(dllexport)` usage
- `build_info.h` — `BuildInfo::version()` static accessor

---

## [1.0.0] — 2024-09-01

### Added
- `edition.h`, `edition_manager.h` — initial ThemisDB edition management
