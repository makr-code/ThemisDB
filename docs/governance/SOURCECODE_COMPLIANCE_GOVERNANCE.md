# Sourcecode Compliance & Governance – Analysebericht

**Stand:** 2026-04-21
**Scope:** `src/` (1 356 `.cpp`-Dateien, 15 Header-Dateien direkt in `src/`)
**Methodik:** Statische Code-Analyse via grep-basierter Mustersuche auf allen Quelldateien

---

## 1. Zusammenfassung der Befunde

| Kategorie | Schwere | Anzahl |
|---|---|---|
| Auskommentierte AuthZ-Checks (arbitrary code execution) | **Critical** | 3 Stellen |
| SSL/TLS Zertifikatsverifikation deaktiviert | **Critical** | 3 Komponenten |
| SAML akzeptiert SHA-1 Digest/Signatur | **High** | 1 Datei |
| Silent Exception-Swallowing in Security-Code | **High** | 18 Stellen |
| MD5 für Datei-Integritätsprüfung | **Medium** | 1 Datei |
| `sscanf` auf HTTP-Eingaben ohne konsistente Längenprüfung | **Medium** | 4 Stellen |
| `delete[]` ohne RAII | **Low** | 5 Stellen |
| Fehlende RBAC-Granularität in API-Handlern | **Low** | 2 Handlern |

---

## 2. Critical: Auskommentierte AuthZ-Checks – Arbitrary Code Execution

### `src/scheduler/task_scheduler.cpp`

Das Modul trägt explizit einen eigenen Sicherheitshinweis im Quelltext:

```
// ⚠️ SECURITY WARNING: This implementation executes arbitrary AQL queries and functions.
// Production deployments MUST implement proper security controls
```

Drei Permission-Checks sind auskommentiert:

| Zeile | Methode | Fehlende Permission |
|---|---|---|
| 510 | `registerTask()` | `task:register` |
| 711 | `executeTask()` | `task:execute` |
| 1286 | `registerFunction()` | `task:register_function` + `system_admin` |

Zeile 1279 markiert den kritischsten Punkt:

```cpp
// ⚠️ SECURITY CRITICAL: This allows arbitrary code execution
```

**Befund:** Der Task Scheduler kann ohne Authentifizierung beliebige AQL-Queries und
Funktionen ausführen. Dies ist ein direkter Pfad für Privilege Escalation und
Remote Code Execution.

**Empfehlung:** Die auskommentierten Checks aktivieren, in Integrationstests
verifizieren, Audit-Log für jeden Aufruf sicherstellen.

---

## 3. Critical: SSL/TLS Zertifikatsverifikation deaktiviert

### 3.1 `src/server/http_server.cpp` (Zeilen 1688, 2000)

```cpp
// Wenn tls_ca_cert_path leer ist:
ssl_ctx_->set_verify_mode(boost::asio::ssl::verify_none);
// Auch beim TLS-Context-Reload:
new_ctx->set_verify_mode(boost::asio::ssl::verify_none);
```

Der Server fällt bei fehlender CA-Konfiguration still auf `verify_none` zurück
ohne Warnung oder Fehler.

### 3.2 `src/network/quic_server.cpp` (Zeile 680)

```cpp
if (!config_.verify_tls) {
    SSL_CTX_set_verify(ssl_ctx_, SSL_VERIFY_NONE, nullptr);
}
```

Ein Konfigurations-Flag deaktiviert die TLS-Verifikation für QUIC vollständig.

### 3.3 `src/server/mqtt_client_service.cpp` (Zeile 751)

```cpp
// Wenn kein CA-Pfad konfiguriert:
ctx.set_verify_mode(boost::asio::ssl::verify_none, ec);
```

**Befund:** Alle drei Komponenten fallen bei fehlender CA-Konfiguration auf
`verify_none` zurück ohne Logging der unsicheren Konfiguration.

**Empfehlung:** Beim Aktivieren von `verify_none` mindestens `THEMIS_ERROR` loggen.
Für Produktivbetrieb `verify_none` via Build-Flag (`THEMIS_ALLOW_TLS_VERIFY_NONE`)
sperren.

---

## 4. High: SAML akzeptiert SHA-1 Signaturen

**Datei:** `src/auth/saml_authenticator.cpp`, Zeilen 338–353

```cpp
// Digest algorithm
} else if (digest_algorithm_uri.find("sha1") != std::string::npos ||
           digest_algorithm_uri.find("SHA1") != std::string::npos) {
    digest_md = EVP_sha1();

// Signature algorithm
} else if (sig_algorithm_uri.find("rsa-sha1") != std::string::npos ||
           sig_algorithm_uri.find("RSA-SHA1") != std::string::npos) {
    sig_md = EVP_sha1();
```

**Befund:** SHA-1 ist kryptografisch gebrochen (SHAttered-Angriff 2017). Ein
kompromittierter IdP kann gefälschte SAML-Assertions einschleusen.

**Nicht als Befund:** HMAC-SHA1 in `src/auth/mfa_authenticator.cpp:301` ist
kein Befund – TOTP/HOTP (RFC 4226/6238) schreibt HMAC-SHA1 vor.

**Empfehlung:** SHA-1 in SAML ablehnen; nur SHA-256+ akzeptieren. Bestehende
SAML-Verbindungen auf SHA-256 migrieren.

---

## 5. High: Silent Exception-Swallowing in sicherheitskritischem Code

Folgende Stellen schlucken Exceptions in Security-Pfaden ohne Logging:

| Datei | Zeile | Risiko |
|---|---|---|
| `src/security/aql_injection_detector.cpp` | 237 | Detection-Fehler → Injection unerkannt |
| `src/security/zero_trust_policy_enforcer.cpp` | 314, 362 | Policy-Fehler → fail-open möglich |
| `src/auth/saml_authenticator.cpp` | 1134 | Assertion-Expiry-Parse-Fehler → Expiry nicht geprüft |
| `src/security/confidential_computing.cpp` | 278, 325 | Attestation-Fehler unsichtbar |
| `src/security/keyprovider_signing.cpp` | 60, 83 | Signing-Fehler versteckt |
| `src/security/vcc_pki_client.cpp` | 344 | PKI-Verbindungsfehler stumm (hier: fail-closed, akzeptabel) |
| `src/security/malware_scanner.cpp` | 712, 883 | Scanner-Fehler → Status unklar |
| `src/security/field_encryption.cpp` | 80 | Debug-Dump-Pfad (unkritisch) |
| `src/security/row_level_security.cpp` | 107 | RLS-Auswertungsfehler stumm |
| `src/security/rbac.cpp` | 197 | RBAC-Lade-Fehler stumm |

Kritischste Stelle: `src/security/aql_injection_detector.cpp:237` – wenn der
Detector eine Exception wirft, kann eine Injection unerkannt durchkommen.

**Empfehlung:** In sicherheitskritischen catch-Blöcken mindestens `THEMIS_ERROR`
loggen. Bei Auswertungsfehlern in Security-Modulen fail-closed verhalten
(Anfrage ablehnen).

---

## 6. Medium: MD5 für Datei-Integritätsprüfung

**Datei:** `src/utils/checksum_utils.cpp`, Zeilen 64–75

```cpp
MD5_CTX md5;
MD5_Init(&md5);
// ...
MD5_Update(&md5, buffer.data(), file.gcount());
MD5_Final(hash, &md5);
```

**Befund:** MD5 ist kollisionsanfällig und für kryptografische Integritätsprüfungen
nicht geeignet (SLSA, Supply-Chain-Anforderungen).

**Nicht als Befund:** Content-MD5 in `blob_backend_s3.cpp` / `blob_backend_azure.cpp`
ist HTTP-API-Standard dieser Storage-Dienste.

**Empfehlung:** `checksum_utils.cpp` auf SHA-256 (OpenSSL EVP) umstellen.

---

## 7. Medium: `sscanf` auf HTTP-Eingaben

**Datei:** `src/server/query_api_handler.cpp`, Zeilen 881, 884, 1091, 1096

```cpp
// Zeile 881: korrekt – Längenprüfung vor sscanf
if (s.size() == 10 && std::sscanf(s.c_str(), "%d-%d-%d", &Y,&M,&D) == 3) { ... }

// Zeile 884: fehlende Längenprüfung vor sscanf
if (std::sscanf(s.c_str(), "%d-%d-%d%c%d:%d:%d%c", &Y,&M,&D,&T,&h,&m,&sec,&Z) >= 7) { ... }
```

**Befund:** Inkonsistente Längenprüfung. Zeile 884 und 1096 fehlt die Prüfung der
Eingabelänge vor dem `sscanf`-Aufruf.

**Empfehlung:** Konsistent vor jedem `sscanf`-Aufruf auf HTTP-Eingaben die Länge
prüfen oder auf `std::istringstream`/`std::get_time` umstellen.

---

## 8. Low: `delete[]` ohne RAII

| Datei | Zeile | Kontext |
|---|---|---|
| `src/storage/zero_copy_blob_transfer.cpp` | 203 | `delete[] static_cast<uint8_t*>(mapping_)` |
| `src/content/ocr_processor.cpp` | 257 | `delete[] raw_text` |
| `src/utils/memory/pool_allocator.cpp` | 133, 391, 629 | Pool-Allokator (intentional manuell) |

**Empfehlung:** `zero_copy_blob_transfer.cpp` und `ocr_processor.cpp` auf
`std::unique_ptr<uint8_t[]>` umstellen. Pool-Allokator ist intentional manuell
und benötigt keine Änderung.

---

## 9. Low: Fehlende RBAC-Granularität in API-Handlern

### `src/server/rope_api_handler.cpp` (Zeile 869)

```cpp
// Note: Fine-grained permission checks (vector:read, vector:write, data:read, data:write)
// are not yet implemented. [...]
// Current behavior: If authentication is enabled, all authenticated requests are allowed.
```

Alle authentifizierten Nutzer haben vollen Lese- und Schreibzugriff.

### `src/server/async_job_api_handler.cpp` (Zeile 377)

```cpp
// Prüft nur ob Authorization-Header vorhanden – keine Token-Validierung hier
if (auth_ && auth_->isEnabled()) {
    const auto auth_hdr = req[http::field::authorization];
    if (std::string(auth_hdr).empty()) { return 401; }
    // Delegation der eigentlichen Validierung an Executor (keine Garantie)
```

**Empfehlung:** ROPE-Handler mit RBAC-Permissions (`vector:read`, `vector:write`)
versehen. Async-Job-Handler Token-Validierung vor Delegation sicherstellen.

---

## 10. Nicht als Befund eingestuft

| Muster | Datei | Begründung |
|---|---|---|
| HMAC-SHA1 | `src/auth/mfa_authenticator.cpp:301` | RFC 4226/6238 TOTP-Standard |
| `verify_none` in Debug-Dump | `src/security/field_encryption.cpp:80` | Nicht produktiver Pfad |
| `catch → return false` bei PKI-Health | `src/security/vcc_pki_client.cpp:344` | Fail-closed (false = unhealthy) |
| Content-MD5 in Blob Storage | `src/storage/blob_backend_s3.cpp` | HTTP-API-Standard |
| Token-Strings in Vorlagen | `src/observability/alertmanager.cpp:456` | Template-Platzhalter |

---

## 11. Compliance-Control-Matrix (aus Analyse)

| Control ID | Schwere | Bereich | Befund | Datei / Zeile | Status |
|---|---|---|---|---|---|
| C-001 | Critical | AuthZ | Scheduler ohne AuthZ – arbitrary AQL/code execution | `src/scheduler/task_scheduler.cpp:510,711,1286` | Missing |
| C-002 | Critical | TLS | `verify_none` ohne Warnung als Fallback (HTTP-Server) | `src/server/http_server.cpp:1688,2000` | Partial |
| C-003 | Critical | TLS | `SSL_VERIFY_NONE` via Flag aktivierbar (QUIC) | `src/network/quic_server.cpp:680` | Partial |
| C-004 | Critical | TLS | `verify_none` bei fehlendem CA-Pfad (MQTT) | `src/server/mqtt_client_service.cpp:751` | Partial |
| C-005 | High | Krypto | SAML akzeptiert SHA-1 Digest und Signatur | `src/auth/saml_authenticator.cpp:338-353` | Missing |
| C-006 | High | Error Handling | Silent catch in Injection-Detector → fail-open | `src/security/aql_injection_detector.cpp:237` | Missing |
| C-007 | High | Error Handling | Silent catch in Zero-Trust-Enforcer → fail-open | `src/security/zero_trust_policy_enforcer.cpp:314,362` | Missing |
| C-008 | High | Error Handling | Silent catch in SAML Assertion-Expiry-Parse | `src/auth/saml_authenticator.cpp:1134` | Missing |
| C-009 | Medium | Krypto | MD5 für Datei-Integritätsprüfung | `src/utils/checksum_utils.cpp:64` | Missing |
| C-010 | Medium | Input Val. | `sscanf` ohne Längenprüfung auf HTTP-Input | `src/server/query_api_handler.cpp:884,1096` | Partial |
| C-011 | Low | AuthZ | Keine RBAC-Granularität in ROPE-Handler | `src/server/rope_api_handler.cpp:869` | Missing |
| C-012 | Low | AuthZ | Async-Job-Auth prüft nur Header-Präsenz | `src/server/async_job_api_handler.cpp:377` | Partial |
| C-013 | Low | Memory | `delete[]` ohne RAII | `src/storage/zero_copy_blob_transfer.cpp:203` | Partial |

---

## 12. Priorisierter Remediation-Backlog

| Gap ID | Schwere | Maßnahme | Datei | Deadline |
|---|---|---|---|---|
| GAP-001 | Critical | Scheduler AuthZ-Checks aktivieren und testen | `src/scheduler/task_scheduler.cpp` | Q2 2026 |
| GAP-002 | Critical | `verify_none`-Fallback mit THEMIS_ERROR versehen | `src/server/http_server.cpp`, `quic_server.cpp`, `mqtt_client_service.cpp` | Q2 2026 |
| GAP-003 | High | SAML SHA-1 ablehnen; nur SHA-256+ akzeptieren | `src/auth/saml_authenticator.cpp` | Q2 2026 |
| GAP-004 | High | catch-Blöcke in Security-Modulen: fail-closed + THEMIS_ERROR | `src/security/aql_injection_detector.cpp`, `zero_trust_policy_enforcer.cpp` | Q2 2026 |
| GAP-005 | Medium | MD5 → SHA-256 in checksum_utils | `src/utils/checksum_utils.cpp` | Q3 2026 |
| GAP-006 | Medium | `sscanf` Längenprüfung normieren | `src/server/query_api_handler.cpp` | Q3 2026 |
| GAP-007 | Low | ROPE/AsyncJob RBAC-Granularität implementieren | `src/server/rope_api_handler.cpp`, `async_job_api_handler.cpp` | Q3 2026 |

---

## 13. Analyse-Methodik (reproduzierbar)

Grep-Muster für Wiederholung der Analyse:

```bash
# AuthZ-Checks auskommentiert
grep -rn --include="*.cpp" -E '⚠️ SECURITY|^\s*//.*hasPermission' src/

# SSL verify_none
grep -rn --include="*.cpp" --include="*.h" \
  -E 'verify_none|SSL_VERIFY_NONE' src/

# Silent exception swallowing in security/auth
grep -rn --include="*.cpp" -A2 'catch\s*(\.\.\.)' src/security/ src/auth/

# Schwache Kryptografie
grep -rn --include="*.cpp" --include="*.h" \
  -E '\b(MD5_Init|EVP_sha1|SHA1)\b' src/

# sscanf auf HTTP-Eingaben
grep -rn --include="*.cpp" 'sscanf\|std::sscanf' src/server/

# Rohes delete[]
grep -rn --include="*.cpp" 'delete\s*\[\]' src/
```
