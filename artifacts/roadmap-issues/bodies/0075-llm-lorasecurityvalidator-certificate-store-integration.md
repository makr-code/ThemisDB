### Context

This issue implements the roadmap item '`LoraSecurityValidator`: Certificate Store Integration' for the llm domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: `LoraSecurityValidator`: Certificate Store Integration

### Goal

Deliver the scoped changes for `LoraSecurityValidator`: Certificate Store Integration in src/llm/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `LoraSecurityValidator`: Certificate Store Integration
**Priority:** High
**Target Version:** v1.8.0

`lora_security_validator.cpp` has 2 critical TODOs for certificate retrieval (lines 249 and 365):
- Line 249: "TODO: Retrieve from certificate store by fingerprint" — `cert_pem` is left empty; signature verification is effectively **never performed** when no inline certificate is provided in the metadata.
- Line 365: "TODO: Implement certificate store integration for production deployments"

This means LoRA adapter signature validation silently succeeds without verifying the certificate chain.

**Implementation Notes:**
- `[ ]` Implement a `LoRACertificateStore` class backed by `src/security/secret_manager.cpp` or a filesystem path (`config/security/lora_certs/`).
- `[ ]` In `verifyLoRASignature()` at line 248, look up the certificate by fingerprint from `LoRACertificateStore`; fail closed if the certificate is not found (return `SIGNATURE_UNVERIFIABLE` error, not success).
- `[ ]` Wire integration with the system certificate store (`/etc/ssl/certs` on Linux; `HCERTSTORE` on Windows) as a fallback after the local `LoRACertificateStore`.
- `[ ]` Add unit tests: missing cert → verification fails; valid cert + valid sig → passes; valid cert + tampered sig → fails.

---

### Acceptance Criteria

- [ ] Line 249: "TODO: Retrieve from certificate store by fingerprint" — `cert_pem` is left empty; signature verification is effectively **never performed** when no inline certificate is provided in the metadata.
- [ ] Line 365: "TODO: Implement certificate store integration for production deployments"
- [ ] Implement a `LoRACertificateStore` class backed by `src/security/secret_manager.cpp` or a filesystem path (`config/security/lora_certs/`).
- [ ] In `verifyLoRASignature()` at line 248, look up the certificate by fingerprint from `LoRACertificateStore`; fail closed if the certificate is not found (return `SIGNATURE_UNVERIFIABLE` error, not success).
- [ ] Wire integration with the system certificate store (`/etc/ssl/certs` on Linux; `HCERTSTORE` on Windows) as a fallback after the local `LoRACertificateStore`.
- [ ] Add unit tests: missing cert → verification fails; valid cert + valid sig → passes; valid cert + tampered sig → fails.

### Relationships

- Roadmap row: #75 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/llm/FUTURE_ENHANCEMENTS.md#lorasecurityvalidator-certificate-store-integration
- Source key: roadmap:75:llm:v1.8.0:lorasecurityvalidator-certificate-store-integration

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:75:llm:v1.8.0:lorasecurityvalidator-certificate-store-integration -->
<!-- roadmap-ref: row=75;module=llm;target=v1.8.0 -->
<!-- roadmap-detail: src/llm/FUTURE_ENHANCEMENTS.md#lorasecurityvalidator-certificate-store-integration -->
