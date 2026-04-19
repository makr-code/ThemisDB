<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Security Module (Public Headers)

All notable public API changes. Format: [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Implementation changelog: `../../src/security/CHANGELOG.md`.

## [Unreleased]

## [1.5.0] — 2026-03-12
### Added
- `post_quantum_crypto.h` — `IPostQuantumSigner` and `IPqKem` interfaces for NIST PQC algorithms
- `vram_secure_clear.h` — GPU VRAM zeroing contract for ML workloads
- `security_evidence_collector.h` — compliance evidence export interface
- `zero_trust_policy_enforcer.h` — zero-trust policy evaluation engine interface
- `confidential_computing.h` — TEE/SGX/TDX attestation contract

### Changed
- `key_provider.h`: `KeyMaterial` now carries `algorithm` and `key_usage` fields

## [1.4.0] — 2026-01-15
### Added
- `hsm_security_metrics.h` — Prometheus-compatible HSM metrics
- `usb_admin_authenticator.h` — physical USB token authentication interface
- `transport_security_checker.h` — mTLS/TLS transport validation interface
- `fips_crypto_mode.h` — FIPS 140-2/3 mode enforcement

## [1.3.0] — 2025-09-01
### Added
- `vault_signing_provider.h` — HashiCorp Vault Transit signing provider
- `cms_signing.h` — CMS/PKCS#7 signing/verification interface
- `manifest_signer.h` — signed release manifest interface
- `binary_manifest.h` — typed binary manifest entry

## [1.0.0] — 2024-01-01
### Added
- Initial public header set: `encryption.h`, `rbac.h`, `key_provider.h`, `signing.h`,
  `secret_manager.h`, `row_level_security.h`, `access_control.h`, `output_encoding.h`,
  `pii_redaction_policy.h`, `query_masking_policy.h`
