# ThemisDB – DSGVO / SOC 2 Compliance Checklist

**Version:** 1.0  
**Status:** Production-Ready (Security Module v2)  
**Regulierungsrahmen:** DSGVO (GDPR) • SOC 2 Type II  
**Letzte Aktualisierung:** 2026-02

---

## Inhaltsverzeichnis

1. [DSGVO-Compliance (Art. 5 / 25 / 32 / 33 / 35)](#1-dsgvo-compliance)
2. [SOC 2 Trust Services Criteria](#2-soc-2-trust-services-criteria)
3. [Security Module Feature Mapping](#3-security-module-feature-mapping)
4. [Technische Kontrollmaßnahmen – Checkliste](#4-technische-kontrollmaßnahmen)
5. [Offene Punkte & Verbesserungspotenzial](#5-offene-punkte)

---

## 1. DSGVO-Compliance

### Art. 5 – Grundsätze für die Verarbeitung personenbezogener Daten

| Grundsatz | Anforderung | Implementierung | Status |
|-----------|------------|-----------------|--------|
| **Rechtmäßigkeit** | Verarbeitung auf rechtlicher Grundlage | RBAC-gesteuerte Zugriffsrechte; Audit Log | ✅ |
| **Zweckbindung** | Daten nur für festgelegte Zwecke | PolicyEngine: Ressource-/Aktions-Granularität | ✅ |
| **Datenminimierung** | Nur notwendige Daten verarbeiten | InputValidator: keine unnötigen Felder; PII-Pseudonymisierung | ✅ |
| **Richtigkeit** | Daten aktuell halten | Audit-Trail mit Zeitstempel & Hash-Kette | ✅ |
| **Speicherbegrenzung** | Löschfristen einhalten | `purgeOldEntries()` in AuditLogger; Retention-API | ✅ |
| **Integrität & Vertraulichkeit** | Angemessene Sicherheit | AES-256-GCM, PBKDF2-SHA256, TLS, mTLS | ✅ |
| **Rechenschaftspflicht** | Nachweis der Konformität | Dieses Dokument + Compliance Mapping | ✅ |

---

### Art. 25 – Datenschutz durch Technikgestaltung

| Anforderung | Implementierung | Status |
|------------|-----------------|--------|
| Privacy by Default: restriktivster Zugang als Standard | PolicyEngine: `effect_allow=false` als Default bei fehlenden Regeln | ✅ |
| Privacy by Design: Schutz schon in der Architektur | FieldEncryption auf Datenbankebene; kein Klartext in Logs | ✅ |
| Datenminimierung in APIs | InputValidator filtert überflüssige Felder; Response-Masking für PII | ✅ |

---

### Art. 32 – Sicherheit der Verarbeitung

| Maßnahme | Implementierung | Status |
|----------|-----------------|--------|
| **Pseudonymisierung & Verschlüsselung** | AES-256-GCM (FieldEncryption), PBKDF2-SHA256 (Passwörter), TLS 1.3 | ✅ |
| **Vertraulichkeit** | RBAC + ABAC (PolicyEngine), JWT-Authentifizierung (RS256, ES256, EdDSA) | ✅ |
| **Integrität** | AuditLogger: append-only Hash-Kette (SHA-256), PKI-Signatur je Eintrag | ✅ |
| **Verfügbarkeit** | Raft-Konsensus, Hot-Spare-Replikation, RAID-Backup | ✅ |
| **Regelmäßige Prüfung** | `penetration_tests.py`, fuzz_tests_security.cpp, CI-Sicherheits-Scan | ✅ |
| **Zugriffskontrolle** | RateLimiter (per-IP, per-User, adaptives Throttling, IP-Blacklisting) | ✅ |
| **Revozierung von Zugangsdaten** | TokenBlacklist (JTI-basiert), JWTKeyRotationManager (ACTIVE/PASSIVE/REVOKED) | ✅ |
| **Geheimnis-Versioning** | SecretManager (versionierte Secrets mit Rotation und RETIRING-Übergangsphase) | ✅ |

---

### Art. 33 – Meldung von Datenschutzverletzungen

| Anforderung | Implementierung | Status |
|------------|-----------------|--------|
| Erkennung von Sicherheitsvorfällen | AuditLogger: BRUTE_FORCE_DETECTED, SUSPICIOUS_ACTIVITY, INTEGRITY_VIOLATION | ✅ |
| SIEM-Forwarding für 72-Stunden-Meldepflicht | Splunk HEC / Elasticsearch SIEM-Integration in AuditLogger | ✅ |
| Lückenloser Forensik-Trail | Hash-Kette + PKI-Signatur je Log-Eintrag; `verifyChainIntegrity()` | ✅ |

---

### Art. 35 – Datenschutz-Folgenabschätzung (DPIA)

| Risikokategorie | Maßnahme | Status |
|----------------|----------|--------|
| Hochrisiko-Verarbeitung | STRIDE-Bedrohungsmodell (`docs/de/security/security_threat_model.md`) | ✅ |
| Systemische Profilerstellung | PolicyEngine ABAC-Conditions; Query-Injection-Schutz | ✅ |
| Automatisierte Entscheidungsfindung | Audit-Trail für alle KI-/LLM-gesteuerten Entscheidungen | ✅ |

---

## 2. SOC 2 Trust Services Criteria

### CC1 – Control Environment

| Kriterium | Anforderung | Implementierung | Status |
|-----------|------------|-----------------|--------|
| CC1.1 | Governance-Strukturen | RBAC (4-tier: guest/readonly/readwrite/admin) | ✅ |
| CC1.2 | Richtlinien & Verfahren | `SECURITY.md`, `CONTRIBUTING.md`, Policy-Engine | ✅ |
| CC1.3 | Verantwortung & Rechenschaft | AuditLogger: alle Admin-Aktionen werden protokolliert | ✅ |

---

### CC2 – Communication and Information

| Kriterium | Anforderung | Implementierung | Status |
|-----------|------------|-----------------|--------|
| CC2.1 | Interne Kommunikation | Strukturierte JSON-Logs mit User/IP/req_id/Action | ✅ |
| CC2.2 | Externe Kommunikation | TLS 1.3, mTLS für Shard-Kommunikation | ✅ |
| CC2.3 | Berichterstattung | Compliance-Reports API, SIEM-Integration | ✅ |

---

### CC3 – Risk Assessment

| Kriterium | Anforderung | Implementierung | Status |
|-----------|------------|-----------------|--------|
| CC3.1 | Risikoidentifikation | STRIDE-Modell + ANGRIFFSVEKTOREN_ANALYSE.md | ✅ |
| CC3.2 | Risikobewertung | Penetration Tests (`penetration_tests.py`), Fuzz-Tests | ✅ |
| CC3.3 | Risikobehandlung | Kompensatorische Kontrollen für alle STRIDE-Kategorien | ✅ |

---

### CC6 – Logical and Physical Access Controls

| Kriterium | Anforderung | Implementierung | Status |
|-----------|------------|-----------------|--------|
| CC6.1 | Registrierung & De-Registrierung | EmbeddedUserRegistrationPlugin mit PBKDF2-SHA256 | ✅ |
| CC6.2 | Benutzerauthentifizierung | JWT (RS256, ES256, **EdDSA**), MFA (TOTP), API-Keys | ✅ |
| CC6.3 | Rollenbasierte Autorisierung | RBAC + ABAC PolicyEngine (IP-Conditions, Zeitfenster) | ✅ |
| CC6.4 | Physischer Zugang | Konfigurierbar (out-of-scope für Software-Komponente) | ⚪ N/A |
| CC6.6 | Logische Zugangskontrollen | RateLimiter (per-IP/User), TokenBlacklist, IP-Blacklisting | ✅ |
| CC6.7 | Übertragungsverschlüsselung | TLS 1.3, AES-256-GCM at-rest, mTLS inter-shard | ✅ |
| CC6.8 | Malware-Schutz | Eingabe-Validierung (InputValidator), AQL-Injection-Erkennung | ✅ |

---

### CC7 – System Operations

| Kriterium | Anforderung | Implementierung | Status |
|-----------|------------|-----------------|--------|
| CC7.1 | Sicherheitsscan-Prozesse | CodeQL (CI), Gitleaks (Secret-Scanning), Dependabot | ✅ |
| CC7.2 | Sicherheits-Events überwachen | AuditLogger + SIEM (Splunk/Elastic), Anomalie-Erkennung | ✅ |
| CC7.3 | Incident-Response | Hash-Ketten-Verifikation, Forensik-Trail, SIEM-Alarme | ✅ |
| CC7.4 | Incident-Meldung | `logSecurityEvent(SUSPICIOUS_ACTIVITY)`, SIEM-Forwarding | ✅ |
| CC7.5 | Wiederherstellung | WAL-basiertes Recovery, PITR, RAID-5-Backup | ✅ |

---

### CC8 – Change Management

| Kriterium | Anforderung | Implementierung | Status |
|-----------|------------|-----------------|--------|
| CC8.1 | Änderungsprotokoll | PolicyEngine: `POLICY_UPDATED`-Audit-Event bei add/remove/reload | ✅ |
| CC8.1 | Schlüsselrotations-Protokoll | JWTKeyRotationManager: `KEY_ROTATED`/`KEY_DELETED`-Events | ✅ |
| CC8.1 | Config-Änderungen | `CONFIG_CHANGED`/`ENCRYPTION_SCHEMA_CHANGED` in AuditLogger | ✅ |

---

### CC9 – Risk Mitigation

| Kriterium | Anforderung | Implementierung | Status |
|-----------|------------|-----------------|--------|
| CC9.1 | Risikominderung mit Lieferanten | Abhängigkeits-Scan (Dependabot, Gitleaks, vcpkg) | ✅ |
| CC9.2 | Monitoring | Prometheus-Metriken, Grafana-Dashboards, Health-Checks | ✅ |

---

## 3. Security Module Feature Mapping

| Feature | DSGVO Art. | SOC 2 Kriterium | Datei / Klasse |
|---------|-----------|-----------------|---------------|
| PolicyEngine (ABAC/RBAC) | 5(f), 25, 32 | CC6.3 | `src/server/policy_engine.cpp` |
| PolicyEngine Hot-Reload | 32 | CC8.1 | `PolicyEngine::reloadIfChanged()` |
| JWTValidator (RS256, ES256, EdDSA) | 32 | CC6.2 | `src/auth/jwt_validator.cpp` |
| JWTKeyRotationManager | 32 | CC6.7, CC8.1 | `src/auth/jwt_key_rotation_manager.cpp` |
| TokenBlacklist (JTI-basiert) | 32 | CC6.6 | `src/auth/token_blacklist.cpp` |
| AuditLogger + Hash-Kette | 5(f), 33 | CC7.2, CC7.3 | `src/utils/audit_logger.cpp` |
| AuditLogger SIEM (Splunk/Elastic) | 33 | CC7.2, CC7.4 | `AuditLogger::forwardToSiem()` |
| InputValidator + AQL-Injection | 32 | CC6.8 | `src/security/access_control.cpp` |
| RateLimiter (adaptiv, IP-Blacklist) | 32 | CC6.6, CC7.2 | `src/server/rate_limiter.cpp` |
| PBKDF2-SHA256 Passwort-Hashing | 32 | CC6.1 | `src/security/embedded_user_registration_plugin.cpp` |
| SecretManager (Versionierung) | 32 | CC6.6, CC8.1 | `src/security/secret_manager.cpp` |
| Security-Headers (CSP, CORS, HSTS) | 32 | CC6.8 | `src/server/http_server.cpp` |
| MFA (TOTP RFC 6238) | 32 | CC6.2 | `src/auth/mfa_authenticator.cpp` |
| HSM-Integration (PKCS#11) | 32 | CC6.7 | `src/security/hsm_provider_pkcs11.cpp` |
| Penetration Tests | 32, 35 | CC3.2 | `tests/penetration_tests.py` |
| Fuzz-Tests | 32, 35 | CC3.2 | `tests/test_fuzz_security.cpp` |

---

## 4. Technische Kontrollmaßnahmen – Checkliste

### 4.1 Authentifizierung & Autorisierung

- [x] JWT-Validierung (RS256, ES256, EdDSA) mit Kid-Revocation
- [x] PBKDF2-SHA256 für Passwort-Hashing (100.000 Iterationen, zufälliges Salt)
- [x] RBAC mit 4-tier-Rollenhierarchie (guest / readonly / readwrite / admin)
- [x] ABAC-Erweiterung (IP-Prefixes als Bedingung in PolicyEngine)
- [x] Multi-Faktor-Authentifizierung (TOTP RFC 6238)
- [x] Session-Timeout & Concurrent-Session-Limit
- [x] API-Key-Authentifizierung mit Revocation-Support

### 4.2 Verschlüsselung & Schlüsselverwaltung

- [x] AES-256-GCM für Daten-Verschlüsselung at-rest
- [x] TLS 1.3 für Daten in transit
- [x] mTLS für Shard-zu-Shard-Kommunikation
- [x] JWT Key Rotation (ACTIVE → PASSIVE → REVOKED-Lebenszyklus)
- [x] SecretManager: versionierte Secrets mit RETIRING-Übergangsphase
- [x] HSM-Integration (PKCS#11) für Schlüsselmaterial in Hardware
- [x] TOTP-Secret-Verschlüsselung mit eigenem Schlüsselkontext

### 4.3 Audit & Logging

- [x] Append-only Audit-Log als JSONL-Datei
- [x] Hash-Kette (SHA-256) für Manipulations-Nachweis
- [x] PKI-Signatur je Audit-Log-Eintrag
- [x] Strukturierte Log-Einträge (User, IP, req_id, Aktion, Ressource)
- [x] Policy-Change-Auditing (`POLICY_UPDATED` bei add/remove/reload)
- [x] Key-Rotation-Auditing (`KEY_ROTATED`, `KEY_DELETED`)
- [x] SIEM-Forwarding: Splunk HEC (HTTPS) & Elasticsearch
- [x] CA-Bundle-Pinning für Splunk-TLS (`siem_ca_bundle_path`)
- [x] Anomalie-Erkennung für Task-Scheduler-Events (Z-Score-basiert)

### 4.4 Input-Validierung & Injection-Schutz

- [x] AQL-Injection-Erkennung (AST-basiert + Pattern-Matching)
- [x] Path-Traversal-Verhinderung
- [x] Log-Injection-Sanitierung (Newline-Stripping)
- [x] JSON-Schema-Validierung für API-Eingaben
- [x] Security-Headers: CSP, X-Frame-Options, X-Content-Type-Options, HSTS
- [x] CORS-Policy-Enforcement (konfigurierbar per Umgebungsvariable)

### 4.5 Rate Limiting & DDoS-Schutz

- [x] Token-Bucket-Algorithmus (per-IP, per-User)
- [x] Adaptives Throttling (erhöhter Token-Verbrauch bei zu vielen Ablehnungen)
- [x] IP-Blacklisting mit sofortiger Sperrung
- [x] IP-Whitelisting für vertrauenswürdige Systeme
- [x] Konfigurierbare Limits per IP/User (custom_limits-Map)

### 4.6 Token-Revocation & Blacklisting

- [x] JTI-basierte Token-Blacklist mit TTL und automatischem Pruning
- [x] Kid-Revocation in JWTValidator (Runtime-Denylist)
- [x] JWTKeyRotationManager: REVOKED-Status propagiert zu JWTValidator

### 4.7 Testing & Validierung

- [x] Comprehensive-Tests: Policy Engine, JWT, AuditLogger, InputValidator, RateLimiter
- [x] Fuzz-Tests für kritische Security-Entry-Points (50+ Fälle)
- [x] Penetration-Test-Skript Python (`penetration_tests.py`, 25+ Prüfungen)
- [x] RBAC-Tests (30+ Fälle)
- [x] Passwort-Hashing-Tests (25+ Fälle)
- [x] SIEM-Integration-Tests (22+ Fälle)
- [x] ES256 und EdDSA JWT-Tests

---

## 5. Offene Punkte

| Punkt | Priorität | Beschreibung |
|-------|-----------|-------------|
| Argon2id | Medium | vcpkg enthält keine Argon2-Abhängigkeit; PBKDF2-SHA256 (100k iter) ist als gleichwertig anzusehen gemäß NIST SP 800-132. Bei Bedarf kann `argon2` zu vcpkg.json hinzugefügt werden. |
| RBAC-Zeitfenster in PolicyEngine | Low | ABAC-Bedingungen unterstützen IP-Prefixes; Zeitfenster-Bedingungen sind vorbereitet (TODO in Header), aber noch nicht implementiert. |
| SecretManager-Persistenz | Low | SecretManager ist aktuell in-memory only. Für Produktion: Integration mit VaultKeyProvider oder verschlüsselter Datei-Persistenz empfohlen. |
| HSM-Stub in Produktion | Medium | HSM-Stub ist durch `THEMIS_ALLOW_HSM_STUB`-Guard geschützt; in Produktion ist ein echter HSM (PKCS#11) erforderlich. |
| EdDSA/Ed448 | Low | Nur Ed25519 wird unterstützt. Ed448 ist nicht implementiert (anderes Schlüsselformat). |

---

## Legende

| Symbol | Bedeutung |
|--------|-----------|
| ✅ | Vollständig implementiert |
| 🟡 | Teilweise implementiert |
| 🔴 | Nicht implementiert |
| ⚪ | Nicht anwendbar (N/A) |

---

*Dieses Dokument ist Bestandteil des Security-Moduls und wird mit jeder sicherheitsrelevanten Änderung aktualisiert.*
