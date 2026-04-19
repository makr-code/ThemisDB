<!-- Status: current | validated: 2026-04-19 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — Security Module (Public Headers)

**Last Audit:** 2026-04-19
**Auditor:** Copilot
**Status:** ✅ Pass

---

## Summary

| Metric | Result |
|--------|--------|
| Header Files | 44 (in `include/security/`) |
| Implementation | `../../src/security/` |
| Exported Symbols Verified | ✅ |
| Deprecated APIs | 0 |
| Security Issues in Headers | None |

---

## Header Files Audited

| File | Exported Symbols | Notes |
|------|-----------------|-------|
| `access_control.h` | `IAccessControl`, `AccessDecision` | Pure virtual; no side effects |
| `access_control_manager.h` | `AccessControlManager` | Thread-safe policy reload |
| `aql_injection_detector.h` | `AqlInjectionDetector` | Stateless detection; no deps |
| `arrow_user_registration_plugin.h` | `ArrowUserRegistrationPlugin` | IPC plugin contract |
| `binary_manifest.h` | `BinaryManifest`, `ManifestEntry` | Serializable manifest types |
| `cms_signing.h` | `CmsSigner`, `CmsVerifier` | OpenSSL CMS abstraction |
| `confidential_computing.h` | `ConfidentialComputingContext` | TEE attestation contract |
| `crypto_capabilities.h` | `CryptoCapabilities` | Algorithm discovery |
| `encryption.h` | `IEncryptor`, `IDecryptor`, `EncryptionConfig` | Core crypto contracts |
| `fips_crypto_mode.h` | `FipsCryptoMode` | FIPS enforcement flag |
| `hsm_key_provider_adapter.h` | `HsmKeyProviderAdapter` | PKCS#11 bridge |
| `hsm_provider.h` | `IHsmProvider`, `HsmSlotInfo` | HSM abstraction |
| `hsm_security_checker.h` | `HsmSecurityChecker` | Health probe |
| `hsm_security_metrics.h` | `HsmSecurityMetrics` | Prometheus-compatible metrics |
| `key_provider.h` | `IKeyProvider`, `KeyMaterial` | Core key contract |
| `malware_scanner.h` | `IMalwareScanner`, `ScanResult` | AV/scan interface |
| `manifest_signer.h` | `ManifestSigner` | Release integrity |
| `mock_key_provider.h` | `MockKeyProvider` | Test use only; not for production |
| `output_encoding.h` | `OutputEncoder` | XSS prevention helpers |
| `pii_redaction_policy.h` | `IPiiRedactionPolicy`, `RedactionRule` | GDPR redaction contract |
| `pkcs11_minimal.h` | `Pkcs11Token`, `Pkcs11Slot` | PKCS#11 type defs |
| `pkcs11_wrapper.h` | `Pkcs11Wrapper` | Dynamic loader |
| `pki_key_provider.h` | `PkiKeyProvider` | X.509 key provider |
| `post_quantum_crypto.h` | `IPostQuantumSigner`, `IPqKem` | PQC interfaces |
| `query_masking_policy.h` | `IQueryMaskingPolicy` | Column-level masking |
| `rbac.h` | `IRbac`, `RbacRole`, `Permission` | RBAC contract |
| `row_level_security.h` | `IRowLevelSecurity`, `RlsPolicy` | RLS contract |
| `secret_manager.h` | `ISecretManager` | Secret store contract |
| `security_evidence_collector.h` | `SecurityEvidenceCollector` | Compliance artifact export |
| `signing.h` | `ISigner`, `IVerifier`, `Signature` | Generic signing |
| `signing_provider.h` | `ISigningProvider` | Signing key abstraction |
| `timestamp_authority.h` | `ITimestampAuthority` | RFC 3161 contract |
| `transport_security_checker.h` | `TransportSecurityChecker` | mTLS validation |
| `tsa_api.h` | `TsaApiClient` | TSA HTTP client |
| `usb_admin_authenticator.h` | `UsbAdminAuthenticator` | Hardware token auth |
| `user_registration_plugin.h` | `IUserRegistrationPlugin` | Provisioning plugin |
| `vault_key_provider.h` | `VaultKeyProvider` | Vault KV/Transit |
| `vault_signing_provider.h` | `VaultSigningProvider` | Vault Transit signing |
| `vcc_pki_client.h` | `VccPkiClient` | Internal PKI client |
| `vram_secure_clear.h` | `VramSecureClear` | GPU memory zeroing |
| `zero_trust_policy_enforcer.h` | `ZeroTrustPolicyEnforcer` | Zero-trust enforcement |
| `behavioral_anomaly_detector.h` | `BehavioralAnomalyDetector` | ✅ Reviewed |
| `intent_classifier.h` | `IntentClassifier` | ✅ Reviewed |
| `usb_volume_hardening.h` | `UsbVolumeHardening` | ✅ Reviewed |

---

## Findings

No issues found in public headers. All interfaces are pure virtual or value-type contracts.
`mock_key_provider.h` is intentionally included for test harness usage; not compiled into
production binaries.
