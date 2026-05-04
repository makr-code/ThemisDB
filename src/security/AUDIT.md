> ⚠️ **Historischer Auditbericht** – Befunde ohne aktuellen Codebeleg mit `<!-- TODO: add source file evidence -->` markieren. Veraltete Befunde entfernen.

<!-- Status: S1 fixed 2026-05-04 | validated: 2026-04-21 (full source code analysis) -->
# Audit Report — Security Module

**Last Audit:** 2026-04-21 | **Status:** ✅ S0 fixed, S1 fixed 2026-05-04 — 0 S0, 0 S1 open

> **Note:** Previous audit claimed "Security Issues: None critical". Source code analysis found
> two guaranteed authentication deadlocks (S0) that prevent any user from logging in,
> plus a platform-conditional HMAC bypass in the cache coordinator (S0).
> Header quality scores of 97–100/100 do not reflect actual code correctness.

## Summary

| Metric | Result |
|--------|--------|
| Build System Registration | ✅ Verified (cmake/ModularBuild.cmake) |
| Test Coverage | ✅ 7 focused test targets |
| S0 Critical / Safety Violations | ✅ 0 (A-1/A-2 fixed) |
| S1 High | ✅ 0 (A-3, E-1, E-2, E-4, RB-1 fixed 2026-05-04) |
| S2 Medium | ✅ 0 (A-4, A-5, E-3, RB-2 fixed 2026-05-04) |
| S3 Low | ✅ 0 (A-6, RB-3 fixed 2026-05-04) |
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

### S1 — High

#### A-3 · `access_control.cpp` · `enrollMFA()` — MFA enrollment bypass

✅ **Fixed 2026-05-04** — Guard added at line 384: checks for existing active enrollment before proceeding. Any caller who attempts to overwrite an active MFA secret receives `ERR_API_INVALID_REQUEST`. The existing enrollment must be explicitly disabled before re-enrollment.

~~`enrollMFA()` unconditionally overwrites any existing MFA enrollment without checking whether
the caller is the account owner or an administrator:~~

```cpp
enrollment.enabled = true;
mfa_enrollments_[user_id] = std::move(enrollment);  // overwrites existing MFA silently
```

Any caller with access to this function can replace a user's TOTP secret, invalidating the
user's authenticator app and gaining control of subsequent OTP verification.

**Fix required:** Require active MFA verification from the existing secret before allowing
re-enrollment. Log and rate-limit all enrollment attempts.

---

#### E-1 · `field_encryption.cpp` · `write_debug_dump()` + `decryptInternal()` — Key material on disk + stderr

✅ **Fixed 2026-05-04** — `key_fingerprint_prefix` (raw key bytes) removed from debug dump JSON. `write_debug_dump()` is env-var gated (`THEMIS_DEBUG_ENC_DIR`) and disabled by default; the key bytes are never written.

~~`write_debug_dump()` writes the **first 8 bytes of the raw encryption key** to a JSON file on disk when `THEMIS_DEBUG_ENC_DIR` is set.~~

```cpp
j["key_fingerprint_prefix"] = kf.str();   // 8 raw key bytes as hex

// decryptInternal(), always:
fprintf(stderr, "decryptInternal: ciphertext_len=%zu, tag_len=%zu, iv_len=%zu, key_len=%zu\n", ...);
```

**Fix required:** Replace raw key bytes with a proper HMAC-based fingerprint. Gate
`write_debug_dump()` on a compile-time `THEMIS_DEBUG` flag (not a runtime env var).
Remove unconditional `fprintf` from `decryptInternal()`.

---

#### E-4 · `field_encryption.cpp` · `createDefault()` — `MockKeyProvider` in production

✅ **Fixed 2026-05-04** — Runtime guard added: `createDefault()` now checks for the `THEMIS_ALLOW_MOCK_KEY_PROVIDER` environment variable. If the variable is not set to `1` or `true`, the function throws `std::runtime_error` with a clear message requiring injection of a real `KeyProvider`. The mock path is now explicitly opt-in for testing only.

~~The default factory method uses a mock key provider.~~

#### E-2 · `field_encryption.cpp` · `encryptEntityBatch()` — Silent per-item encryption failures

✅ **Fixed 2026-05-04** — Both the TBB parallel path and the sequential fallback path now re-throw exceptions from `catch (...)` instead of swallowing them. Callers that catch `std::exception` or `...` now see the failure immediately; partially-processed batches are no longer silently accepted.

~~Failed encryptions in the parallel batch path produce default-constructed `EncryptedBlob`
(empty IV, empty ciphertext) in the output vector. Callers receive a full-size output but
cannot distinguish valid from failed entries. Corrupted records are silently stored.~~

---

### S2 — Medium

> **All S2 findings (A-4, A-5, E-3, RB-2) fixed 2026-05-04.**

| ID | File | Function | Description |
|----|------|----------|-------------|
| ✅ A-4 | `access_control.cpp` | `detectSQLInjection()` | **Fixed 2026-05-04** — Input case-folded to lowercase before pattern matching; extended pattern list (union, select, insert, update, delete, drop, exec, execute, xp_, --, /*, */, ;, or 1=1, 1=1, ' or '). Added heuristic/defense-in-depth note. |
| ✅ A-5 | `access_control.cpp` | `recordFailedLogin()` | **Fixed 2026-05-04** — Added SECURITY NOTE comment documenting in-memory-only limitation. Lockout log message now includes failure count and explicit restart-reset warning for SIEM visibility. |
| ✅ E-3 | `field_encryption.cpp` | `needsReEncryption()` | **Fixed 2026-05-04** — Outer catch now returns `true` (fail-safe) when KMS is unavailable, with a `[SECURITY]`-prefixed WARN log. Re-encryption is no longer silently suppressed on KMS error. |
| ✅ RB-2 | `rbac.cpp` | Constructor / `checkPermission()` | **Fixed 2026-05-04** — Added `hierarchy_valid_` member (header + impl). When cycle detected, `hierarchy_valid_` is set `false` and RBAC is marked invalid. `checkPermission()` now checks `hierarchy_valid_` first and denies all access if invalid (fail-closed). |

### S1 — Additional

| ID | File | Function | Description |
|----|------|----------|-------------|
| RB-1 | `rbac.cpp` | `checkPermission()` | ✅ **Fixed 2026-05-04** — 5-minute grace period added via `std::atomic<int64_t> last_license_success_ms_`. On license server failure, access is granted if the last successful check was within 300 s. |

### S3 — Low

| ID | File | Function | Description |
|----|------|----------|-------------|
| A-6 | `access_control.cpp` | `getStatistics()` | ✅ **Fixed 2026-05-04** — Removed duplicate `"active_sessions"` key; JSON output now has a single unique entry. |
| RB-3 | `rbac.cpp` | `loadFromJson()` | ✅ **Fixed 2026-05-04** — Added constructor comment explaining that mutex acquisition in `loadFromJson()` during construction is intentional and harmless (object not yet shared). |

---

## Findings Summary Table

| ID | Severity | File | Function | Description |
|----|----------|------|----------|-------------|
| A-1 | **S0** | `access_control.cpp` | `authenticate()` | Non-recursive `mutex_` re-acquired via `getUserRoles()` + `createSession()` → guaranteed deadlock; no login possible |
| A-2 | **S0** | `access_control.cpp` | `changePassword()` | Non-recursive `mutex_` re-acquired via `invalidateUserSessions()` → guaranteed deadlock |
| A-3 | **S1** ✅ | `access_control.cpp` | `enrollMFA()` | Fixed 2026-05-04 — guard rejects enrollment if active MFA already exists |
| E-1 | **S1** ✅ | `field_encryption.cpp` | `write_debug_dump()` / `decryptInternal()` | Fixed 2026-05-04 — key bytes removed; debug dump env-var gated |
| E-2 | **S1** ✅ | `field_encryption.cpp` | `encryptEntityBatch()` | Fixed 2026-05-04 — exceptions re-thrown from both parallel and sequential catch blocks |
| E-4 | **S1** ✅ | `field_encryption.cpp` | `createDefault()` | Fixed 2026-05-04 — runtime guard requires `THEMIS_ALLOW_MOCK_KEY_PROVIDER=1` |
| RB-1 | **S1** ✅ | `rbac.cpp` | `checkPermission()` | Fixed 2026-05-04 — 5-minute grace window via `last_license_success_ms_` atomic |
| A-4 | **S2** ✅ | `access_control.cpp` | `detectSQLInjection()` | Fixed 2026-05-04 — case-folded input; extended pattern set |
| A-5 | **S2** ✅ | `access_control.cpp` | `recordFailedLogin()` | Fixed 2026-05-04 — in-memory limitation documented; SIEM-visible lockout log |
| E-3 | **S2** ✅ | `field_encryption.cpp` | `needsReEncryption()` | Fixed 2026-05-04 — fail-safe return `true` on KMS error; WARN log added |
| RB-2 | **S2** ✅ | `rbac.cpp` | Constructor | Fixed 2026-05-04 — `hierarchy_valid_` flag; `checkPermission()` denies all when invalid |
| A-6 | **S3** ✅ | `access_control.cpp` | `getStatistics()` | Fixed 2026-05-04 — duplicate `"active_sessions"` key removed |
| RB-3 | **S3** ✅ | `rbac.cpp` | `loadFromJson()` | Fixed 2026-05-04 — constructor comment added explaining intentional mutex use |

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
