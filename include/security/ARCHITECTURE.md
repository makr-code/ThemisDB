<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/security/ -->

# Security Module — Public Header Architecture

**Version:** 1.0  
**Last Updated:** 2026-04-06  
**Header Path:** `include/security/`  
**Implementation:** `../../src/security/`

---

## Overview

The Security module exposes public headers for cryptographic operations, identity management,
access control, secrets management, and compliance enforcement across ThemisDB. All production
security logic resides in `../../src/security/`; these headers define the contracts.

---

## Design Principles

1. **Zero-Trust by Default** — every component assumes untrusted input; authentication and
   authorization checks are mandatory at every API boundary.
2. **Key Abstraction** — `key_provider.h` and provider variants (`hsm_key_provider_adapter.h`,
   `vault_key_provider.h`, `pki_key_provider.h`) decouple key lifecycle from business logic.
3. **Post-Quantum Readiness** — `post_quantum_crypto.h` defines interfaces for NIST PQC
   candidates alongside classical algorithms.
4. **Audit-First** — `security_evidence_collector.h` and `hsm_security_metrics.h` ensure every
   security event is observable.
5. **Policy Enforcement Separation** — `rbac.h`, `row_level_security.h`,
   `zero_trust_policy_enforcer.h`, and `pii_redaction_policy.h` are pure policy interfaces with
   no storage coupling.

---

## Interface Inventory

| Header | Classes/Interfaces | Purpose |
|--------|-------------------|---------|
| `access_control.h` | `IAccessControl`, `AccessDecision` | Core ACL evaluation interface |
| `access_control_manager.h` | `AccessControlManager` | Policy lifecycle manager (load/reload/evaluate) |
| `aql_injection_detector.h` | `AqlInjectionDetector` | AQL query injection detection and sanitization |
| `arrow_user_registration_plugin.h` | `ArrowUserRegistrationPlugin` | Apache Arrow IPC user registration extension |
| `binary_manifest.h` | `BinaryManifest`, `ManifestEntry` | Signed binary manifest for integrity verification |
| `cms_signing.h` | `CmsSigner`, `CmsVerifier` | CMS/PKCS#7 signing and verification |
| `confidential_computing.h` | `ConfidentialComputingContext` | TEE/SGX/TDX attestation interface |
| `crypto_capabilities.h` | `CryptoCapabilities` | Runtime crypto algorithm capability discovery |
| `encryption.h` | `IEncryptor`, `IDecryptor`, `EncryptionConfig` | Symmetric/asymmetric encryption contracts |
| `fips_crypto_mode.h` | `FipsCryptoMode` | FIPS 140-2/3 mode enforcement and validation |
| `hsm_key_provider_adapter.h` | `HsmKeyProviderAdapter` | PKCS#11 HSM adapter implementing `IKeyProvider` |
| `hsm_provider.h` | `IHsmProvider`, `HsmSlotInfo` | Generic HSM provider interface |
| `hsm_security_checker.h` | `HsmSecurityChecker` | HSM health and security posture verification |
| `hsm_security_metrics.h` | `HsmSecurityMetrics` | HSM operation metrics (latency, errors, key usage) |
| `key_provider.h` | `IKeyProvider`, `KeyMaterial` | Abstract key provider interface |
| `malware_scanner.h` | `IMalwareScanner`, `ScanResult` | Blob/file malware scanning interface |
| `manifest_signer.h` | `ManifestSigner` | Signs release manifests with CMS/PEM |
| `mock_key_provider.h` | `MockKeyProvider` | Test-only in-memory key provider |
| `output_encoding.h` | `OutputEncoder` | Safe HTML/JSON/URL output encoding |
| `pii_redaction_policy.h` | `IPiiRedactionPolicy`, `RedactionRule` | PII field redaction policy interface |
| `pkcs11_minimal.h` | `Pkcs11Token`, `Pkcs11Slot` | Minimal PKCS#11 interface types |
| `pkcs11_wrapper.h` | `Pkcs11Wrapper` | PKCS#11 library loader and function wrapper |
| `pki_key_provider.h` | `PkiKeyProvider` | PKI certificate-based key provider |
| `post_quantum_crypto.h` | `IPostQuantumSigner`, `IPqKem` | NIST PQC (Dilithium, Kyber) interface |
| `query_masking_policy.h` | `IQueryMaskingPolicy` | Query result field masking by classification |
| `rbac.h` | `IRbac`, `RbacRole`, `Permission` | Role-Based Access Control interface |
| `row_level_security.h` | `IRowLevelSecurity`, `RlsPolicy` | Row-level security policy evaluation |
| `secret_manager.h` | `ISecretManager` | Secret store interface (Vault, env, file) |
| `security_evidence_collector.h` | `SecurityEvidenceCollector` | Compliance evidence collection and export |
| `signing.h` | `ISigner`, `IVerifier`, `Signature` | Generic signing/verification interface |
| `signing_provider.h` | `ISigningProvider` | Signing key provider abstraction |
| `timestamp_authority.h` | `ITimestampAuthority` | RFC 3161 timestamp authority interface |
| `transport_security_checker.h` | `TransportSecurityChecker` | mTLS/TLS transport security validator |
| `tsa_api.h` | `TsaApiClient` | TSA HTTP API client |
| `usb_admin_authenticator.h` | `UsbAdminAuthenticator` | Physical USB token admin authentication |
| `user_registration_plugin.h` | `IUserRegistrationPlugin` | User provisioning plugin interface |
| `vault_key_provider.h` | `VaultKeyProvider` | HashiCorp Vault key provider |
| `vault_signing_provider.h` | `VaultSigningProvider` | Vault Transit signing provider |
| `vcc_pki_client.h` | `VccPkiClient` | VCC/internal PKI client |
| `vram_secure_clear.h` | `VramSecureClear` | GPU VRAM secure memory zeroing |
| `zero_trust_policy_enforcer.h` | `ZeroTrustPolicyEnforcer` | Zero-trust policy evaluation engine |
| *(planned)* `intent_classifier.h` | `IntentClassifier`, `IntentAlert` | Layer 7: semantic query-intent analysis; sets `session_risk_score` via ZeroTrustPolicyEnforcer (IMPL-B7) |

---

> Implementation details in `../../src/security/`. Header-only interfaces only; no business logic in `include/`.
> **Paper 2 addition (IMPL-B7):** `IntentClassifier` receives `AnomalyScore` from `MLAnomalyDetector` as a prior; alerts with confidence ≥ 0.85 update `ZeroTrustContext::session_risk_score`.
