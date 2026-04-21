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

---

## 14. Deep Dive – Erweiterte Analysebefunde

**Analysedatum:** 2026-04-21 (Runde 2)
**Fokus:** Timing-Angriffe, Path-Traversal, CORS, DoS, SSRF, JSON-Injection, Brute-Force, Token-Logging

---

### 14.1 Timing-Angriff: Nicht-konstant-zeitlicher Token-Vergleich

**Datei:** `src/server/export_api_handler.cpp`, Zeile 443

```cpp
return token == admin_token;   // std::string::operator== – nicht constant-time
```

Der Admin-Token des Export-Endpunkts wird mit `operator==` verglichen. C++-String-Vergleich
ist nicht constant-time: Ein Angreifer kann durch präzise Zeitmessung Byte-für-Byte
herausfinden, wie viele Zeichen des richtigen Tokens er bereits erraten hat (timing side-channel).

**Datei:** `src/server/auth_middleware.cpp`, Zeilen 204–206

```cpp
auto it = tokens_.find(std::string(token));   // map-Lookup – nicht constant-time
if (it != tokens_.end()) { ... }
```

Hash-Map-Lookup via `std::unordered_map::find` ist ebenfalls nicht constant-time.

**Empfehlung:** `CRYPTO_memcmp` (OpenSSL, bereits als Dependency vorhanden) oder
`std::equal` mit Kurzschluss-Vermeidung für alle token-Vergleiche verwenden.

---

### 14.2 Path-Traversal: LLM Model Load ohne Pfadvalidierung

**Datei:** `src/server/http_server.cpp`, Zeilen 3507–3518 und `src/server/llm_api_handler.cpp`, Zeile 637

```cpp
const std::string model_path = payload.value("path", std::string{});
if (model_path.empty()) { /* 400 */ return; }
// Einzige Validierung: path darf nicht leer sein
plugin_mgr.loadModel(model_id, model_path);
```

HTTP-authentifizierte Nutzer können beliebige Dateisystempfade übergeben:
- `../../../etc/passwd` – Dateileck
- `/dev/null` – hängt Loader auf  
- Netzwerkpfade (`smb://`, NFS-Mounts)

Es gibt keine `canonical()`/`lexically_normal()`-Prüfung, keine Whitelist für
erlaubte Modell-Verzeichnisse.

**Empfehlung:** Vor `loadModel()` prüfen:
```cpp
auto canon = std::filesystem::canonical(model_path);
if (!canon.string().starts_with(config_.model_dir)) throw std::runtime_error("path out of sandbox");
```

---

### 14.3 DoS: Graph-BFS ohne `max_depth`-Obergrenze

**Datei:** `src/server/graph_api_handler.cpp`, Zeile 71

```cpp
size_t max_depth = body_json["max_depth"];   // direkt aus HTTP-Body
graph_index_->bfs(start_vertex, static_cast<int>(max_depth));
```

Ein Angreifer kann `max_depth: 999999999` senden und eine vollständige
Graphtraversierung auslösen. Der BFS-Algorithmus (`graph_index.cpp:423`) hat
intern keine Obergrenze – er traversiert alle erreichbaren Knoten bis `maxDepth`.
Bei großen Graphen führt das zu OOM und CPU-Saturation (Denial of Service).

**Empfehlung:** Maximale Tiefe serverseitig beschränken:
```cpp
static constexpr size_t kMaxBfsDepth = 20;
if (max_depth > kMaxBfsDepth) { /* 400 Bad Request */ return; }
```

---

### 14.4 Token-Wert in Startup-Logs (Partial Leak)

**Datei:** `src/server/http_server.cpp`, Zeilen 638–640

```cpp
THEMIS_INFO("Auth check after addToken: validateToken(token='{}') -> ...",
    cfg.token.size() > 8
        ? (std::string(cfg.token).substr(0,4) + "..." + std::string(cfg.token).substr(cfg.token.size()-4))
        : cfg.token);   // ← kurze Tokens werden vollständig geloggt
```

Tokens kürzer als 9 Zeichen werden im Klartext in die THEMIS_INFO-Logs geschrieben.
Bei Tokens ≥ 9 Zeichen werden 8 Zeichen (Prefix + Suffix) exponiert – bei einem
8-Zeichen-Token sind das 100 % des Wertes.

**Befund:** Tokens mit ≤ 8 Zeichen werden vollständig geloggt. Selbst bei längeren
Tokens reduziert das Loggen von Prefix+Suffix den effektiven Suchraum erheblich.

**Empfehlung:** Nur die Tokenlänge loggen, kein Prefix/Suffix:
```cpp
THEMIS_INFO("Auth check after addToken: token_len={}", cfg.token.size());
```

---

### 14.5 CORS: Wildcard in individuellen Handlern (unkontrolliert)

**Dateien:** `src/server/changefeed_api_handler.cpp:403`,
`src/server/llm_api_handler.cpp:504,571`,
`src/server/query_api_handler.cpp:3447`,
`src/llm/grafana_metrics.cpp:1392`

Diese Handler setzen `Access-Control-Allow-Origin: *` **direkt und hardcoded**,
unabhängig von der zentralen CORS-Konfiguration in `http_server.cpp`:

```cpp
res.set(http::field::access_control_allow_origin, "*");
```

Der zentrale CORS-Handler in `http_server.cpp` hat eine korrekte Origin-Whitelist-Prüfung
(Zeile 9443: `cors_allow_credentials_ && allow_origin != "*"`). Die einzelnen Handler
umgehen diese Logik vollständig.

**Befund:** CORS-Policy wird inkonsistent angewendet. Wenn `cors_allow_credentials_`
aktiv ist und bestimmte Endpunkte trotzdem `*` setzen, ignorieren Browser zwar
`Allow-Credentials`, aber es entsteht eine konfuse und schwer zu auditierende Lage.

**Empfehlung:** Alle CORS-Header müssen über den zentralen Handler gesetzt werden.
Hardcoded `*` in individuellen API-Handlern entfernen.

---

### 14.6 Brute-Force: Kein Rate-Limiting auf Export-Admin-Auth

**Datei:** `src/server/export_api_handler.cpp`, Zeile 443

Der `ExportApiHandler` implementiert eine eigene Token-Verifikation gegen
`THEMIS_TOKEN_ADMIN` – ohne Rate-Limiting, ohne Audit-Log bei fehlgeschlagenem
Versuch, ohne Account-Lockout.

Der zentrale Rate-Limiter in `http_server.cpp` (Zeile 3284) schützt diesen Handler
möglicherweise, abhängig von der Routing-Konfiguration. Es gibt jedoch keinen
expliziten Auth-Failure-Audit-Log-Eintrag in diesem Handler.

**Empfehlung:**
1. Auth-Fehler explizit mit IP-Adresse und Zeitstempel loggen.
2. Rate-Limit explizit im Handler prüfen (nicht nur über globales Routing).

---

### 14.7 Command-Injection: `popen` mit GPG via Single-Quote-Injection

**Datei:** `src/base/module_loader.cpp`, Zeilen 1415, 1449

```cpp
static const std::string kForbidden = "'\";&|`$\n\r\\()\t";
// ...
std::string command = "gpg --verify '" + sigFile + "' '" + modulePath + "' 2>&1";
FILE* pipe = popen(command.c_str(), "r");
```

**Befund:** `kForbidden` enthält `'` – der Single-Quote ist also geblockt. Das
verhindert die einfachste Shell-Injection. **Jedoch:** Der Forbidden-Check prüft
Zeichen, die im Dateisystem-Pfad vorkommen können. Pfade auf Systemen mit exotischen
Locales oder Unicode-Normalisierung können Zeichen enthalten, die nach Byte-Iteration
harmlos erscheinen, aber als Shell-Metazeichen interpretiert werden.

**Bessere Lösung:** `popen()` durch direkten `execvp()`-Aufruf ersetzen, der kein
Shell-Parsing durchführt:
```cpp
// Kein Shell-Parsing, kein Injection-Risiko:
execvp("gpg", {"gpg", "--verify", sigFile.c_str(), modulePath.c_str(), nullptr});
```

---

### 14.8 Backup: User-kontrollierter Pfad in `system()` via Admin-API

**Datei:** `src/server/admin_api_handler.cpp`, Zeile 49 →  
**Datei:** `src/storage/backup_manager.cpp`, Zeilen 940–941

```cpp
// admin_api_handler.cpp:49 – direkt aus HTTP-Body:
std::string dir = body.value("directory", "./data/backup_" + timestamp);
storage_->createCheckpoint(dir);   // dir wird weitergegeben
```

`createCheckpoint()` ruft intern `BackupManager::compressBackup(backup_dir)`, das:

```cpp
std::string cmd = "tar -czf \"" + compressed_file + "\" -C \""
                + fs::path(backup_dir).parent_path().string() + "\" \""
                + fs::path(backup_dir).filename().string() + "\"";
int result = system(cmd.c_str());
```

Ein Admin-Nutzer kann `directory` auf beliebige Pfade setzen. Das führt zu:
- Backup beliebiger Systemverzeichnisse (Datenexfiltration)
- Durch eingebettete `"` im Pfad: potenzielle Shell-Injection in `system()`

**Aber:** Admin-Auth ist vorausgesetzt. Dennoch verstößt dies gegen
Least-Privilege (Admin ≠ root, Backup sollte auf definierte Verzeichnisse beschränkt sein).

**Empfehlung:**
1. `directory` auf eine Whitelist erlaubter Backup-Verzeichnisse beschränken.
2. `system()` durch `std::filesystem` + `libarchive` ersetzen.

---

### 15. Ergänzte Compliance-Control-Matrix (Deep Dive)

| Control ID | Schwere | Bereich | Befund | Datei / Zeile | Status |
|---|---|---|---|---|---|
| C-014 | High | Krypto | Token-Vergleich nicht constant-time (Timing-Angriff) | `src/server/export_api_handler.cpp:443` | Missing |
| C-015 | High | Krypto | Auth-Middleware-Lookup nicht constant-time | `src/server/auth_middleware.cpp:204` | Missing |
| C-016 | High | Path Traversal | LLM model_path ohne Sandboxing (Path Traversal) | `src/server/http_server.cpp:3518` | Missing |
| C-017 | High | DoS | Graph-BFS `max_depth` ohne Obergrenze (DoS) | `src/server/graph_api_handler.cpp:71` | Missing |
| C-018 | Medium | Secrets | Token-Teilwert (Prefix+Suffix) in Startup-Logs | `src/server/http_server.cpp:638` | Partial |
| C-019 | Medium | CORS | Hardcoded `*` in 4 API-Handlern, umgeht zentrale CORS-Policy | `src/server/changefeed_api_handler.cpp:403` u.a. | Missing |
| C-020 | Medium | AuthZ | Kein Auth-Failure-Audit-Log in Export-Handler | `src/server/export_api_handler.cpp` | Missing |
| C-021 | Medium | Injection | `popen(gpg)` via Shell-String – besser `execvp()` | `src/base/module_loader.cpp:1449` | Partial |
| C-022 | Medium | Injection | `system(tar)` mit user-kontrolliertem Admin-Backup-Pfad | `src/storage/backup_manager.cpp:940` | Partial |

---

### 16. Kompletter Remediation-Backlog (alle Runden)

| Gap ID | Schwere | Maßnahme | Datei | Deadline |
|---|---|---|---|---|
| GAP-001 | Critical | Scheduler AuthZ-Checks aktivieren | `src/scheduler/task_scheduler.cpp:510,711,1286` | Q2 2026 |
| GAP-002 | Critical | `verify_none` mit THEMIS_ERROR versehen | `src/server/http_server.cpp:1688`, `quic_server.cpp:680`, `mqtt_client_service.cpp:751` | Q2 2026 |
| GAP-003 | High | SAML SHA-1 ablehnen | `src/auth/saml_authenticator.cpp:338` | Q2 2026 |
| GAP-004 | High | catch-Blöcke in Security-Modulen fail-closed | `src/security/aql_injection_detector.cpp:237`, `zero_trust_policy_enforcer.cpp:314` | Q2 2026 |
| GAP-008 | High | `CRYPTO_memcmp` für Token-Vergleiche | `src/server/export_api_handler.cpp:443`, `auth_middleware.cpp:204` | Q2 2026 |
| GAP-009 | High | LLM model_path auf Verzeichnis-Sandbox beschränken | `src/server/http_server.cpp:3518`, `llm_api_handler.cpp:637` | Q2 2026 |
| GAP-010 | High | Graph-BFS max_depth serverseitig ≤ 20 begrenzen | `src/server/graph_api_handler.cpp:71` | Q2 2026 |
| GAP-005 | Medium | MD5 → SHA-256 in checksum_utils | `src/utils/checksum_utils.cpp:64` | Q3 2026 |
| GAP-006 | Medium | `sscanf` Längenprüfung normieren | `src/server/query_api_handler.cpp:884,1096` | Q3 2026 |
| GAP-011 | Medium | Token-Logging: nur Länge loggen, kein Prefix/Suffix | `src/server/http_server.cpp:638` | Q3 2026 |
| GAP-012 | Medium | Hardcoded CORS `*` aus API-Handlern entfernen | `changefeed_api_handler.cpp:403`, `llm_api_handler.cpp:504,571`, `query_api_handler.cpp:3447` | Q3 2026 |
| GAP-013 | Medium | Auth-Failure-Audit-Log im Export-Handler | `src/server/export_api_handler.cpp` | Q3 2026 |
| GAP-014 | Medium | `popen(gpg)` → `execvp()` | `src/base/module_loader.cpp:1449` | Q3 2026 |
| GAP-015 | Medium | Backup-Pfad whitelist + `system(tar)` → libarchive | `src/storage/backup_manager.cpp:940,976` | Q3 2026 |
| GAP-007 | Low | ROPE/AsyncJob RBAC-Granularität implementieren | `src/server/rope_api_handler.cpp`, `async_job_api_handler.cpp` | Q3 2026 |

---

### 17. Positiv-Befunde (Best Practices im Code)

Einige Aspekte sind gut implementiert und sollten als Referenz dienen:

| Bereich | Positiv-Befund | Datei |
|---|---|---|
| CORS | Zentrale CORS-Logik mit Origin-Whitelist und Vary-Header | `src/server/http_server.cpp:9406` |
| Input-Injection | AQL-Injection-Detektor mit AST-Analyse (3-stufig) | `src/security/aql_injection_detector.cpp:79` |
| GPG-Injection | kForbidden-Liste mit Shell-Metachar-Whitelist | `src/base/module_loader.cpp:1415` |
| Rate Limiting | API-Gateway und HTTP-Server haben zentrales Rate-Limiting | `src/server/api_gateway.cpp:161`, `http_server.cpp:3284` |
| Body-Size-Limit | `THEMIS_MAX_BODY_BYTES` konfigurierbar (Default 10 MB) | `src/server/http_server.cpp:1612` |
| SAML-Parse-Fehler | SHA-1 wird gewarnt; unsupportete Algo lehnt ab | `src/auth/saml_authenticator.cpp:342` |
| gRPC TLS | gRPC-Transport hat konfigurierbares `max_message_size_bytes` | `src/network/grpc_transport.cpp:200` |
| Token-Logging | Token-Masking für ≥ 9 Zeichen (Prefix+Suffix) existiert | `src/server/http_server.cpp:639` (partiell) |

---

## 18. Deep Dive – Runde 3

**Analysedatum:** 2026-04-21 (Runde 3)
**Fokus:** AQL-Injection im Export, Information-Leakage in Error-Responses, gRPC Insecure Mode, mt19937 in Security-Contexts, unbegrenzte JSON-Array-Iteration

---

### 18.1 AQL-Injection via `buildAqlQuery` (Export Handler)

**Datei:** `src/server/export_api_handler.cpp`, Zeilen 354–388

```cpp
conditions.push_back("category='" + theme + "'");    // theme direkt aus HTTP body
conditions.push_back("domain='" + domain + "'");
// ...
conditions.push_back(custom_query);  // user-supplied AQL without validation
```

`buildAqlQuery()` baut AQL-Queries durch direkte String-Konkatenation aus HTTP-Body-Feldern.
Der Wert `request_json["query"]` wird **unvalidiert direkt als AQL-Condition eingebettet**.
Das erlaubt:
- AQL-Injection: `"query": "OR true"` – gibt alle Datensätze zurück
- AQL-Injection mit Mutation: `"query": "OR UPDATE ..."` – schreibender Zugriff

Das Ergebnis wird **nicht** durch `AQLInjectionDetector::validateAQLAST()` geprüft (überprüft via Quellcode).

**Schwere:** **High** (Datenexfiltration aller Datensätze, potenziell schreibende Operationen)

**Empfehlung:**
1. `custom_query` (Zeile 388) durch `AQLInjectionDetector::validateForReadOnlyContext()` validieren
2. String-Concat-Conditions durch parametrisierte Queries ersetzen (Bind-Parameter-API)
3. `FILTER category == @category` statt `category='...'` (AQL-native Escaping)

---

### 18.2 Information-Leakage: `e.what()` in HTTP-500-Responses

**Dateien:** 245 Stellen in `src/server/*.cpp` – z.B.:
- `src/server/admin_api_handler.cpp:66` – `e.what()` im HTTP 500
- `src/server/admin_api_handler.cpp:54` – interner Pfad in 500: `"Failed to create checkpoint at " + dir`

```cpp
return makeErrorResponse(http::status::internal_server_error, e.what(), req);
return makeErrorResponse(http::status::internal_server_error,
    std::string("Failed to create checkpoint at ") + dir, req);
```

`std::exception::what()` kann enthalten:
- Interne Dateipfade (`/srv/data/themisdb/...`)
- C++-Type-Namen (RTTI, Demangling bei Boost/STL-Exceptions)
- RocksDB-interne Fehlertexte mit Datenbankpfaden
- SQL/AQL-Ausschnitte, die sensitives Schema enthüllen

**Schwere:** **Medium** – Fingerprinting des Servers, erleichtert gezielte Angriffe

**Empfehlung:**
- HTTP-500-Responses nur generische Meldung zurückgeben (`"Internal server error"`)
- `e.what()` in strukturierten Server-Logs mit `THEMIS_ERROR`, aber **nicht** in die HTTP-Antwort
- Einheitlicher Error-Wrapper: `makeInternalErrorResponse(req)` ohne Payload-Parameter

---

### 18.3 gRPC Insecure Mode in Produktion aktivierbar

**Dateien:**
- `src/api/grpc_server.cpp:295` – `InsecureServerCredentials()` wenn `!config_.tls_enabled`
- `src/main_server.cpp:1505` – `InsecureServerCredentials()` wenn mTLS nicht konfiguriert (WAL gRPC)
- `src/network/grpc_transport.cpp:100` – `InsecureServerCredentials()` im Fallback

```cpp
if (!config_.tls_enabled) {
    THEMIS_INFO("GrpcApiServer: using insecure credentials (TLS disabled)");
    return grpc::InsecureServerCredentials();   // ← Plaintext gRPC, kein Auth
}
```

`InsecureServerCredentials()` = kein TLS, kein Mutual-Auth, gRPC-Payload im Klartext.
Die WAL-gRPC-Verbindung kommuniziert WAL-Daten (Datenbankänderungen) **unverschlüsselt** wenn
mTLS nicht konfiguriert ist.

**Befund:** Konfigurationsfehler oder fehlende mTLS-Konfiguration führt zu:
- Plaintext-Replikationsstream (WAL) über Netzwerk
- Kein Client-Auth → beliebige Clients können gRPC-Calls machen
- `THEMIS_WARN` für WAL existiert, fehlt für den API-gRPC-Server komplett

**Schwere:** **High** (je nach Deployment – inakzeptabel in Produktionsumgebungen)

**Empfehlung:**
- Build-Flag oder Runtime-Check: in Production-Mode (`THEMIS_ENV=production`) `InsecureServerCredentials()` verbieten
- `THEMIS_WARN` → `THEMIS_CRITICAL` für alle Insecure-Fallbacks, inkl. Startup-Fehler wenn env = production
- API-gRPC-Server muss eine Warnung auf CRITICAL-Level loggen

---

### 18.4 mt19937 für Security-relevante IDs

**Dateien:**
- `src/auth/auth_error.cpp:215` – `mt19937` für Auth-Request-IDs (`auth-XXXXXXXX`)
- `src/server/export_api_handler.cpp:405` – `mt19937` für Export-IDs (`exp_XXXXXXXXXXXXXXXX`)

```cpp
static std::mt19937 gen(rd());
// ...
ss << std::hex << dis(gen);  // auth-XXXXXXXX or exp_XXXXXXXXXXXXXXXX
```

`std::mt19937` ist eine deterministische PRNG; kein CSPRNG. Der State-Raum ist 2^19937
theoretisch, aber wenn `std::random_device rd` intern auf `std::time()` oder einem anderen
seed mit geringer Entropie zurückfällt (wie auf manchen Embedded-Linux-Systemen), ist die
Entropie erheblich reduziert.

**Risiko:** Export-IDs könnten vorhersagbar sein → Angreifer kann Export-Ergebnisse anderer
Nutzer abrufen (IDOR wenn IDs rate-limitiert oder guessable sind).

**Empfehlung:**
- `RAND_bytes(buf, 8)` (OpenSSL) für kryptografisch sichere IDs
- Alternativ: `std::random_device` direkt ohne MT-Wrapper (auf Linux read from `/dev/urandom`)

---

### 18.5 Unbegrenzte JSON-Array-Iteration (DoS durch großen Batch)

**Dateien:** `src/server/vector_api_handler.cpp:233,300`, `src/server/compliance_reporting_api_handler.cpp:210`,
`src/server/distributed_txn_api_handler.cpp:59`, `src/server/api_key_mgmt_handler.cpp:133` u.a.

```cpp
for (const auto& it : body["items"]) {   // items.size() ist unbegrenzt!
    // vector insert per item
}
```

JSON-Arrays aus dem HTTP-Body werden **ohne Size-Limit** iteriert. Ein Angreifer kann
`{"items": [...1M items...]}` senden (Body-Size-Limit schützt, aber 10 MB reichen für
tausende Vektoren der Dimension 128).

**Schwere:** **Medium** – CPU-Saturation, langer Transaktions-Hold, OOM bei Deserialisierung

**Empfehlung:** Jeder Array-Iterator muss vor der Schleife einen Size-Check haben:
```cpp
if (body["items"].size() > kMaxBatchSize) { return makeErrorResponse(400, "batch too large"); }
```
Empfohlenes `kMaxBatchSize`: konfigurierbar, Default 10.000

---

### 18.6 MQTT-Client: `verify_none` ohne CA-Konfigurationswarnung auf CRITICAL

**Datei:** `src/server/mqtt_client_service.cpp:751`

```cpp
if (!config_.tls_ca_path.empty()) {
    ctx.set_verify_mode(boost::asio::ssl::verify_peer, ec);
} else {
    ctx.set_verify_mode(boost::asio::ssl::verify_none, ec);  // ← kein CRITICAL log
}
```

Wenn kein CA-Pfad konfiguriert ist, akzeptiert der MQTT-Client **jedes TLS-Zertifikat**
(inkl. selbstsignierter und gefälschter). Es gibt keinen `CRITICAL`-Log-Eintrag – lediglich
implizites Verhalten. Ein Man-in-the-Middle-Angriff auf den MQTT-Broker ist damit trivial.

**Schwere:** **High** (MQTT transportiert CDC-Events – Datenexfiltration oder Datenmanipulation)

**Empfehlung:**
- `THEMIS_CRITICAL("MQTT TLS: verify_none active – MITM possible; set tls_ca_path")` bei Aktivierung
- Umgebungsvariable `THEMIS_MQTT_ALLOW_NO_VERIFY` als opt-in mit CRITICAL warning

---

### 19. Ergänzte Compliance-Control-Matrix (Runde 3)

| Control ID | Schwere | Bereich | Befund | Datei / Zeile | Status |
|---|---|---|---|---|---|
| C-023 | High | AQL-Injection | `buildAqlQuery` – user-controlled `custom_query` ohne Validation eingebettet | `src/server/export_api_handler.cpp:388` | Missing |
| C-024 | High | gRPC-Security | `InsecureServerCredentials()` in API-gRPC ohne CRITICAL-Log | `src/api/grpc_server.cpp:295`, `src/main_server.cpp:1505` | Missing |
| C-025 | High | TLS | MQTT-Client `verify_none` ohne CRITICAL-Warnung | `src/server/mqtt_client_service.cpp:751` | Missing |
| C-026 | Medium | Info-Leakage | `e.what()` in 245 HTTP-500-Responses (interne Pfade, C++-Types) | `src/server/admin_api_handler.cpp:66`, widespread | Missing |
| C-027 | Medium | RNG | `mt19937` für Export-IDs (predictable) | `src/server/export_api_handler.cpp:405` | Missing |
| C-028 | Medium | DoS | Unbegrenzte JSON-Array-Iteration in Batch-Endpunkten | `src/server/vector_api_handler.cpp:233`, widespread | Missing |
| C-029 | Low | RNG | `mt19937` für Auth-Request-IDs (diagnostic only – Low risk) | `src/auth/auth_error.cpp:215` | Low |

---

### 20. Gesamtübersicht aller Befunde (alle 3 Runden)

| GAP ID | Schwere | Bereich | Maßnahme | Status |
|---|---|---|---|---|
| GAP-001 | Critical | AuthZ | Scheduler AuthZ-Checks aktivieren | Missing |
| GAP-002 | Critical | TLS | `verify_none` mit CRITICAL-Log + Production-Guard | Partial |
| GAP-003 | High | Krypto | SAML SHA-1 ablehnen | Partial (TODO) |
| GAP-004 | High | Injection | AQL injection in export `buildAqlQuery` | Missing |
| GAP-005 | Medium | Krypto | MD5 → SHA-256 | Missing |
| GAP-006 | Medium | Input | `sscanf` Längenprüfung | Low risk |
| GAP-007 | Low | AuthZ | ROPE/AsyncJob RBAC | Missing |
| GAP-008 | High | Krypto | Constant-time Token-Vergleich | Missing |
| GAP-009 | High | Path | LLM model_path Sandbox | Missing |
| GAP-010 | High | DoS | Graph-BFS max_depth Cap | Missing |
| GAP-011 | Medium | Secrets | Token-Logging bereinigen | Missing |
| GAP-012 | Medium | CORS | Hardcoded `*` durch zentrales CORS ersetzen | Missing |
| GAP-013 | Medium | Auth | Auth-Failure-Audit-Log im Export-Handler | Missing |
| GAP-014 | Medium | Injection | popen(gpg) → execvp() | Missing |
| GAP-015 | Medium | Injection | system(tar) → libarchive + Sandbox | Missing |
| GAP-016 | High | gRPC | InsecureServerCredentials ohne Production-Guard | Missing |
| GAP-017 | High | TLS | MQTT verify_none ohne CRITICAL | Missing |
| GAP-018 | Medium | Info-Leak | e.what() in HTTP 500 → generisch | Missing |
| GAP-019 | Medium | RNG | mt19937 für Export-IDs → CSPRNG | Missing |
| GAP-020 | Medium | DoS | Unbegrenzte JSON-Array-Batch-Größen | Missing |

**Statistik:**
- Critical: 2 (GAP-001, GAP-002)
- High: 8 (GAP-003, GAP-004, GAP-008, GAP-009, GAP-010, GAP-016, GAP-017)
- Medium: 9 (GAP-005, GAP-006, GAP-011..015, GAP-018..020)
- Low: 1 (GAP-007)

---

## 21. Deep Dive – Runde 4

**Analysedatum:** 2026-04-21 (Runde 4)
**Fokus:** Query-Resource-Limit-Bypass, pugixml XXE, AQL Komplexitätsgrenzen

---

### 21.1 Query-Traversal: `max_frontier_size` / `max_results` ohne Server-seitiges Cap

**Datei:** `src/server/query_api_handler.cpp:531–532`

```cpp
size_t max_frontier_size = body.contains("max_frontier_size")
    ? body["max_frontier_size"].get<size_t>() : 100000;   // kein Server-Cap!
size_t max_results = body.contains("max_results")
    ? body["max_results"].get<size_t>() : 10000;          // kein Server-Cap!
```

Die Standard-Defaults (100.000 / 10.000) schützen nur wenn der User die Felder weglässt.
Ein authentifizierter Nutzer kann `{"max_frontier_size": 999999999}` senden und dadurch
eine Traversal mit massivem Speicher- und CPU-Bedarf auslösen (DoS-Amplifikation).

**Schwere:** **High** – Query-DoS ohne Admin-Rechte erforderlich

**Empfehlung:**
```cpp
static constexpr size_t kServerMaxFrontier = 500'000;
max_frontier_size = std::min(max_frontier_size, kServerMaxFrontier);
max_results = std::min(max_results, size_t{100'000});
```

---

### 21.2 Query-Memory-Limit: `max_memory_bytes=0` deaktiviert den Check

**Datei:** `src/server/query_api_handler.cpp:537`, `src/query/aql_runner.cpp:826`

```cpp
// query_api_handler.cpp: default 0 → unlimited
resource_limits.max_memory_bytes = body.contains("max_memory_bytes")
    ? body["max_memory_bytes"].get<size_t>() : 0;

// aql_runner.cpp:826 – check nur wenn > 0
if (limits.max_memory_bytes > 0) { /* enforce */ }
```

Ein Nutzer kann explizit `{"max_memory_bytes": 0}` senden und damit das Memory-Limit
vollständig deaktivieren. Da auch der Default 0 ist, gilt für alle Queries, die den
Parameter weglassen: **kein Memory-Limit**.

**Schwere:** **High** – Jede AQL-Query kann unbegrenzten Heap-Speicher verbrauchen

**Empfehlung:**
```cpp
static constexpr size_t kDefaultMaxMemoryBytes = 256 * 1024 * 1024; // 256 MB
if (resource_limits.max_memory_bytes == 0) {
    resource_limits.max_memory_bytes = kDefaultMaxMemoryBytes;
}
```

---

### 21.3 Positiv: pugixml ist XXE-sicher by Design

`src/auth/saml_authenticator.cpp` verwendet `pugixml` (`load_string()` mit Default-Flags).
pugixml unterstützt **keine** DTD/External-Entity-Auflösung by design; der Parser verarbeitet
DOCTYPE-Deklarationen nicht. XXE-Angriffe auf den SAML-Parser sind daher nicht möglich.

---

### 22. Ergänzte GAPs (Runde 4)

| GAP ID | Schwere | Bereich | Maßnahme | Datei |
|---|---|---|---|---|
| GAP-021 | High | DoS | `max_frontier_size` / `max_results` Server-seitigen Cap hinzufügen | `src/server/query_api_handler.cpp:531` |
| GAP-022 | High | DoS | `max_memory_bytes=0` → server-seitiger Default 256 MB | `src/server/query_api_handler.cpp:537`, `aql_runner.cpp:826` |

---

### 23. Finale Gesamtstatistik (alle 4 Runden)

| Kategorie | Anzahl |
|---|---|
| **Critical** | 2 |
| **High** | 10 |
| **Medium** | 9 |
| **Low** | 1 |
| **Gesamt GAPs** | **22** |

**Abgedeckte Bereiche:**
- Authentication & Authorization (GAP-001, GAP-003, GAP-007, GAP-013)
- Cryptography (GAP-002, GAP-005, GAP-008)
- Input Validation & Injection (GAP-004, GAP-006, GAP-014)
- Network Security / TLS (GAP-002, GAP-016, GAP-017)
- Path & Resource Security (GAP-009, GAP-010, GAP-015, GAP-020, GAP-021, GAP-022)
- Information Leakage (GAP-011, GAP-018)
- CORS / Browser Security (GAP-012)
- Random Number Generation (GAP-019)
- Shell Command Injection (GAP-014, GAP-015)
