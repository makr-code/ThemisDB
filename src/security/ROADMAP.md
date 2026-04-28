> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Security Module Roadmap

## Current Status
v1.x – Enterprise-grade, defense-in-depth security infrastructure. Six distinct security layers (transport, authentication, authorization, data protection, audit/compliance, and threat detection) are production-ready.

## Completed ✅
- [x] Transport security: TLS 1.3 and mutual TLS (mTLS)
- [x] Authentication: USB admin key, PKI certificates, multi-factor auth
- [x] RBAC with role hierarchy and permission inheritance
- [x] Field-level AES-256-GCM encryption (document, array, and VRAM fields)
- [x] Key management hierarchy (Master Key → KEK → DEK) with HSM support
- [x] Key rotation (active, deprecated, rotating DEK states)
- [x] HashiCorp Vault integration for key storage
- [x] CMS/PKCS#7 signing and eIDAS-compliant timestamping
- [x] Malware scanner for plugin manifests
- [x] AQL injection detection
- [x] SecurityManager orchestrator
- [x] Audit log with tamper-evident chaining
- [x] Compliance features (eIDAS, GDPR-related controls)
- [x] Attribute-Based Access Control (ABAC) alongside RBAC
- [x] Zero-trust network policy enforcement (per-request identity verification)
- [x] Dynamic data masking for PII fields in query results (`QueryMaskingPolicy`, PR: #3050, v1.5.0)
- [x] Row-level security policies in AQL execution (`RLSManager`, `RLSPolicy`, `RLSPredicate`, `AccessControlManager` integration)
- [x] JWT / OIDC federated authentication (OAuth 2.0 provider integration)
- [x] Session token revocation list with real-time invalidation (`TokenBlacklist`)
- [x] Anomaly detection on authentication patterns: brute-force and credential stuffing (`AuthRateLimiter`)
- [x] Post-quantum cryptography migration path (CRYSTALS-Kyber / Dilithium) (`include/security/post_quantum_crypto.h`, `src/security/post_quantum_crypto.cpp`)
- [x] Systematic attack vector test suite (`tests/security/attack-vectors/crypto/`, `injection/`, `authentication/`)
- [x] USB admin authenticator HMAC-SHA256 challenge-response with replay protection; Windows MachineGuid fix
- [x] SOC 2 Type II security compliance evidence collection (`include/security/security_evidence_collector.h`)
- [x] Performance benchmarks for security hot-paths (`benchmarks/bench_security.cpp`)
  - AES-256-GCM encrypt/decrypt throughput (1 KB, 64 KB, 1 MB); FieldEncryption API
  - RBAC policy evaluation latency: single-role and 100-role checks, role-hierarchy validation
  - Post-quantum: Kyber-1024 key-gen/encapsulate/decapsulate, Dilithium-5 sign/verify
  - FIPS algorithm-list validation overhead (approved vs. rejected paths)
  - AQL injection detection throughput (benign and malicious queries)
  - Audit log tamper-evident append latency (single entry and batch-100)
- [x] Focused standalone test targets for all `tests/security/` test files
  - `SecurityNegativeIntegrationFocusedTests` (JWT/RBAC/Vault negative tests)
  - `InputValidationSecurityFocusedTests` (14 security validation tests: AQL injection, path traversal, XSS, command injection, XXE, LDAP, email, URL, buffer-overflow, integer-overflow, format-string, unicode normalization, CRLF)

## In Progress 🚧
- [~] FIPS 140-2 / 140-3 validated cryptography mode (Target: Q3 2026) (Issue: #2297)
  - `FipsCryptoMode` singleton, `FipsPolicyViolation` exception, approved-algorithm set implemented
  - Activation requires FIPS-validated OpenSSL 3.x build (not bundled); graceful degradation on unavailable provider
  - 20 tests in `tests/security/test_fips_crypto_mode.cpp`; `FipsCryptoModeFocusedTests` standalone target

## Planned Features 📋

### Short-term (Next 3-6 months)

### Long-term (6-12 months)
- [x] SOC 2 Type II compliance evidence collection (Issue: #2293) — **Shipped** in `src/security/security_evidence_collector.cpp`; 28 tests in `tests/security/test_security_evidence_collector.cpp`

## Implementation Phases

### Phase 1: Transport, Authentication & Data Protection (Status: Completed ✅)
- [x] Transport security: TLS 1.3 and mutual TLS (mTLS)
- [x] Authentication: USB admin key, PKI certificates, multi-factor auth
- [x] RBAC with role hierarchy and permission inheritance
- [x] Field-level AES-256-GCM encryption (document, array, and VRAM fields)
- [x] Key management hierarchy (Master Key → KEK → DEK) with HSM support
- [x] Key rotation (active, deprecated, rotating DEK states)
- [x] HashiCorp Vault integration for key storage
- [x] CMS/PKCS#7 signing and eIDAS-compliant timestamping
- [x] Malware scanner for plugin manifests
- [x] AQL injection detection
- [x] `SecurityManager` orchestrator
- [x] Audit log with tamper-evident chaining
- [x] Compliance features (eIDAS, GDPR-related controls)

### Phase 2: ABAC & HSM Direct Integration (Status: Completed ✅)
- [x] Attribute-Based Access Control (ABAC) alongside RBAC
- [x] Hardware Security Module (HSM) direct PKCS#11 integration
- [x] PKCS#11 C++ wrapper interface (`include/security/pkcs11_wrapper.h`, Issue: #3252)
  - RAII `Pkcs11Library` (load/unload dynamic library)
  - RAII `Pkcs11Session` (open/close/login/logout)
  - `Pkcs11Category` + `makePkcs11Error()` for `std::error_code` integration
  - Free helpers: `listSlots`, `findObjects`, `findObjectsByLabel`, `signData`,
    `verifyData`, `encryptData`, `decryptData`, `generateRsaKeyPair`,
    `getAttribute`, `getAttributeBytes`
  - Tests: `tests/test_pkcs11_wrapper.cpp` (pure-unit + optional SoftHSM2 integration)
- [~] FIPS 140-2 / 140-3 validated cryptography mode (see Phase 4: Zero-Trust & Post-Quantum Cryptography)

### Phase 3: Federated Auth & Anomaly Detection (Status: Completed ✅)
- [x] JWT / OIDC federated authentication (OAuth 2.0 provider integration)
- [x] Session token revocation list with real-time invalidation (`TokenBlacklist`)
- [x] Anomaly detection on authentication patterns (brute-force, credential stuffing) (`AuthRateLimiter`)
- [x] Row-level security policies in AQL execution (`RLSManager`, `AccessControlManager` integration)

### Phase 4: Zero-Trust & Post-Quantum Cryptography (Status: In Progress 🚧)
- [x] Zero-trust network policy enforcement (per-request identity verification)
- [x] Confidential computing support (Intel TDX / AMD SEV encrypted enclaves)
- [x] Dynamic data masking for PII fields in query results (`QueryMaskingPolicy`, PR: #3050, v1.5.0)
- [x] Secret scanning pre-commit hook for CI pipelines (`scripts/secret_scan.py`, `.pre-commit-config.yaml`, `.github/workflows/secret-scanning-ci.yml`)
- [x] Post-quantum cryptography migration path (CRYSTALS-Kyber, Dilithium) — `include/security/post_quantum_crypto.h`, `src/security/post_quantum_crypto.cpp`
  - KyberKEM: key generation, encapsulate/decapsulate round-trip, all three security levels (512/768/1024)
  - DilithiumSigner: sign/verify round-trip, all three security levels (2/3/5)
  - PostQuantumKeyProvider: Kyber-wrapped DEK wrapKeyWithKyber / unwrapKeyWithKyber
  - HybridEncryption: HYBRID / CLASSICAL_ONLY / POST_QUANTUM_ONLY modes; AES-256-GCM + Kyber-1024
  - Tests (production-ready): `tests/test_post_quantum_crypto.cpp` — 40 test cases; throughput ≥ 2 000 ops/s
- [x] SPHINCS+ added as alternative signing option alongside Dilithium (`include/security/sphincs_plus.h`)
  - SPHINCS+-SHA2-256s (small, conservative) and SPHINCS+-SHA2-256f (fast) parameter sets
  - Hash-based, stateless, no secret-state management required
  - Suitable for long-lived signatures and audit log signing (quantum-resistant beyond 2040)
  - Tests: `tests/security/test_sphincs_plus.cpp`
- [x] Post-Quantum Migration Plan (Phase 7.2) — Three-phase migration timeline:

  #### Phase A — v1.x (current): Hybrid Mode
  - Classical + PQ algorithms co-exist; PQ is opt-in via configuration.
  - Default: AES-256-GCM + X25519 (TLS) + RSA-4096 / ECDSA-P384 (signatures).
  - PQ opt-in: Kyber-1024 (DEK wrapping), Dilithium-5 or SPHINCS+ (signing), X25519-Kyber768 (TLS).

  #### Phase B — v2.0: PQ-Default + Classical Fallback
  - PQ algorithms become default; classical remains available via `THEMIS_CLASSICAL_FALLBACK=1`.
  - Default: Kyber-1024 (DEK), Dilithium-5 (signatures), X25519-Kyber768 (TLS).
  - Classical fallback configurable per key type.

  #### Phase C — v3.0: PQ-Only Mode
  - `THEMIS_PQ_ONLY_MODE=1` disables all classical algorithms.
  - Classical algorithms removed from default build; only available via compatibility shim.
  - Audit log signing: Dilithium-5 mandatory; SPHINCS+ as alternative.

  #### Per-Key-Type Migration Table

  | Key Type           | Phase A (v1.x)                  | Phase B (v2.0)               | Phase C (v3.0)           |
  |--------------------|---------------------------------|------------------------------|--------------------------|
  | DEK wrapping       | AES-256-GCM (classical)         | Kyber-1024 (default)         | Kyber-1024 only          |
  | Digital signatures | RSA-4096 / ECDSA-P384           | Dilithium-5 (default)        | Dilithium-5 or SPHINCS+  |
  | SPHINCS+ option    | SPHINCS+-SHA2-256s/256f opt-in  | SPHINCS+-SHA2-256s/256f opt-in | SPHINCS+ or Dilithium-5 |
  | TLS handshake      | X25519 (classical)              | X25519-Kyber768 hybrid group | X25519-Kyber768 only     |
  | Audit log signing  | HMAC-SHA256                     | Dilithium-5 (default)        | Dilithium-5 mandatory    |

  #### THEMIS_PQ_ONLY_MODE=1 Environment Variable
  - When set, the runtime refuses to load or generate classical-algorithm keys.
  - Throws `PQOnlyModeViolation` exception if a classical algorithm is requested.
  - CI gate: all PQ-only tests must pass before a v3.0 release tag is created.
  - Set in production deployments requiring NIST PQC Standard compliance (FIPS 203/204/205).

- [x] Systematic attack vector test suite (`tests/security/attack-vectors/`)
  - `crypto/test_crypto_attack_vectors.cpp` — IV/nonce reuse, tag tampering, bit-flip, key confusion, PQ key confusion, signature forgery
  - `injection/test_injection_attack_vectors.cpp` — AQL injection: comment markers, dangerous ops, boolean-blind, union, stacked queries, case bypass, oversized params; read-only context DDL/write rejection; unbounded FOR loop detection
  - `authentication/test_authentication_attack_vectors.cpp` — RBAC: privilege escalation, permission boundary, lateral movement, deleted/unknown roles, role injection, multi-role combinations
- [x] USB admin authenticator: HMAC-SHA256 challenge-response with replay protection (`src/security/usb_admin_authenticator.cpp`)
  - `createChallenge()` now uses OpenSSL CSPRNG and registers challenges with timestamps
  - `validateChallengeResponse()` verifies HMAC-SHA256(license_key, challenge), enforces TTL and one-time-use
  - Windows `MachineGuid` read from registry (`HKLM\SOFTWARE\Microsoft\Cryptography\MachineGuid`)
  - Tests: 18 tests in `tests/test_usb_admin_authenticator.cpp` (valid HMAC, wrong response, replay, unknown, expired, empty, multi-challenge)
- [x] USB Volume Hardening — defence against FAT filesystem manipulation (`include/security/usb_volume_hardening.h`, `src/security/usb_volume_hardening.cpp`)
  - **Volume integrity hash**: SHA-256 of license file content pinned at provisioning time; any FAT-level file replacement or byte-level edit is detected before the license is parsed
  - **Read-only mount enforcement**: verifies `/proc/mounts` (Linux) or `FILE_READ_ONLY_VOLUME` (Windows); prevents live writes to the stick during authentication
  - **USB device serial binding**: reads SCSI VPD serial via sysfs (`/sys/class/block/<dev>/device/../../serial`) on Linux and volume serial on Windows; prevents `dd`-cloned sticks from being accepted
  - Constant-time comparison for all hash/serial comparisons (OpenSSL `CRYPTO_memcmp`)
  - 3 new `Metrics` counters: `usb_denied_not_readonly`, `usb_denied_volume_hash_mismatch`, `usb_denied_serial_mismatch`
  - 3 new `USBAdminConfig` fields: `require_readonly_mount`, `expected_volume_hash`, `expected_usb_serial`
  - All hardening checks produce audit-log entries with descriptive event names
  - Tests: 22 tests in `tests/test_usb_volume_hardening.cpp` + `USBVolumeHardeningFocusedTests` standalone target
- [x] SOC 2 Type II compliance evidence collection (`include/security/security_evidence_collector.h`, `src/security/security_evidence_collector.cpp`)
  - Audit log export via `AuditLogger::generateComplianceReport()` + `searchEntries()`
  - Key-rotation records from `KeyProvider::listKeys()` with version-based rotation detection
  - Access-control report from `RBAC::listRoles()` / `getRole()` — empty roles, admin wildcard detection
  - Security metrics snapshot: active keys, deprecated keys, role count, audit log entry count
  - Append-only JSON export with atomic file write; 12-month retention enforcement; retention verification
  - NetworkControlsEvidence: TLS 1.3 cipher suites, mTLS shard count, rate-limiter config snapshot
  - ChangeManagementEvidence: config audit trail, key rotation log, time window
  - Tests: 34 test cases in `tests/security/test_security_evidence_collector.cpp` (+6 extended)
- [x] Performance benchmarks for security hot-paths (`benchmarks/bench_security.cpp`)
  - AES-256-GCM encrypt/decrypt (1 KB / 64 KB / 1 MB); target ≥ 1 GB/s (AES-NI, single core)
  - RBAC single-role check and 100-role check; role hierarchy validation
  - Kyber-1024 key-gen / encapsulate / decapsulate; target ≥ 2 000 ops/s
  - Dilithium-5 key-gen / sign / verify; target ≥ 1 000 ops/s
  - FIPS algorithm-list validation overhead (approved + rejected paths)
  - AQL injection detection throughput (benign and malicious)
  - Audit log tamper-evident append; target p99 ≤ 2 ms
- [x] Focused standalone test targets for all `tests/security/` test files
  - `SecurityNegativeIntegrationFocusedTests` — JWT/RBAC/Vault negative integration tests
  - `InputValidationSecurityFocusedTests` — 14 input validation security tests (AQL injection, path traversal, XSS, command injection, NoSQL/XXE/LDAP/email/URL injection, buffer overflow, integer overflow, format-string, Unicode normalization, CRLF); backed by `utils::InputValidator` security API (`include/utils/input_validator.h`)
- [~] FIPS 140-2 / 140-3 validated cryptography mode (`src/security/fips_crypto_mode.cpp`) (Issue: #2297)
  - `FipsCryptoMode` singleton + `FipsPolicyViolation` exception fully implemented
  - Approved-algorithm set: all NIST SP 800-175B rev.1 ciphers, hashes, MACs, KDFs, DRBGs
  - Graceful degradation if FIPS provider is not installed (returns false, no abort)
  - 20 tests in `tests/security/test_fips_crypto_mode.cpp` (`FipsCryptoModeFocusedTests`)
  - Pending: FIPS-validated OpenSSL 3.x build + full CI gate (not bundled)

## Production Readiness Checklist
- [x] Unit tests coverage > 80%:
  - KyberKEM/DilithiumSigner/HybridEncryption: 40 tests (`tests/test_post_quantum_crypto.cpp`)
  - SecurityEvidenceCollector: 28 tests (`tests/security/test_security_evidence_collector.cpp`)
  - FipsCryptoMode: 20 tests (`tests/security/test_fips_crypto_mode.cpp`)
  - USBVolumeHardening: 22 tests (`tests/test_usb_volume_hardening.cpp`)
  - QueryMaskingPolicy, RLSManager, ZeroTrustPolicyEnforcer, AuthRateLimiter, HsmProvider: covered
  - InputValidationSecurity: 14 tests (`tests/security/test_input_validation_security.cpp`) — AQL, path, XSS, command, NoSQL, XXE, LDAP, email, URL, buffer-overflow, int-range, format-string, unicode, CRLF
  - Standalone focused test targets for all security sub-directory tests:
    `SecurityEvidenceCollectorFocusedTests`, `FipsCryptoModeFocusedTests`,
    `AccessControlManagerFocusedTests`, `RowLevelSecurityFocusedTests`,
    `SecurityNegativeIntegrationFocusedTests`, `InputValidationSecurityFocusedTests`,
    `USBVolumeHardeningFocusedTests`,
    `CryptoAttackVectorTests`, `InjectionAttackVectorTests`, `AuthenticationAttackVectorTests`
- [x] Integration tests (TLS handshake, key rotation, RBAC enforcement, RLS filtering, JWT revocation)
- [x] Attack vector tests (crypto IV/tag/key confusion, injection, authentication privilege escalation)
- [x] Challenge-response security: HMAC-SHA256 with replay protection, TTL, one-time-use
- [x] SOC 2 evidence collection: audit log, key rotations, metrics, RBAC report, retention enforcement
- [x] Performance benchmarks (AES-256-GCM, RBAC, Kyber-1024/Dilithium-5, FIPS, AQL injection, audit-log) — `benchmarks/bench_security.cpp`
- [x] Security audit (penetration testing, CVE dependency scan) (Target: Q2 2026) — Meta sourcecode security audit documented in `src/security/AUDIT.md` (2026-04-20)
- [x] Documentation complete (ROADMAP.md, FUTURE_ENHANCEMENTS.md, inline docblocks)
- [x] API stability guaranteed (SecurityManager v1.x, RLSManager v1.5.0, QueryMaskingPolicy v1.5.0)

## Known Issues & Limitations
- HSM integration uses RSA-OAEP (SHA-256 / MGF1-SHA-256) for DEK wrapping via PKCS#11 C_Encrypt/C_Decrypt.
- FIPS 140-2 mode requires a FIPS-validated OpenSSL build; not bundled by default.
- AQL injection detection is AST/parser-driven for query-shape enforcement: `validateForReadOnlyContext()` delegates to `validateAQLAST()` and rejects non-parseable/non-AQL write payloads via parser errors; `validateUnboundedForLoops()` rejects unbounded FOR loops without LIMIT clause.
- Zero-trust `ZeroTrustPolicyEnforcer` supports IPv4 CIDR policies only; IPv6 support planned for a follow-up.
- USB admin challenge-response uses HMAC-SHA256 with the license key as the HMAC secret; consider migrating to Ed25519 signatures with a dedicated per-USB key pair in a future iteration.
- USB Volume Hardening: `getUSBDeviceSerial()` on Linux reads the serial from sysfs; if the USB stick does not expose a serial via the SCSI VPD page 0x80 string descriptor (some cheap sticks do not), `expected_usb_serial` verification is unavailable.  In that case `verifyUSBSerial()` returns false and the stick will be rejected; set `expected_usb_serial` only for sticks known to expose a stable serial.
- USB Volume Hardening: `isMountedReadOnly()` verifies the _current_ mount flags at authentication time; an attacker with root access could remount the volume as read-write after the check passes.  Use `require_readonly_mount` as a defence-in-depth measure alongside OS-level policies.
- **[A-1] `access_control.cpp::authenticate()`**: Guaranteed deadlock — `mutex_` acquired at entry, then `getUserRoles()` and `createSession()` both re-acquire the same non-recursive `std::mutex` → no login can complete. Fix: internal unlocked variants.
- **[A-2] `access_control.cpp::changePassword()`**: Guaranteed deadlock — `mutex_` acquired, then `invalidateUserSessions()` re-acquires it.
- **[A-3] `access_control.cpp::enrollMFA()`**: No authorization check before overwriting existing MFA enrollment → MFA bypass path.
- **[E-1] `field_encryption.cpp`**: 8 raw encryption key bytes written to disk via `write_debug_dump()` when `THEMIS_DEBUG_ENC_DIR` is set; unconditional `fprintf(stderr)` in `decryptInternal()` leaks metadata.
- **[E-4] `field_encryption.cpp::createDefault()`**: Uses `MockKeyProvider` — any caller of `createDefault()` silently uses non-production key material.

## Phase 5: AI Safety Layer (Status: Planned 📋)

> **Hintergrund:** Cursor-KI-Vorfall (April 2026) — KI-Agent löschte Produktionsdatenbank
> ohne Approval, Snapshot oder Umgebungsunterscheidung.
> Vollständige Analyse: `docs/de/security/ai_safety/AI_SAFETY_ARCHITECTURE.md`
> Entwickler-Referenz: `src/security/AI_SAFETY_ARCHITECTURE.md`

### Phase 1 — Kritische Fixes (Target: Q2 2026)
- [x] **ASL-1:** `DATA_DESTRUCTION` + `SCHEMA_MUTATION` IntentType-Werte hinzufügen (Target: Q2 2026)
  - `src/security/intent_classifier.h` + `intent_classifier.cpp`
  - Neue Feature-Tabellen: `kDataDestructionFeatures[]`, `kSchemaMutationFeatures[]`
  - Spezialregel: `FOR...REMOVE` ohne `FILTER` → Confidence 0.90
  - Blockierungsschwellenwert: Confidence ≥ 0.65
  - `riskDelta()`: DATA_DESTRUCTION=0.90, SCHEMA_MUTATION=0.75
  - Tests: IC-09..IC-15 in `tests/test_intent_classifier.cpp`
- [x] **ASL-2:** `AqlSafetyValidator` implementieren (Target: Q2 2026)
  - `include/query/aql_safety_validator.h` + `src/query/aql_safety_validator.cpp` [NEU]
  - Erkennung: REMOVE, INSERT, UPDATE, REPLACE, UPSERT, DROP, TRUNCATE, CREATE COLLECTION
  - Integration: `McpServer::toolQuery()` wenn `enforce_read_only: true` im Mode aktiv
  - Latenz-Ziel: p99 < 0.1ms bei 1 KB Queries
  - Tests: ASV-01..ASV-16 in `tests/security/test_aql_safety_validator.cpp`
- [x] **ASL-3:** Dry-Run-Flag in `toolDeleteEntity()` + `toolDropIndex()` (Target: Q2 2026)
  - `src/server/mcp_server.cpp`: args.value("dry_run", false) → Preview statt Execution

### Phase 2 — DOG + HILG (Target: Q3 2026)
- [x] **ASL-4:** `AiOperationGuard` Klassifikations-Engine (Target: Q3 2026)
  - `include/security/ai_operation_guard.h` + `src/security/ai_operation_guard.cpp` [NEU]
  - OperationClass: READ_ONLY, WRITE_SAFE, DESTRUCTIVE, CRITICAL
  - `evaluate()` → GuardDecision{op_class, preview, requires_approval, operation_id}
- [x] **ASL-5:** Approval-Queue in `McpServer` (Target: Q3 2026)
  - `pending_approvals_` map + mutex; TTL-basierter Expiry
  - Operation wird gecached bis Approval oder Expiry
- [x] **ASL-6:** HTTP-Approval-Endpoints (Target: Q3 2026)
  - `POST /v1/ai/approve/{operation_id}`
  - `POST /v1/ai/deny/{operation_id}`
  - `GET  /v1/ai/pending-approvals`
  - `HttpServer::setMcpServer()` + route-enum Einträge + Handler in `routeRequest()`
  - Registriert in `include/server/http_server.h` + `src/server/http_server.cpp`
- [~] **ASL-7:** `safety:`-Sektion aus Mode-YAML auswerten (Target: Q3 2026)
  - `config/ai_ml/llm/modes/default.yaml`: `safety:` Block für `agentic` Mode ✅
  - `McpServer`: Guard-Konfiguration wird im Konstruktor geladen (Defaults); YAML-Override ausstehend

### Phase 3 — POS + Environment (Target: Q3 2026)
- [x] **ASL-8:** Pre-Operation-Snapshot-Hook (Target: Q3 2026)
  - `storage_->createCheckpoint(snap_dir)` vor jeder approved DESTRUCTIVE/CRITICAL Op
  - MCP-Response enthält `"pre_operation_snapshot"` Pfad
- [x] **ASL-9:** Environment-Konfiguration auswerten (Target: Q3 2026)
  - `config/security.yaml`: `environment:` + `ai_agent_restrictions:` Block
  - `McpServer`: production → hard-block CRITICAL ohne Rolle
- [x] **ASL-10:** `POST /v1/ai/rollback/{snapshot_id}` (Target: Q3 2026)
  - `storage_->restoreFromCheckpoint(path)` nach Operator-Bestätigung
- [x] **ASL-11:** Snapshot-Cleanup-Job (Retention-Policy) (Target: Q3 2026)
  - Retention: `snapshot_retention_days`, `snapshot_max_total_gb`

### Phase 4 — Audit + LoRA-Classifier (Target: Q4 2026)
- [x] **ASL-12:** AI-Session-Audit-Trail in `AuditLogger` (Target: Q4 2026)
  - Neue Events: AI_TOOL_CALL, AI_APPROVAL_REQUIRED, AI_OPERATION_EXECUTED, AI_SNAPSHOT_CREATED, ...
  - `ai_session_id` über alle MCP-Tool-Calls einer Session konsistent
- [x] **ASL-13:** LoRA-Adapter für IntentClassifier — IMPL-A2 (Target: Q4 2026)
  - Präzision: ≥ 92% (vs. ~80% regelbasiert)
  - Trainingsdaten: Annotierte AQL-Queries
- [x] **ASL-14:** Chaos-Tests für AI Safety Layer (Target: Q4 2026)
  - `tests/security/ai_safety/` [NEU]
  - Simulierter destruktiver KI-Agent gegen alle Guards
- [x] **ASL-15:** Grafana-Dashboard AI Safety Metriken (Target: Q4 2026)
  - `config/grafana/dashboards/ai-safety-monitoring.json`

## Breaking Changes
- SecurityManager API is stable from v1.x.
- Key management API is stable in v1.5.0; additional rotation hooks planned for v1.6.0+.
- DEK versioning scheme is fixed; no breaking changes planned.

## Latente Symbole (Unused-Functions-Audit)

_Stand: 2026-04-20 – Quelle: [`src/UNUSED_FUNCTIONS_REPORT.md`](../UNUSED_FUNCTIONS_REPORT.md)_

### ✅ Aktiv (implementiert + externer Aufrufer bestätigt)

- `AccessControl` – ABAC-Zugriffssteuerung; genutzt in compliance_reporter.cpp + Tests + Bench

### ✅ Aktiv — Security-Fixes 2026-04-20

- `verifyMFA` – Prüft MFA-Token für privilegierte Operationen.
  **Implementierungsstatus (ab 2026-04-20):** Vollständig implementiert. Schlägt den User in
  `AccessControl::mfa_enrollments_` nach (map: `user_id → MFAAuthenticator::EnrollmentData`),
  ruft `mfa_authenticator_->validateTOTP(secret_base32, token)` auf und fällt bei Misserfolg auf
  `validateRecoveryCode()` zurück (markiert den Code als verbraucht). Fehlendes oder deaktiviertes
  Enrollment → `false`, kein Silent-Pass-through mehr.
  `enrollMFA()` persistiert das Enrollment (`enabled = true`) in `mfa_enrollments_`.

- `disableMFA` – Deaktiviert MFA für einen User (Admin-Only-Pfad).
  **Implementierungsstatus (ab 2026-04-20):** Vollständig implementiert. Ruft
  `mfa_enrollments_.erase(user_id)` auf — nach diesem Aufruf schlägt `verifyMFA()` für den User
  garantiert fehl. Audit-Log-Event wird weiterhin geschrieben.
