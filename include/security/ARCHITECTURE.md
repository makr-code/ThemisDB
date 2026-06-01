> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/security/ARCHITECTURE.md -->

# Security Module — Public Header Architecture

**Module Path:** `include/security/`
**Implementation:** `../../src/security/`
**Canonical architecture doc:** [`../../src/security/ARCHITECTURE.md`](../../src/security/ARCHITECTURE.md)

---

## 1. Overview

`include/security/` is the public C++ contract for ThemisDB's security layer. With 51 headers it covers: access control (RBAC, ACL, row-level), encryption (symmetric/asymmetric), HSM/PKCS#11 integration, PKI and signing, behavioral anomaly detection, PII/prompt-injection protection, FIPS compliance, post-quantum cryptography, and zero-trust policy enforcement.

Full threat model, cryptographic design, and audit trail are in:
→ [`../../src/security/ARCHITECTURE.md`](../../src/security/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Access Control

| Header | Public Type | Purpose |
|--------|------------|---------|
| `access_control.h` | `AccessControl` | Core ACL evaluation |
| `access_control_manager.h` | `AccessControlManager` | ACL lifecycle and caching |
| `rbac.h` | `RBACPolicy` | Role-based access control |
| `row_level_security.h` | `RowLevelSecurityPolicy` | Row-level predicate injection |

### 2.2 Encryption and Key Management

| Header | Public Type | Purpose |
|--------|------------|---------|
| `encryption.h` | `EncryptionProvider` | Symmetric/asymmetric encryption façade |
| `key_provider.h` | `IKeyProvider` | Key provider interface |
| `vault_key_provider.h` | `VaultKeyProvider` | HashiCorp Vault key provider |
| `pki_key_provider.h` | `PKIKeyProvider` | PKI-backed key provider |
| `mock_key_provider.h` | `MockKeyProvider` | Test-only key provider |
| `secret_manager.h` | `SecretManager` | Secret lifecycle management |
| `post_quantum_crypto.h` | `PostQuantumCrypto` | PQC (Kyber/Dilithium) primitives |
| `vram_secure_clear.h` | `VRAMSecureClear` | Secure GPU memory erasure |
| `crypto_capabilities.h` | `CryptoCapabilities` | Runtime crypto feature detection |
| `fips_crypto_mode.h` | `FIPSCryptoMode` | FIPS 140-2/3 mode enforcement |

### 2.3 HSM and PKCS#11

| Header | Public Type | Purpose |
|--------|------------|---------|
| `hsm_key_provider_adapter.h` | `HSMKeyProviderAdapter` | IKeyProvider ↔ HSM bridge |
| `hsm_provider.h` | `IHSMProvider` | HSM provider interface |
| `hsm_security_checker.h` | `HSMSecurityChecker` | HSM health and policy check |
| `hsm_security_metrics.h` | `HSMSecurityMetrics` | HSM metrics emission |
| `hsm_startup_policy.h` | `HSMStartupPolicy` | HSM initialization policy |
| `pkcs11_minimal.h` | — | Minimal PKCS#11 type stubs |
| `pkcs11_wrapper.h` | `PKCS11Wrapper` | PKCS#11 C++ wrapper |

### 2.4 Signing and PKI

| Header | Public Type | Purpose |
|--------|------------|---------|
| `signing.h` | `SigningService` | Document/token signing |
| `signing_provider.h` | `ISigningProvider` | Signing provider interface |
| `vault_signing_provider.h` | `VaultSigningProvider` | Vault-backed signing |
| `manifest_signer.h` | `ManifestSigner` | Artifact manifest signing |
| `binary_manifest.h` | `BinaryManifest` | Binary manifest format |
| `cms_signing.h` | `CMSSigning` | CMS/PKCS#7 envelope signing |
| `tsa_api.h` | `TSAClient` | RFC 3161 timestamp authority client |
| `timestamp_authority.h` | `TimestampAuthority` | Local timestamp authority |
| `transport_security_checker.h` | `TransportSecurityChecker` | TLS/mTLS policy check |
| `vcc_pki_client.h` | `VCCPKIClient` | Vehicle connectivity PKI client |

### 2.5 Behavioral and AI Security

| Header | Public Type | Purpose |
|--------|------------|---------|
| `behavioral_anomaly_detector.h` | `BehavioralAnomalyDetector` | ML-based anomaly detection |
| `intent_classifier.h` | `IntentClassifier` | Query intent classification |
| `ai_operation_guard.h` | `AIOperationGuard` | AI operation safety fence |
| `ai_snapshot_cleanup.h` | `AISnapshotCleanup` | Secure AI snapshot expiry |
| `aql_injection_detector.h` | `AQLInjectionDetector` | AQL injection pattern detection |
| `prompt_injection_pattern_registry.h` | `PromptInjectionPatternRegistry` | Prompt injection patterns |
| `malware_scanner.h` | `MalwareScanner` | Uploaded content malware scan |

### 2.6 PII, Masking, and Output Safety

| Header | Public Type | Purpose |
|--------|------------|---------|
| `pii_redaction_policy.h` | `PIIRedactionPolicy` | PII field masking rules |
| `query_masking_policy.h` | `QueryMaskingPolicy` | Per-query output masking |
| `output_encoding.h` | `OutputEncoding` | HTML/JSON/SQL output encoding |
| `input_validator.hpp` | `InputValidator<T>` | Generic input validation template |

### 2.7 Zero Trust and Compliance

| Header | Public Type | Purpose |
|--------|------------|---------|
| `zero_trust_policy_enforcer.h` | `ZeroTrustPolicyEnforcer` | Zero-trust continuous verification |
| `confidential_computing.h` | `ConfidentialComputingConfig` | TEE/SGX/TDX config |
| `security_evidence_collector.h` | `SecurityEvidenceCollector` | Compliance evidence aggregation |

### 2.8 USB and Auxiliary

| Header | Public Type | Purpose |
|--------|------------|---------|
| `usb_admin_authenticator.h` | `USBAdminAuthenticator` | Hardware token admin auth |
| `usb_volume_hardening.h` | `USBVolumeHardening` | USB volume hardening policy |
| `user_registration_plugin.h` | `IUserRegistrationPlugin` | User registration extension point |
| `arrow_user_registration_plugin.h` | `ArrowUserRegistrationPlugin` | Arrow-format user registration |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis::security` | All security types |
| `themis::security::hsm` | HSM and PKCS#11 types |
| `themis::security::pki` | PKI, signing, and TSA types |
| `themis::security::ai` | AI/ML security types |
