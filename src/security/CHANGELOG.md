> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-05-31 -->
# Changelog — Security Module
Based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Changed
- Added Wave-C production-validation focused coverage in `tests/security/test_security_wavec_production_validation_focused.cpp`:
  - Vault production config hardening and fail-closed validation.
  - HSM stub/production guardrail enforcement validation.
  - Failure-injection matrix for dependency/malformed-response handling.
  - Mixed ABAC+RLS real-query workload filtering validation.
  - Conflict-resolution and concurrent policy-update atomicity checks.
  - Sustained-load integrity and boundary/license/hash/SBOM policy gate checks.
- Documentation governance alignment pass:
  - `ROADMAP.md` and `FUTURE_ENHANCEMENTS.md` kept future-focused.
  - `AUDIT.md`, `README.md`, `ARCHITECTURE.md`, `SECURITY.md`, and `PERFORMANCE_EXPECTATIONS.md` refreshed with sourcecode verification evidence blocks.
  - Historical implementation record remains centralized in `CHANGELOG.md`.
- `AI_SAFETY_ARCHITECTURE.md` refreshed to document only source-verified AI-safety controls in `src/security/`; over-assertive approval, rollback, and orchestration claims were removed.

## [1.6.0] — 2026-03-24
### Added
- **USB Volume Hardening** (`include/security/usb_volume_hardening.h`, `src/security/usb_volume_hardening.cpp`)
  — Defence-in-depth against FAT filesystem manipulation on USB admin sticks:
  - `computeVolumeHash()` / `verifyVolumeHash()` — SHA-256 of the license file content;
    any FAT-level replacement or byte-level edit is detected before the license is parsed.
  - `isMountedReadOnly()` — verifies `/proc/mounts` (Linux) or `FILE_READ_ONLY_VOLUME` (Windows);
    read-only mount enforcement prevents live writes to the stick during authentication.
  - `getUSBDeviceSerial()` / `verifyUSBSerial()` — reads SCSI VPD serial via sysfs on Linux,
    volume serial on Windows; prevents `dd`-cloned sticks from being accepted.
  - All hash/serial comparisons use `CRYPTO_memcmp` (constant-time) to prevent timing attacks.
- Three new `USBAdminConfig` fields: `require_readonly_mount`, `expected_volume_hash`,
  `expected_usb_serial` — all opt-in; existing deployments are unaffected.
- Three new `Metrics` counters: `usb_denied_not_readonly`, `usb_denied_volume_hash_mismatch`,
  `usb_denied_serial_mismatch` — for monitoring and alerting.
- All hardening rejections produce structured audit-log entries with event names
  `USB_DENIED_NOT_READONLY`, `USB_DENIED_VOLUME_HASH_MISMATCH`, `USB_DENIED_SERIAL_MISMATCH`.
- 22 tests in `tests/test_usb_volume_hardening.cpp`; `USBVolumeHardeningFocusedTests`
  standalone target.

## [1.5.0] — 2026-03-12
### Added
- Post-quantum cryptography support (Kyber KEM, Dilithium signatures)
- HSM-backed SigningService for hardware-protected key operations
- QueryMaskingPolicy for PII field masking in query results
- Secret manager with vault integration (Vault, AWS Secrets Manager)
- Security evidence collector for compliance reporting
- Certificate rotation automation

## [1.0.0] — 2024-01-01
### Added
- AES-256-GCM field-level encryption
- PKI certificate management (X.509, GPG)
- RBAC policy enforcement
