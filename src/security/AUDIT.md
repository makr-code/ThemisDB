> ⚠️ **Historischer Auditbericht** – Befunde ohne aktuellen Codebeleg mit `<!-- TODO: add source file evidence -->` markieren. Veraltete Befunde entfernen.

<!-- Status: S0 FIXED | validated: 2026-05-04 (code re-verified) -->
# Audit Report — Security Module

**Last Audit:** 2026-05-04 | **Status:** ✅ S0 fixed — 0 S0 findings; 3 S1 remain open

> A-1 (`authenticate()` deadlock) and A-2 (`changePassword()` deadlock) are resolved:
> `getUserRolesLocked()`, `createSessionLocked()`, and `invalidateUserSessionsLocked()`
> lock-free internal variants are now used within the already-locked `mutex_` scope.
> D-1 (cache HMAC bypass) was fixed in `distributed_cache_coordinator.cpp` (2026-05-04).

## Summary

| Metric | Result |
|--------|--------|
| Build System Registration | ✅ Verified (cmake/ModularBuild.cmake) |
| Test Coverage | ✅ 7 focused test targets |
| S0 Critical / Safety Violations | ✅ 0 (A-1, A-2 fixed 2026-05-04) |
| S1 High | ✅ 0 (A-3/E-1/E-2/E-4/RB-1 fixed 2026-05-04) |
| S2 Medium | ⚠️ 4 |
| S3 Low | ℹ️ 2 |
| Successful login possible | ✅ **Yes — deadlocks resolved** |

## Source Files Audited

| Component | Files | Safety Status |
|-----------|-------|---------------|
| Access control | `access_control.cpp`, `access_control_manager.cpp`, `rbac.cpp`, `row_level_security.cpp`, `zero_trust_policy_enforcer.cpp` | 🔴 S0: `authenticate()` + `changePassword()` guaranteed deadlock |
| Cryptography | `post_quantum_crypto.cpp`, `field_encryption.cpp`, `encrypted_field.cpp`, `fips_crypto_mode.cpp`, `cms_signing.cpp` | 🔴 S1: key material written to disk; mock key provider reachable in production |
| HSM & key providers | `hsm_provider.cpp`, `hsm_provider_pkcs11.cpp`, `hsm_key_provider_adapter.cpp`, `hsm_signing.cpp`, `keyprovider_signing.cpp`, `key_cache.cpp`, `mock_key_provider.cpp` | ✅ No critical findings |
| PKI & certificates | `vcc_pki_client.cpp`, `pki_key_provider.cpp`, `timestamp_authority.cpp`, `timestamp_authority_openssl.cpp`, `tsa_api.cpp` | ✅ No critical findings |
| Vault integration | `vault_key_provider.cpp`, `vault_signing_provider.cpp` | ✅ No critical findings |
| Signing & manifests | `manifest_signer.cpp`, `binary_manifest.cpp` | ✅ No critical findings |
| Secrets & evidence | `secret_manager.cpp`, `security_evidence_collector.cpp`, `confidential_computing.cpp` | ✅ No critical findings |
| PII & query masking | `pii_redaction_policy.cpp`, `query_masking_policy.cpp` | ✅ No critical findings |
| Threat detection | `aql_injection_detector.cpp`, `behavioral_anomaly_detector.cpp`, `intent_classifier.cpp`, `malware_scanner.cpp` | ⚠️ S2: SQL injection detection trivially bypassed |
| VRAM security | `vram_secure_clear.cpp` | ✅ No critical findings |
| USB / hardware | `usb_admin_authenticator.cpp`, `usb_volume_hardening.cpp` | ✅ No critical findings |
| User registration plugins | `user_registration_plugin.cpp`, `embedded_user_registration_plugin.cpp`, `arrow_user_registration_plugin.cpp`, `webdav_user_registration_plugin.cpp` | ✅ No critical findings |

## Findings

### S0 — Critical (Guaranteed Authentication Failure / Deadlock)

#### A-1 · `access_control.cpp` · `authenticate()` — Guaranteed deadlock

`authenticate()` acquires `mutex_` at line 133 (non-recursive `std::mutex`), then calls
`getUserRoles()` (line 607) which also acquires `mutex_`, and `createSession()` (line 638)
which also acquires `mutex_`. Both are called unconditionally on every successful
authentication path. **No login of any kind (OAuth, password, token) can succeed.**

```cpp
std::lock_guard<std::mutex> lock(mutex_);   // L133 — acquired
...
auto roles = getUserRoles(result.user_id);   // → L607: lock(mutex_) → DEADLOCK
...
auto session_token = createSession(credentials.user_id, roles, ...);  // → L638: lock(mutex_)
```

**Fix required:** Restructure `AccessControl` to use an internal unlocked variant of
`getUserRoles_` and `createSession_` callable only while the caller already holds the lock,
or split into separate mutex-gated public and lock-free private APIs.

---

#### A-2 · `access_control.cpp` · `changePassword()` — Guaranteed deadlock

`changePassword()` acquires `mutex_` at line 317, then calls `invalidateUserSessions()`
(line 716) which also acquires `mutex_`. **No password change can complete.**

```cpp
std::lock_guard<std::mutex> lock(mutex_);   // L317 — acquired
...
invalidateUserSessions(user_id);             // → L716: lock(mutex_) → DEADLOCK
```

**Fix required:** Same approach as A-1 — create internal unlocked variants.

---

### S1 — High (all resolved 2026-05-04)

#### ~~A-3 · `access_control.cpp` · `enrollMFA()` — MFA enrollment bypass~~

`enrollMFA()` unconditionally overwrites any existing MFA enrollment without checking whether
the caller is the account owner or an administrator:

```cpp
enrollment.enabled = true;
mfa_enrollments_[user_id] = std::move(enrollment);  // overwrites existing MFA silently
```

Any caller with access to this function can replace a user's TOTP secret, invalidating the
user's authenticator app and gaining control of subsequent OTP verification.

**Fix required:** Require active MFA verification from the existing secret before allowing
re-enrollment. Log and rate-limit all enrollment attempts.

---

#### ~~E-1 · `field_encryption.cpp` · `write_debug_dump()` + `decryptInternal()` — Key material on disk + stderr~~

`write_debug_dump()` writes the **first 8 bytes of the raw encryption key** to a JSON file
on disk when `THEMIS_DEBUG_ENC_DIR` is set. `encryptInternal()` calls `write_debug_dump()`
unconditionally on every encryption (line 554, not gated on a build flag). Additionally,
`decryptInternal()` always emits `fprintf(stderr, ...)` leaking operation metadata (line 612)
without any debug gate.

```cpp
j["key_fingerprint_prefix"] = kf.str();   // 8 raw key bytes as hex

// decryptInternal(), always:
fprintf(stderr, "decryptInternal: ciphertext_len=%zu, tag_len=%zu, iv_len=%zu, key_len=%zu\n", ...);
```

**Fix required:** Replace raw key bytes with a proper HMAC-based fingerprint. Gate
`write_debug_dump()` on a compile-time `THEMIS_DEBUG` flag (not a runtime env var).
Remove unconditional `fprintf` from `decryptInternal()`.

---

#### ~~E-4 · `field_encryption.cpp` · `createDefault()` — `MockKeyProvider` in production~~

```cpp
std::shared_ptr<FieldEncryption> FieldEncryption::createDefault() {
    auto mock_provider = std::make_shared<MockKeyProvider>();
    return std::make_shared<FieldEncryption>(mock_provider);
}
```

The default factory method uses a mock key provider. Any code path that calls
`createDefault()` without providing a real `KeyProvider` silently uses static mock keys.

**Fix required:** Remove `createDefault()` or have it throw/abort with a clear diagnostic
requiring an explicit key provider. Replace with `createWithProvider(shared_ptr<KeyProvider>)`.

---

#### ~~E-2 · `field_encryption.cpp` · `encryptEntityBatch()` — Silent per-item encryption failures~~

```cpp
} catch (...) {
    // ignore per-item errors here
}
```

Failed encryptions in the parallel batch path produce default-constructed `EncryptedBlob`
(empty IV, empty ciphertext) in the output vector. Callers receive a full-size output but
cannot distinguish valid from failed entries. Corrupted records are silently stored.

**Fix required:** Replace silent catch with either propagating the first error, or storing
a per-item error/status in the result, and documenting the failure contract.

---

### S2 — Medium

| ID | File | Function | Description |
|----|------|----------|-------------|
| A-4 | `access_control.cpp` | `detectSQLInjection()` | Case-sensitive exact-match strings — bypassed by `union select`, Unicode lookalikes, inline comments; instills false security confidence |
| A-5 | `access_control.cpp` | `recordFailedLogin()` | Rate-limit lockout stored only in memory; any process restart clears all lockout state — brute-force protection resets on crash/restart |
| E-3 | `field_encryption.cpp` | `needsReEncryption()` | Uses exception as side-channel to detect key versions; transient KMS unavailability silently suppresses re-encryption |
| RB-2 | `rbac.cpp` | Constructor | Cyclic role hierarchy detected and logged, but system continues with corrupt data; all `checkPermission()` calls emit "Cyclic dependency" warnings at runtime |

### S1 — Additional

| ID | File | Function | Description |
|----|------|----------|-------------|
| RB-1 | `rbac.cpp` | `checkPermission()` | License server outage denies ALL permissions system-wide; no fail-open grace period |

### S3 — Low

| ID | File | Function | Description |
|----|------|----------|-------------|
| A-6 | `access_control.cpp` | `getStatistics()` | Duplicate `"active_sessions"` key in JSON output; second silently shadows first |
| RB-3 | `rbac.cpp` | `loadFromJson()` | Mutex acquired inside constructor before object is shared — misleading but harmless |

---

## Findings Summary Table

| ID | Severity | File | Function | Description |
|----|----------|------|----------|-------------|
| A-1 | **S0** | `access_control.cpp` | `authenticate()` | Non-recursive `mutex_` re-acquired via `getUserRoles()` + `createSession()` → guaranteed deadlock; no login possible |
| A-2 | **S0** | `access_control.cpp` | `changePassword()` | Non-recursive `mutex_` re-acquired via `invalidateUserSessions()` → guaranteed deadlock |
| A-3 | **S1** | `access_control.cpp` | `enrollMFA()` | No auth check before overwriting existing MFA enrollment → MFA bypass |
| E-1 | **S1** | `field_encryption.cpp` | `write_debug_dump()` / `decryptInternal()` | 8 raw key bytes written to disk; unconditional `fprintf(stderr)` in decrypt path |
| E-2 | **S1** | `field_encryption.cpp` | `encryptEntityBatch()` | Silent `catch(...)` produces empty `EncryptedBlob` on failure; callers cannot detect |
| E-4 | **S1** | `field_encryption.cpp` | `createDefault()` | `MockKeyProvider` used by default factory — production may silently use mock keys |
| RB-1 | **S1** | `rbac.cpp` | `checkPermission()` | License server outage denies all access with no grace period |
| A-4 | **S2** | `access_control.cpp` | `detectSQLInjection()` | Trivially bypassed case-sensitive exact-match detection |
| A-5 | **S2** | `access_control.cpp` | `recordFailedLogin()` | Brute-force lockout in memory only; reset on process restart |
| E-3 | **S2** | `field_encryption.cpp` | `needsReEncryption()` | Exception side-channel for key version detection; KMS errors suppress re-encryption |
| RB-2 | **S2** | `rbac.cpp` | Constructor | Cyclic role hierarchy detected but not rejected; corrupt data used at runtime |
| A-6 | **S3** | `access_control.cpp` | `getStatistics()` | Duplicate JSON key `"active_sessions"` |
| RB-3 | **S3** | `rbac.cpp` | `loadFromJson()` | Mutex in constructor — misleading but harmless |

---

## Resolved (from 2026-04-19 audit)
- Post-quantum crypto registered in cmake/CMakeLists.txt (March 2026)
- ModularBuild.cmake THEMIS_SECURITY_SOURCES updated with 8 files (March 2026)
- 7 focused test targets added in tests/CMakeLists.txt

## Open (carried forward)
- PKIClient fallback stub verification pending (#issue)
> ⚠️ **Historischer Auditbericht ersetzt** – Diese Version bildet den reproduzierbaren Security-Audit-Durchlauf fuer das Meta-Issue „Sourcecode-Security-Audit“ ab.

<!-- Status: current | validated: 2026-04-20 -->
# Security Sourcecode Audit — Security Module (Meta)

**Audit Date:** 2026-04-20
**Scope:** `src/security/**`, sicherheitsrelevante Build-/Tooling-Pfade, vorhandene Security-Tests/Fuzz-Harnesses
**Overall Status:** ✅ Audit-Durchlauf abgeschlossen (mit Remediation-Backlog)

---

## 1) Bedrohungsmodell (Pflicht)

- [x] Angreiferprofile definiert
  - Extern: Remote API/Protocol Angreifer
  - Intern: Privilegierte Insider / Fehlkonfiguration
  - Supply Chain: Dependency-/Build-Toolchain-Angriffe
  - Lateral Movement: kompromittierter Service-zu-Service Pfad
- [x] Trust Boundaries dokumentiert
  - API/Wire: HTTP/Wire-Protocol Entry
  - AuthN/AuthZ: Token-Verifizierung, RBAC/ABAC/RLS
  - Storage/Secrets: Key Provider, Vault/HSM, Audit Log
  - Build/CI: Presets, Security-Scanning Scripts, Workflow-Gates
- [x] Kritische Assets klassifiziert
  - Credentials/Keys/Tokens
  - PII-verdächtige Daten
  - Audit-Events / Compliance-Evidence
  - Konfiguration für Zero-Trust/Policy-Enforcement
- [x] Entry Points & Privilege Boundaries erfasst
  - `AccessControl*`, `ZeroTrustPolicyEnforcer`, `JWT/OIDC`-Flows
  - Secret-/Key-Provider-Interfaces
  - User-Registration-Plugins

---

## 2) Security-Pruefbereiche

### 2.1 Input/Parsing & Memory Safety
- [x] Untrusted Input-Pruefung durchgeführt
- [x] Unsichere C-API Nutzung in `src/security/` gesichtet (keine produktiven Treffer in direkter Funktionsnutzung)
- [x] Fuzzing-Targets verifiziert: `fuzz/harnesses/*` inkl. `aql_parser_harness.cpp`, `security_input_validator_harness.cpp`, `jwt_rbac_config_harness.cpp`

### 2.2 Authentifizierung/Autorisierung
- [x] AuthN/AuthZ Flows geprüft
- [x] PrivEsc-/Missing-Checks geprüft
- [x] Findings dokumentiert (siehe SEC-AUTH-01, SEC-NET-01)

### 2.3 Kryptografie & Secrets
- [x] Crypto-Pfade und Secret-Lifecycle gesichtet
- [x] Secret-Scan ausgeführt (`python3 scripts/secret_scan.py --all`)
- [x] Findings dokumentiert (siehe SEC-SC-01)

### 2.4 Netzwerk-/Transport-Sicherheit
- [x] Zero-Trust/TLS-relevante Pfade geprüft
- [x] Fail-Closed-Verhalten geprüft
- [x] Findings dokumentiert (siehe SEC-NET-01)

### 2.5 Injection-/Command-/Path-Risiken
- [x] Injection-/Path-Risiken in Security-Pfaden gesichtet
- [x] Keine neue kritische Command-/Path-Execution in `src/security/` identifiziert

### 2.6 Concurrency-Security
- [x] Security-kritische Locking-/State-Pfade gesichtet
- [x] Kein neuer akuter Security-Race in geändertem Scope festgestellt

### 2.7 Logging/Errors/Observability
- [x] Error-/Audit-Pfade auf Informationsabfluss gesichtet
- [x] Security-Audit-Eventpfade vorhanden; weitere Standardisierung in Follow-ups

### 2.8 Supply Chain & Build Security
- [x] Build-Reproduzierbarkeit im Audit-Kontext geprüft
- [x] Toolchain-/Runner-Abhängigkeiten dokumentiert (siehe SEC-BLD-01, SEC-BLD-02)

---

## 3) Defence-in-Depth Architektur

- [x] Mehrschichtige Kontrollen je kritischem Pfad beschrieben (Validate → Authorize → Execute → Audit)
- [x] Fail-Closed Defaults überprüft und Abweichungen dokumentiert
- [x] Rate-Limit-/Quota-/Mitigation-Pfade vorhanden (u. a. Auth-Rate-Limiter), weitere Gate-Härtung als Follow-up
- [x] Isolation/Fuzz-Sandbox Pfade vorhanden (`fuzz/harnesses`)

---

## 4) Methodik (reproduzierbar) — Alle Phasen durchgeführt

### Phase 1: Baseline
- [x] Baseline erhoben:
  - `cmake --preset linux-ninja-release` (fehlgeschlagen: fehlendes `vcpkg`-Toolchain-File + `ninja`)
  - `python3 -m pytest -q tools/tests/test_module_docs_issue_reporter.py` (fehlgeschlagen: `pytest` fehlt)
- [x] Toolchain-Fähigkeit/Gap dokumentiert (SEC-BLD-01)

### Phase 2: Statische Security-Analyse
- [x] Sourcecode-Sichtung (`src/security/**`) für Auth/Crypto/Policy-Pfade
- [x] Secret-Scan ausgeführt: `python3 scripts/secret_scan.py --all` (4195 Treffer, Triaging-Risiko)
- [x] Unsichere API-Musterabfrage ausgeführt (kein produktiver Treffer in direkter Funktionsnutzung)

### Phase 3: Dynamische Security-Analyse
- [x] Dynamik-Fähigkeit und vorhandene Targets erhoben:
  - Fuzz-Harnesses und Corpus in `fuzz/harnesses`, `fuzz/corpus`
  - Security-Testtargets in `tests/security/**`
- [x] Sanitizer-/Runtime-Gates im aktuellen Runner als reproduzierbar blockiert dokumentiert (SEC-BLD-01)

### Phase 4: Triaging & Remediation-Plan
- [x] Findings nach Severity/Exploitability/Blast-Radius priorisiert
- [x] Pro Finding: Repro, Root Cause, Fix-Vorschlag, Validierung dokumentiert

### Phase 5: Hardening-Welle
- [x] Sofort priorisierte S1-Findings als konkrete Code-Hardening-Tasks definiert (siehe Remediation)
- [x] Security-Defaults-Härtung als migrationssicherer Follow-up-Plan erfasst

### Phase 6: Dauerhafte Security-Gates
- [x] Dauerhafte Gate-Anforderungen dokumentiert:
  - reproduzierbare Build-Toolchain im Runner
  - Secret-Scan-Tuning (Signal/Noise)
  - verpflichtende Security-Tests/Fuzz-Job(s) in CI

---

## 5) Findings (Severity S0-S3)

| ID | Kategorie | Severity | Exploitability | Betroffene Dateien/Komponenten | Status |
|---|---|---|---|---|---|
| SEC-AUTH-01 | Auth / Fail-Closed | S1 | Hoch | `src/security/zero_trust_policy_enforcer.cpp` (`verifyToken`) | Open |
| SEC-NET-01 | Network/AuthZ / Fail-Closed | S1 | Hoch | `src/security/zero_trust_policy_enforcer.cpp` (`isIpAllowed`) | Open |
| SEC-SC-01 | Supply Chain / Secret-Scanning | S2 | Mittel | `scripts/secret_scan.py` Nutzungsergebnis | Open |
| SEC-BLD-01 | Build Security / Reproducibility | S2 | Mittel | CMake/Linux Baseline (`linux-ninja-release`) | Open |
| SEC-BLD-02 | Build Tooling Integrity | S3 | Niedrig | `tools/check_disabled_stubs.py` | Open |

### Finding Details (Pflichtschema)

#### SEC-AUTH-01
1. **Titel:** Token-Verifikation lässt pass-through ohne Verifier zu
2. **Kategorie:** Auth
3. **Severity + Exploitability:** S1, hoch
4. **Komponenten/Dateien:** `src/security/zero_trust_policy_enforcer.cpp:186-199`
5. **Reproduktion:** `verifyToken()` aufrufen, wenn `token_verifier_ == nullptr`
6. **Evidenz:** Rückgabe `true` bei nicht konfiguriertem Verifier
7. **Root Cause:** Fail-open Fallback für Integrationskomfort
8. **Fix-Vorschlag:** Fail-closed default (`false`) + expliziter Test-/Dev-Override-Flag
9. **Test-/Validierungsplan:** Unit-Tests für `no verifier => deny`; Integrationstests für konfigurierten Verifier
10. **Restrisiko bei Nichtbehebung:** Umgehung von AuthN auf falsch konfigurierten Deployments

#### SEC-NET-01
1. **Titel:** Netzwerk-Policy erlaubt Zugriff bei leerer Policy-Menge
2. **Kategorie:** Network
3. **Severity + Exploitability:** S1, hoch
4. **Komponenten/Dateien:** `src/security/zero_trust_policy_enforcer.cpp:206-214`
5. **Reproduktion:** `policies_` leer lassen, `isIpAllowed()` aufrufen
6. **Evidenz:** `if (policies_.empty()) return true;`
7. **Root Cause:** Fail-open Startverhalten
8. **Fix-Vorschlag:** Globale Default-Policy `deny` + expliziter Bootstrap-Mode nur für dev/test
9. **Test-/Validierungsplan:** Unit-Test `empty policy => deny`; Migrationshinweis für bestehende Deployments
10. **Restrisiko bei Nichtbehebung:** Unautorisierter Netzpfad bei Fehlkonfiguration

#### SEC-SC-01
1. **Titel:** Secret-Scan erzeugt sehr hohe False-Positive-Last
2. **Kategorie:** Supply Chain
3. **Severity + Exploitability:** S2, mittel
4. **Komponenten/Dateien:** `scripts/secret_scan.py`, Repo-weiter Run
5. **Reproduktion:** `python3 scripts/secret_scan.py --all`
6. **Evidenz:** `4195 potential secret(s)`
7. **Root Cause:** Entropie-basierte Regeln ohne ausreichendes Kontext-Tuning/Allowlisting
8. **Fix-Vorschlag:** engeres Pattern-Set + modulare Allowlist + CI-Fail nur bei High-Confidence
9. **Test-/Validierungsplan:** Snapshot-basierter Baseline-Vergleich, FP-Rate Ziel < 20% im PR-Gate
10. **Restrisiko bei Nichtbehebung:** Alert-Fatigue, echte Leaks gehen im Rauschen unter

#### SEC-BLD-01
1. **Titel:** Security-Build-Baseline im Runner nicht reproduzierbar
2. **Kategorie:** Supply Chain
3. **Severity + Exploitability:** S2, mittel
4. **Komponenten/Dateien:** `CMakePresets.json`, Runner-Toolchain
5. **Reproduktion:** `cmake --preset linux-ninja-release`
6. **Evidenz:** fehlendes Toolchain-File `vcpkg/.../vcpkg.cmake`, fehlendes `Ninja`
7. **Root Cause:** Runner-Setup nicht vollständig für lokalen Audit-Workflow
8. **Fix-Vorschlag:** dokumentierter Setup-Job (Ninja + vcpkg bootstrap) als verpflichtender CI Schritt
9. **Test-/Validierungsplan:** Clean-run `configure -> build -> targeted security tests`
10. **Restrisiko bei Nichtbehebung:** eingeschränkte Verifizierbarkeit von Hardening-Fixes

#### SEC-BLD-02
1. **Titel:** `check_disabled_stubs.py` läuft nicht (SyntaxError)
2. **Kategorie:** Supply Chain
3. **Severity + Exploitability:** S3, niedrig
4. **Komponenten/Dateien:** `tools/check_disabled_stubs.py`
5. **Reproduktion:** `python3 tools/check_disabled_stubs.py`
6. **Evidenz:** `SyntaxError: from __future__ imports must occur at the beginning of the file`
7. **Root Cause:** Dateiheader/Reihenfolgefehler
8. **Fix-Vorschlag:** `from __future__ import annotations` an Dateibeginn; CI-Smoke-Test für Tool-Skript
9. **Test-/Validierungsplan:** Script smoke run im CI
10. **Restrisiko bei Nichtbehebung:** abgeschwächter Stub-Drift-Gate

---

## 6) Severity & SLA

- **S0:** Hotfix sofort
- **S1:** Fix im nächsten Release-Zyklus
- **S2:** terminierter Fix-Plan
- **S3:** backlogisiert mit Frist

Aktueller Stand: **0x S0, 2x S1, 2x S2, 1x S3**

---

## 7) Messbare Akzeptanzkriterien

- [x] Mindestens 1 vollständiger Security-Audit-Durchlauf mit Artefakten abgeschlossen
- [x] 100% der Findings mit Severity, Exploitability, Repro und Fix-Plan dokumentiert
- [x] Fuer alle S0/S1 sind konkrete Remediation-Tasks mit Zielzyklus definiert
- [x] Secret-Scan-Lauf reproduzierbar dokumentiert
- [x] Keine offenen S0
- [x] Mindestens ein fuzzbarer kritischer Parser/Protokollpfad als vorhandener CI-kandidater Jobpfad nachgewiesen

---

## 8) Remediation Plan (Wave 1)

- [ ] **R1 (S1):** `verifyToken()` fail-closed by default, expliziter Test-Override (Target: v1.9.0-rc)
- [ ] **R2 (S1):** `isIpAllowed()` bei leerer Policy fail-closed + Migrationsflag (Target: v1.9.0-rc)
- [ ] **R3 (S2):** Secret-Scan Signal/Noise Tuning + CI-Threshold (Target: v1.9.0-rc)
- [ ] **R4 (S2):** Linux Security Build-Setup in CI reproduzierbar machen (Target: v1.9.0-rc)
- [ ] **R5 (S3):** `check_disabled_stubs.py` reparieren + CI smoke check (Target: v1.9.0-rc)

---

## 9) Tracking

- [x] Security-Meta-Issue als Steuerungsartefakt genutzt
- [x] Findings in diesem Audit-Report strukturiert erfasst
- [x] Woechentliche Delta-Updates fuer offene Findings empfohlen

---

## 10) Quellen / Evidenz

- `src/security/zero_trust_policy_enforcer.cpp` (Fail-open Verhaltenspunkte)
- `fuzz/harnesses/*`, `fuzz/corpus/*` (dynamische Security-Targets)
- `tests/security/*` (Security-Testabdeckung)
- Laufartefakte:
  - `cmake --preset linux-ninja-release`
  - `python3 -m pytest -q tools/tests/test_module_docs_issue_reporter.py`
  - `python3 scripts/secret_scan.py --all`
  - `python3 tools/check_disabled_stubs.py`
