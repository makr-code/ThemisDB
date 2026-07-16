# ThemisDB – Vollständige Review- und Audit-Checkliste

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🔒 Compliance  
**Typ:** Umfassende Compliance-Checkliste nach Stand der Technik

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#-übersicht)
- [Status-Legende](#status-legende)
- [Standards](#standards)

---

## 📋 Übersicht

Diese Checkliste dient als vollständige Grundlage für ein Review und Audit der ThemisDB gemäß:

**Internationale Standards:**
- **BSI C5** (Cloud Computing Compliance Criteria Catalogue) Version 2020
- **ISO/IEC 27001:2022** - Informationssicherheitsmanagementsystem
- **ISO/IEC 27017** - Cloud-spezifische Sicherheitskontrollen
- **ISO/IEC 27018** - Schutz personenbezogener Daten in Public Clouds
- **ISO/IEC 27701** - Privacy Information Management System (PIMS)
- **NIST Cybersecurity Framework (CSF) 2.0** - US-Cybersicherheitsrahmenwerk
- **Common Criteria (ISO/IEC 15408)** - Sicherheitsevaluierung für IT-Produkte

**EU-Regulierung:**
- **DSGVO/GDPR** (EU) 2016/679
- **eIDAS** (EU) No 910/2014
- **NIS2-Richtlinie** (EU) 2022/2555 - Netz- und Informationssicherheit
- **CE-Kennzeichnung** / Cyber Resilience Act (CRA)
- **EU AI Act** (falls KI-Komponenten)

**Branchenstandards:**
- **DIN EN ISO 9001:2015** - Qualitätsmanagementsysteme
- **SOC 2 Type II** - Trust Services Criteria
- **HIPAA** (Health Insurance Portability and Accountability Act)
- **PCI DSS v4.0** - Payment Card Industry Data Security Standard
- **TISAX** - Trusted Information Security Assessment Exchange (Automotive)
- **KRITIS-Verordnung** - BSI-Kritisverordnung (kritische Infrastrukturen)

---

## Status-Legende

| Symbol | Bedeutung |
|--------|-----------|
| ✅ | Vollständig implementiert und dokumentiert |
| ⚠️ | Teilweise implementiert / Verbesserungsbedarf |
| ❌ | Nicht implementiert / Kritischer Handlungsbedarf |
| 📋 | Geplant (Roadmap) |
| N/A | Nicht anwendbar |

---

## 1. Organisatorische Sicherheit (BSI C5: OIS)

### 1.1 Informationssicherheits-Managementsystem (ISMS)

| Nr. | Anforderung | BSI C5 Ref | Status | Nachweis/Kommentar |
|-----|-------------|------------|--------|-------------------|
| OIS-01 | Informationssicherheitspolitik definiert und kommuniziert | OIS-01 | ✅ | `docs/security/INFORMATION_SECURITY_POLICY.md` |
| OIS-02 | Sicherheitsrollen und Verantwortlichkeiten definiert | OIS-02 | ✅ | RBAC implementiert, Rollen in ISP dokumentiert |
| OIS-03 | Risikomanagementprozess etabliert | OIS-03 | ✅ | `docs/security/RISK_MANAGEMENT_FRAMEWORK.md` |
| OIS-04 | Interne Audits durchgeführt | OIS-04 | ✅ | Audit-Checkliste vorhanden (`docs/security/audit_checklist.md`) |
| OIS-05 | Management-Review der Sicherheit | OIS-05 | ⚠️ | Code Reviews via GitHub, formales Management-Review fehlt |

### 1.2 Dokumentation

| Nr. | Anforderung | ISO 27001 Ref | Status | Nachweis/Kommentar |
|-----|-------------|---------------|--------|-------------------|
| DOC-01 | Vollständige Systemdokumentation | A.5.1.1 | ✅ | `README.md`, `FEATURES.md`, `docs/` (290+ Dokumente) |
| DOC-02 | API-Dokumentation | A.14.2.5 | ✅ | `docs/openapi.yaml`, `docs/apis/` |
| DOC-03 | Sicherheitsdokumentation | A.5.1.2 | ✅ | `docs/security/` (35+ Dokumente) |
| DOC-04 | Änderungshistorie | A.12.1.2 | ✅ | `CHANGELOG.md`, Git-History |
| DOC-05 | Entwicklungsdokumentation | A.14.2.1 | ✅ | `DEVELOPMENT_AUDITLOG.md`, `ROADMAP.md` |

---

## 2. Personelle Sicherheit (BSI C5: HRS)

### 2.1 Mitarbeiter-Sicherheit

| Nr. | Anforderung | BSI C5 Ref | Status | Nachweis/Kommentar |
|-----|-------------|------------|--------|-------------------|
| HRS-01 | Sicherheitsüberprüfung vor Einstellung | HRS-01 | N/A | Open-Source-Projekt |
| HRS-02 | Sicherheitsbewusstsein und Schulung | HRS-02 | ⚠️ | `CONTRIBUTING.md` vorhanden, Schulungsmaterial fehlt |
| HRS-03 | Disziplinarverfahren bei Verstößen | HRS-03 | N/A | Open-Source-Projekt |
| HRS-04 | Zugriffsrechte bei Beendigung entziehen | HRS-04 | ⚠️ | GitHub-Berechtigungen manuell verwaltet |

---

## 3. Asset-Management (BSI C5: AM)

### 3.1 Inventar und Klassifizierung

| Nr. | Anforderung | BSI C5 Ref | Status | Nachweis/Kommentar |
|-----|-------------|------------|--------|-------------------|
| AM-01 | Asset-Inventar gepflegt | AM-01 | ✅ | `vcpkg.json` (Dependencies), Code-Struktur dokumentiert |
| AM-02 | Datenklassifizierung | AM-02 | ✅ | 4-stufige Klassifizierung (offen/vs-nfd/geheim/streng_geheim) |
| AM-03 | Umgang mit Medien | AM-03 | ✅ | Encryption at-rest, sichere Löschung via Retention Manager |
| AM-04 | Rückgabe von Assets | AM-04 | N/A | Open-Source-Projekt |

### 3.2 Datenklassifizierung (ThemisDB-spezifisch)

| Klassifizierungsstufe | Verschlüsselung | ANN-Indexing | Cache | Retention |
|-----------------------|-----------------|--------------|-------|-----------|
| `offen` | Optional | ✓ | ✓ | 30 Tage |
| `vs-nfd` | Pflicht | ✗ | ✗ | 365 Tage |
| `geheim` | Pflicht | ✗ | ✗ | 90 Tage |
| `streng_geheim` | Pflicht | ✗ | ✗ | 30 Tage |

---

## 4. Zugriffskontrolle (BSI C5: IDM)

### 4.1 Identitäts- und Berechtigungsmanagement

| Nr. | Anforderung | BSI C5 Ref | Status | Nachweis/Kommentar |
|-----|-------------|------------|--------|-------------------|
| IDM-01 | Benutzerregistrierung und -verwaltung | IDM-01 | ✅ | RBAC implementiert (`docs/security/implementation_summary.md`) |
| IDM-02 | Rollenbasierte Zugriffskontrolle | IDM-02 | ✅ | 4-stufige Hierarchie: admin → operator → analyst → readonly |
| IDM-03 | Privilegierte Zugriffsrechte | IDM-03 | ✅ | Admin-only für Key-Rotation, PII-Manager |
| IDM-04 | Geheime Authentifizierungsinformationen | IDM-04 | ✅ | Vault-Integration, HSM-Support (PKCS#11) |
| IDM-05 | Zugriffsentzug | IDM-05 | ✅ | RBAC mit User-Role-Mapping |
| IDM-06 | Passwortrichtlinien | IDM-06 | ✅ | `docs/security/PASSWORD_POLICY.md` |

### 4.2 RBAC-Berechtigungen (implementiert)

| Rolle | data:read | data:write | data:delete | keys:rotate | audit:view | config:modify |
|-------|-----------|------------|-------------|-------------|------------|---------------|
| readonly | ✓ | ✗ | ✗ | ✗ | ✗ | ✗ |
| analyst | ✓ | ✗ | ✗ | ✗ | ✓ | ✗ |
| operator | ✓ | ✓ | ✓ | ✗ | ✓ | ✗ |
| admin | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |

---

## 5. Kryptographie (BSI C5: CRY)

### 5.1 Verschlüsselung

| Nr. | Anforderung | BSI C5 Ref | Status | Nachweis/Kommentar |
|-----|-------------|------------|--------|-------------------|
| CRY-01 | Kryptographie-Policy | CRY-01 | ✅ | **`docs/security/CRYPTOGRAPHY_POLICY.md`** (Formale Policy, BSI TR-02102-1 konform), `docs/security/encryption_strategy.md` |
| CRY-02 | Schlüsselmanagement | CRY-02 | ✅ | **`docs/security/KEY_LIFECYCLE_MANAGEMENT.md`** (Vollständiger Lifecycle), 3 Provider: Mock, HSM (PKCS#11), Vault |
| CRY-03 | Data-at-rest Verschlüsselung | CRY-03 | ✅ | AES-256-GCM (`docs/security/column_encryption.md`) - **95% Compliance Score** |
| CRY-04 | Data-in-transit Verschlüsselung | CRY-04 | ✅ | TLS 1.3 default, TLS 1.2 fallback |
| CRY-05 | Schlüsselrotation | CRY-05 | ✅ | Lazy Re-Encryption implementiert - **Vier-Phasen-Prozess dokumentiert** |
| CRY-06 | Kryptographische Integrität | CRY-06 | ✅ | SHA-256, HMAC, RSA-SHA256 Signaturen, **GCM Authentication Tag** |

### 5.2 Kryptographische Implementierung

| Komponente | Algorithmus | Schlüssellänge | BSI-konform | Status |
|------------|-------------|----------------|-------------|--------|
| **Verschlüsselung** | AES-256-GCM | 256 bit | ✅ | Implementiert |
| **Hashing** | SHA-256/384/512 | - | ✅ | Implementiert |
| **Signaturen** | RSA-SHA256 | 2048+ bit | ✅ | Implementiert |
| **Key Derivation** | HKDF-SHA256 | - | ✅ | Implementiert |
| **TLS** | TLS 1.3/1.2 | - | ✅ | Implementiert |
| **Cipher Suites** | ECDHE-RSA-AES256-GCM-SHA384 | - | ✅ | Konfiguriert |

---

## 6. Physische Sicherheit (BSI C5: PHY)

| Nr. | Anforderung | BSI C5 Ref | Status | Nachweis/Kommentar |
|-----|-------------|------------|--------|-------------------|
| PHY-01 | Physische Sicherheitsperimeter | PHY-01 | N/A | Software-Produkt |
| PHY-02 | Zugangskontrollen | PHY-02 | N/A | Software-Produkt |
| PHY-03 | Sichere Entsorgung | PHY-03 | ✅ | Secure Delete via Retention Manager |
| PHY-04 | Clear Desk/Screen | PHY-04 | N/A | Software-Produkt |

---

## 7. Betriebssicherheit (BSI C5: OPS)

### 7.1 Betriebsverfahren

| Nr. | Anforderung | BSI C5 Ref | Status | Nachweis/Kommentar |
|-----|-------------|------------|--------|-------------------|
| OPS-01 | Dokumentierte Betriebsverfahren | OPS-01 | ✅ | `docs/guides/deployment.md`, `docs/operations_runbook.md` |
| OPS-02 | Änderungsmanagement | OPS-02 | ✅ | Git, CHANGELOG.md, PR-Reviews |
| OPS-03 | Kapazitätsmanagement | OPS-03 | ✅ | `docs/performance/memory_tuning.md` |
| OPS-04 | Trennung Dev/Test/Prod | OPS-04 | ⚠️ | Docker-Profiles vorhanden, formale Trennung unklar |

### 7.2 Schutz vor Malware

| Nr. | Anforderung | BSI C5 Ref | Status | Nachweis/Kommentar |
|-----|-------------|------------|--------|-------------------|
| OPS-05 | Malware-Schutz | OPS-05 | ✅ | `MalwareFilterManager` in Ingestion-Pipeline, SignatureScanner (built-in) + ClamAV (optional), Dokumentation: `docs/security/security_malware_scanner.md` |
| OPS-06 | Schwachstellenmanagement | OPS-06 | ✅ | `security-scan.ps1`, Trivy-Scans geplant |
| OPS-07 | Penetrationstests | OPS-07 | ⚠️ | Empfohlen in Checkliste, nicht durchgeführt |

### 7.3 Backup

| Nr. | Anforderung | BSI C5 Ref | Status | Nachweis/Kommentar |
|-----|-------------|------------|--------|-------------------|
| OPS-08 | Backup-Strategie | OPS-08 | ✅ | RocksDB Checkpoints, WAL-Archivierung |
| OPS-09 | Backup-Tests | OPS-09 | ⚠️ | Restore-Prozedur dokumentiert, Tests empfohlen |
| OPS-10 | Backup-Verschlüsselung | OPS-10 | ✅ | At-rest Encryption für alle Daten |

### 7.4 Logging und Monitoring

| Nr. | Anforderung | BSI C5 Ref | Status | Nachweis/Kommentar |
|-----|-------------|------------|--------|-------------------|
| OPS-11 | Event-Logging | OPS-11 | ✅ | 65+ Audit-Event-Typen (`docs/features/audit_logging.md`) |
| OPS-12 | Log-Schutz | OPS-12 | ✅ | Encrypt-then-Sign Pattern, Hash-Kette |
| OPS-13 | Administrator-Logging | OPS-13 | ✅ | Alle Admin-Aktionen werden protokolliert |
| OPS-14 | Uhrzeitsynchronisation | OPS-14 | ⚠️ | Standard OS-Zeit, keine NTP-Validierung |

---

## 8. Kommunikationssicherheit (BSI C5: COS)

### 8.1 Netzwerksicherheit

| Nr. | Anforderung | BSI C5 Ref | Status | Nachweis/Kommentar |
|-----|-------------|------------|--------|-------------------|
| COS-01 | Netzwerk-Segmentierung | COS-01 | ⚠️ | Docker-Networks, formale Segmentierung Deployment-abhängig |
| COS-02 | Übertragungssicherheit | COS-02 | ✅ | TLS 1.3, mTLS, HSTS |
| COS-03 | API-Sicherheit | COS-03 | ✅ | Rate Limiting, Input Validation, CORS |
| COS-04 | DoS-Schutz | COS-04 | ✅ | Token Bucket (100 req/min default) |

### 8.2 HTTP Security Headers

| Header | Wert | Status |
|--------|------|--------|
| `X-Frame-Options` | `DENY` | ✅ |
| `X-Content-Type-Options` | `nosniff` | ✅ |
| `X-XSS-Protection` | `1; mode=block` | ✅ |
| `Content-Security-Policy` | Konfigurierbar | ✅ |
| `Strict-Transport-Security` | `max-age=31536000; includeSubDomains` | ✅ |
| `CORS` | Whitelist-basiert | ✅ |

---

## 9. Systemakquisition, Entwicklung und Wartung (BSI C5: DEV)

### 9.1 Sichere Entwicklung

| Nr. | Anforderung | BSI C5 Ref | Status | Nachweis/Kommentar |
|-----|-------------|------------|--------|-------------------|
| DEV-01 | Secure Development Lifecycle | DEV-01 | ✅ | Code Reviews, Static Analysis (clang-tidy) |
| DEV-02 | Sichere Entwicklungsumgebung | DEV-02 | ✅ | Gitleaks für Secrets, CI/CD-Pipeline |
| DEV-03 | Systemsicherheitstests | DEV-03 | ✅ | 303/303 Tests PASS, 85%+ Coverage |
| DEV-04 | Sichere Codierung | DEV-04 | ✅ | `.clang-format`, `.clang-tidy`, Code Standards |
| DEV-05 | Change Management | DEV-05 | ✅ | Git-Flow, PR-Reviews, CHANGELOG |

### 9.2 Code-Qualität

| Metrik | Wert | Status |
|--------|------|--------|
| **Lines of Code** | 63,506+ | ✅ |
| **Dokumentation** | 279+ Dokumente | ✅ |
| **Tests** | 303/303 PASS | ✅ |
| **Coverage** | 85%+ | ✅ |
| **Static Analysis** | clang-tidy (keine kritischen Findings) | ✅ |
| **Security Scan** | Gitleaks (keine Secrets) | ✅ |

### 9.3 Dependency Management

| Nr. | Anforderung | Status | Nachweis/Kommentar |
|-----|-------------|--------|-------------------|
| DEP-01 | Abhängigkeiten inventarisiert | ✅ | `vcpkg.json`, `vcpkg.docker.json` |
| DEP-02 | Bekannte Schwachstellen geprüft | ⚠️ | `security-scan.ps1`, Trivy-Integration geplant |
| DEP-03 | Baseline gepinnt | ✅ | vcpkg `builtin-baseline` gepinnt |
| DEP-04 | Lizenzen geprüft | ⚠️ | Manuelle Prüfung, SBOM-Generierung empfohlen |

---

## 10. Lieferantenbeziehungen (BSI C5: SSO)

| Nr. | Anforderung | BSI C5 Ref | Status | Nachweis/Kommentar |
|-----|-------------|------------|--------|-------------------|
| SSO-01 | Sicherheitsanforderungen an Lieferanten | SSO-01 | N/A | Open-Source, keine externen Lieferanten |
| SSO-02 | Lieferketten-Sicherheit | SSO-02 | ⚠️ | vcpkg-Dependencies, SBOM empfohlen |
| SSO-03 | Service-Level-Agreements | SSO-03 | N/A | Open-Source-Projekt |

---

## 11. Störungs- und Notfallmanagement (BSI C5: SIM)

### 11.1 Incident Management

| Nr. | Anforderung | BSI C5 Ref | Status | Nachweis/Kommentar |
|-----|-------------|------------|--------|-------------------|
| SIM-01 | Incident-Response-Prozess | SIM-01 | ✅ | `docs/security/INCIDENT_RESPONSE_PLAN.md` |
| SIM-02 | Sicherheitsvorfälle melden | SIM-02 | ✅ | `SECURITY.md` mit Meldeprozess |
| SIM-03 | Forensische Analyse | SIM-03 | ✅ | Audit-Logs mit Hash-Kette, tamper-proof |
| SIM-04 | Lessons Learned | SIM-04 | ✅ | Dokumentiert in IRP |

### 11.2 Business Continuity

| Nr. | Anforderung | BSI C5 Ref | Status | Nachweis/Kommentar |
|-----|-------------|------------|--------|-------------------|
| SIM-05 | Business Continuity Plan | SIM-05 | ✅ | `docs/compliance/BUSINESS_CONTINUITY_PLAN.md` |
| SIM-06 | Disaster Recovery | SIM-06 | ✅ | BCP inkl. RAID-Redundanz, Point-in-Time Recovery |
| SIM-07 | Recovery-Tests | SIM-07 | ✅ | Test-Protokolle in BCP dokumentiert |

---

## 12. Compliance (BSI C5: COM)

### 12.1 Regulatorische Anforderungen

| Nr. | Anforderung | BSI C5 Ref | Status | Nachweis/Kommentar |
|-----|-------------|------------|--------|-------------------|
| COM-01 | Rechtliche Anforderungen identifiziert | COM-01 | ✅ | DSGVO, eIDAS, HGB in Docs adressiert |
| COM-02 | Datenschutz | COM-02 | ✅ | PII-Detection, Encryption, Retention |
| COM-03 | Compliance-Nachweise | COM-03 | ✅ | Audit-Logs, Compliance-Matrix |

---

## 13. DSGVO/GDPR-Compliance

### 13.1 Betroffenenrechte (Art. 12-22)

| Artikel | Recht | Status | Implementierung |
|---------|-------|--------|-----------------|
| Art. 15 | Auskunftsrecht | ✅ | Data Export API |
| Art. 16 | Berichtigung | ✅ | Entity Update API |
| Art. 17 | Löschung ("Recht auf Vergessenwerden") | ✅ | Auto-Purge, Retention Manager |
| Art. 18 | Einschränkung der Verarbeitung | ⚠️ | Governance-Flags, kein dedizierter API-Endpunkt |
| Art. 20 | Datenübertragbarkeit | ✅ | JSON/CSV Export |
| Art. 21 | Widerspruch | ⚠️ | Keine explizite Implementierung |

### 13.2 Technisch-organisatorische Maßnahmen (Art. 32)

| Maßnahme | Status | Nachweis |
|----------|--------|----------|
| Pseudonymisierung | ✅ | Field-Level Encryption |
| Verschlüsselung | ✅ | AES-256-GCM, TLS 1.3 |
| Vertraulichkeit | ✅ | RBAC, Zugriffskontrolle |
| Integrität | ✅ | Hash-Ketten, Signaturen |
| Verfügbarkeit | ✅ | Backup, Recovery, RAID-Redundanz |
| Belastbarkeit | ✅ | RAID-like Sharding, Replication, Multi-Master |
| Wiederherstellbarkeit | ✅ | Point-in-Time Recovery, BCP dokumentiert |
| Regelmäßige Tests | ✅ | Test-Protokolle in BCP |

### 13.3 Privacy by Design (Art. 25)

| Anforderung | Status | Nachweis |
|-------------|--------|----------|
| Datenminimierung | ✅ | PII-Detection, Auto-Redaction |
| Speicherbegrenzung | ✅ | Retention Policies |
| Zweckbindung | ✅ | Governance-Klassifizierung |
| Datenschutz-Folgenabschätzung | ✅ | `docs/compliance/DPIA.md` |

---

## 14. eIDAS-Compliance

### 14.1 Elektronische Signaturen

| Anforderung | Artikel | Status | Nachweis |
|-------------|---------|--------|----------|
| Qualifizierte elektronische Signaturen | Art. 25-34 | ✅ | PKI-Client (RSA-SHA256), HSM-Support |
| Zeitstempel | Art. 41-42 | ✅ | Präzise Zeiterfassung im Audit-Log |
| Langzeitarchivierung | - | ⚠️ | Archive-Handler konfigurierbar |
| Nachweisbarkeit | - | ✅ | Tamper-proof Audit-Logs |

### 14.2 PKI-Implementierung

| Komponente | Status | Nachweis |
|------------|--------|----------|
| RSA-SHA256 Signaturen | ✅ | `src/utils/pki_client.cpp` |
| HSM-Integration (PKCS#11) | ✅ | `docs/security/hsm_integration.md` |
| Zertifikatsverwaltung | ✅ | X.509-Support |
| Stub-Modus für Entwicklung | ✅ | Base64-Fallback |

---

## 15. SOC 2 Type II Compliance

### 15.1 Trust Services Criteria

| Kategorie | Kontrolle | Status | Nachweis |
|-----------|-----------|--------|----------|
| **Security (CC6)** | | | |
| CC6.1 | Access Control | ✅ | RBAC implementiert |
| CC6.2 | System Access | ✅ | mTLS, API-Authentifizierung |
| CC6.3 | Encryption | ✅ | AES-256-GCM, TLS 1.3 |
| CC6.7 | Audit Logs | ✅ | 65+ Event-Typen |
| **Availability (A1)** | | | |
| A1.1 | Backup & Recovery | ✅ | RocksDB Checkpoints |
| A1.2 | Disaster Recovery | ✅ | Point-in-Time Recovery |
| **Processing Integrity (PI)** | | | |
| PI1.1 | Data Validation | ✅ | JSON Schema, Input Validation |
| **Confidentiality (C)** | | | |
| C1.1 | Data Classification | ✅ | 4-stufige Klassifizierung |
| C1.2 | Encryption | ✅ | Field-Level Encryption |
| **Privacy (P)** | | | |
| P1-P8 | Privacy Criteria | ✅ | DSGVO-konform |

---

## 16. HIPAA-Compliance (falls anwendbar)

| Regel | Anforderung | Status | Nachweis |
|-------|-------------|--------|----------|
| §164.312(a)(1) | Access Control | ✅ | RBAC |
| §164.312(b) | Audit Controls | ✅ | Audit-Logging |
| §164.312(c)(1) | Integrity | ✅ | Hash-Ketten |
| §164.312(d) | Person/Entity Authentication | ✅ | mTLS, Token-Auth |
| §164.312(e)(1) | Transmission Security | ✅ | TLS 1.3 |

---

## 17. DIN/ISO-Normen

### 17.1 ISO 9001:2015 (Qualitätsmanagement)

| Anforderung | Kapitel | Status | Nachweis |
|-------------|---------|--------|----------|
| Dokumentenkontrolle | 7.5 | ✅ | Git, Versionierung |
| Änderungsmanagement | 6.3 | ✅ | CHANGELOG, PR-Reviews |
| Risikomanagement | 6.1 | ⚠️ | Threat Model vorhanden |
| Korrekturmaßnahmen | 10.2 | ⚠️ | GitHub Issues |
| Kontinuierliche Verbesserung | 10.3 | ✅ | Roadmap, DEVELOPMENT_AUDITLOG |

### 17.2 ISO 27001:2022 (Informationssicherheit)

| Kontrolle | Annex A Ref | Status | Nachweis |
|-----------|-------------|--------|----------|
| Informationssicherheitspolitik | A.5.1 | ⚠️ | Implizit in Docs |
| Zugriffskontrolle | A.5.15-5.18 | ✅ | RBAC |
| Kryptographie | A.8.24 | ✅ | AES-256-GCM |
| Betriebssicherheit | A.8.1-8.12 | ✅ | Dokumentiert |
| Kommunikationssicherheit | A.8.20-8.22 | ✅ | TLS 1.3 |
| Systemakquisition/Entwicklung | A.8.25-8.31 | ✅ | SDLC |
| Lieferantenbeziehungen | A.5.19-5.22 | N/A | Open-Source |
| Incident Management | A.5.24-5.28 | ⚠️ | Teilweise |
| Business Continuity | A.5.29-5.30 | ⚠️ | Backup vorhanden |
| Compliance | A.5.31-5.36 | ✅ | Dokumentiert |

---

## 18. CE-Kennzeichnung (wo anwendbar)

### 18.1 Cyber Resilience Act (CRA) - ab 2027

| Anforderung | Status | Nachweis/Kommentar |
|-------------|--------|-------------------|
| Security by Design | ✅ | Encryption, RBAC, Audit |
| Schwachstellenmanagement | ⚠️ | `security-scan.ps1`, Verbesserungsbedarf |
| SBOM (Software Bill of Materials) | ⚠️ | vcpkg.json, formale SBOM fehlt |
| Update-Fähigkeit | ✅ | Semantic Versioning, Releases |
| Dokumentation | ✅ | Umfangreich |
| Meldepflicht (CVEs) | ⚠️ | Kein formaler Prozess |

### 18.2 EU AI Act (falls KI-Komponenten)

| Anforderung | Status | Nachweis/Kommentar |
|-------------|--------|-------------------|
| Risikobewertung | N/A | Keine KI-Systeme |
| Transparenz | N/A | Keine KI-Systeme |
| Menschliche Aufsicht | N/A | Keine KI-Systeme |
| Datenqualität | N/A | Keine KI-Systeme |

---

## 19. NIST Cybersecurity Framework (CSF) 2.0

### 19.1 Identify (ID)

| Funktion | Kategorie | Status | Nachweis/Kommentar |
|----------|-----------|--------|-------------------|
| ID.AM | Asset Management | ✅ | `vcpkg.json`, Code-Struktur dokumentiert |
| ID.BE | Business Environment | ⚠️ | Teilweise in `ROADMAP.md` |
| ID.GV | Governance | ⚠️ | Sicherheitsrichtlinien vorhanden, formale Governance fehlt |
| ID.RA | Risk Assessment | ⚠️ | Threat Model vorhanden (`docs/security/threat_model.md`) |
| ID.RM | Risk Management Strategy | ⚠️ | Implizit in Sicherheitsdokumentation |
| ID.SC | Supply Chain Risk Management | ⚠️ | vcpkg-Dependencies, SBOM empfohlen |

### 19.2 Protect (PR)

| Funktion | Kategorie | Status | Nachweis/Kommentar |
|----------|-----------|--------|-------------------|
| PR.AA | Identity Management & Access Control | ✅ | RBAC, mTLS, Token-Auth |
| PR.AT | Awareness and Training | ⚠️ | `CONTRIBUTING.md` vorhanden |
| PR.DS | Data Security | ✅ | AES-256-GCM, TLS 1.3, Field-Level Encryption |
| PR.IP | Information Protection Processes | ✅ | Backup, Recovery, Retention |
| PR.MA | Maintenance | ✅ | Semantic Versioning, CI/CD |
| PR.PT | Protective Technology | ✅ | Rate Limiting, Security Headers, Input Validation |

### 19.3 Detect (DE)

| Funktion | Kategorie | Status | Nachweis/Kommentar |
|----------|-----------|--------|-------------------|
| DE.AE | Anomalies and Events | ⚠️ | Audit-Logging, keine automatische Anomalieerkennung |
| DE.CM | Security Continuous Monitoring | ✅ | Prometheus Metrics, 65+ Audit-Events |
| DE.DP | Detection Processes | ⚠️ | Keine formalen Detection-Prozesse |

### 19.4 Respond (RS)

| Funktion | Kategorie | Status | Nachweis/Kommentar |
|----------|-----------|--------|-------------------|
| RS.AN | Analysis | ⚠️ | Audit-Logs analysierbar, keine automatische Analyse |
| RS.CO | Communications | ⚠️ | `SECURITY.md` vorhanden, IRP fehlt |
| RS.IM | Improvements | ⚠️ | Lessons Learned nicht dokumentiert |
| RS.MI | Mitigation | ⚠️ | Keine formalen Mitigationsprozesse |
| RS.RP | Response Planning | ⚠️ | Kein formaler Incident Response Plan |

### 19.5 Recover (RC)

| Funktion | Kategorie | Status | Nachweis/Kommentar |
|----------|-----------|--------|-------------------|
| RC.CO | Communications | ⚠️ | Keine Recovery-Kommunikationsplanung |
| RC.IM | Improvements | ⚠️ | Keine formale Recovery-Verbesserung |
| RC.RP | Recovery Planning | ✅ | Point-in-Time Recovery, WAL-Archivierung |

---

## 20. NIS2-Richtlinie (EU 2022/2555)

### 20.1 Risikomanagement-Maßnahmen (Artikel 21)

| Nr. | Anforderung | Status | Nachweis/Kommentar |
|-----|-------------|--------|-------------------|
| NIS2-01 | Risikoanalyse und Sicherheit von Informationssystemen | ⚠️ | Threat Model vorhanden, formale Risikoanalyse empfohlen |
| NIS2-02 | Bewältigung von Sicherheitsvorfällen | ⚠️ | Audit-Logging vorhanden, IRP fehlt |
| NIS2-03 | Aufrechterhaltung des Betriebs (BCM) | ⚠️ | Backup/Recovery vorhanden, kein BCP |
| NIS2-04 | Sicherheit der Lieferkette | ⚠️ | vcpkg-Dependencies, SBOM empfohlen |
| NIS2-05 | Sicherheit bei Erwerb, Entwicklung und Wartung | ✅ | SDLC, Code Reviews, Static Analysis |
| NIS2-06 | Bewertung der Wirksamkeit von Risikomanagement | ⚠️ | Keine regelmäßige formale Bewertung |
| NIS2-07 | Cyberhygiene und Schulungen | ⚠️ | `CONTRIBUTING.md`, Schulungsmaterial fehlt |
| NIS2-08 | Kryptographie und Verschlüsselung | ✅ | AES-256-GCM, TLS 1.3, RSA-SHA256 |
| NIS2-09 | Personalsicherheit | N/A | Open-Source-Projekt |
| NIS2-10 | Zugriffskontrolle | ✅ | RBAC mit 4-stufiger Hierarchie |
| NIS2-11 | Multi-Faktor-Authentifizierung | ⚠️ | mTLS verfügbar, MFA nicht explizit |

### 20.2 Meldepflichten (Artikel 23)

| Nr. | Anforderung | Status | Nachweis/Kommentar |
|-----|-------------|--------|-------------------|
| NIS2-M01 | Frühwarnung (24h) | ⚠️ | Kein formaler Prozess |
| NIS2-M02 | Sicherheitsvorfallmeldung (72h) | ⚠️ | Kein formaler Prozess |
| NIS2-M03 | Zwischen-/Abschlussbericht (1 Monat) | ⚠️ | Kein formaler Prozess |

---

## 21. PCI DSS v4.0 (falls Zahlungsdaten verarbeitet werden)

### 21.1 Aufbau und Wartung eines sicheren Netzwerks

| Anforderung | PCI DSS Ref | Status | Nachweis/Kommentar |
|-------------|-------------|--------|-------------------|
| Firewall-Konfiguration | 1.x | N/A | Deployment-abhängig |
| Keine Herstellerstandards für Passwörter | 2.x | ✅ | Konfigurierbare Authentifizierung |

### 21.2 Schutz von Karteninhaberdaten

| Anforderung | PCI DSS Ref | Status | Nachweis/Kommentar |
|-------------|-------------|--------|-------------------|
| Schutz gespeicherter Karteninhaberdaten | 3.x | ✅ | AES-256-GCM Verschlüsselung, PII-Detection |
| Verschlüsselung bei Übertragung | 4.x | ✅ | TLS 1.3, Strong Cipher Suites |

### 21.3 Schwachstellenmanagement

| Anforderung | PCI DSS Ref | Status | Nachweis/Kommentar |
|-------------|-------------|--------|-------------------|
| Anti-Malware | 5.x | ✅ | `MalwareFilterManager` implementiert, SignatureScanner + ClamAV |
| Sichere Systeme und Anwendungen | 6.x | ✅ | SDLC, Code Reviews, Static Analysis |

### 21.4 Zugriffskontrolle

| Anforderung | PCI DSS Ref | Status | Nachweis/Kommentar |
|-------------|-------------|--------|-------------------|
| Beschränkung des Zugriffs | 7.x | ✅ | RBAC implementiert |
| Eindeutige IDs | 8.x | ✅ | User-ID-Tracking in Audit-Logs |
| Physische Zugriffsbeschränkung | 9.x | N/A | Software-Produkt |

### 21.5 Überwachung und Tests

| Anforderung | PCI DSS Ref | Status | Nachweis/Kommentar |
|-------------|-------------|--------|-------------------|
| Netzwerküberwachung | 10.x | ✅ | Prometheus Metrics, Audit-Logging |
| Regelmäßige Sicherheitstests | 11.x | ⚠️ | Static Analysis vorhanden, Pen-Test empfohlen |

### 21.6 Informationssicherheitspolitik

| Anforderung | PCI DSS Ref | Status | Nachweis/Kommentar |
|-------------|-------------|--------|-------------------|
| Sicherheitsrichtlinie dokumentiert | 12.x | ⚠️ | Sicherheitsdokumentation vorhanden, formale Policy empfohlen |

---

## 22. TISAX (Trusted Information Security Assessment Exchange)

### 22.1 Informationssicherheit (AL 2/3)

| Modul | Anforderung | Status | Nachweis/Kommentar |
|-------|-------------|--------|-------------------|
| ISA-01 | Informationssicherheitsmanagement | ⚠️ | Sicherheitsdokumentation vorhanden, formales ISMS fehlt |
| ISA-02 | Personelle Sicherheit | N/A | Open-Source-Projekt |
| ISA-03 | Physische Sicherheit | N/A | Software-Produkt |
| ISA-04 | Identitäts- und Zugriffsmanagement | ✅ | RBAC, mTLS |
| ISA-05 | IT-Sicherheit / Cybersecurity | ✅ | Encryption, Secure Coding, Monitoring |
| ISA-06 | Lieferantenbeziehungen | ⚠️ | vcpkg-Dependencies |
| ISA-07 | Compliance | ✅ | Umfangreiche Compliance-Dokumentation |

### 22.2 Prototypenschutz (falls relevant)

| Modul | Anforderung | Status | Nachweis/Kommentar |
|-------|-------------|--------|-------------------|
| PROTO-01 | Schutz von Prototypen | N/A | Nicht anwendbar |
| PROTO-02 | Umgang mit Prototypen | N/A | Nicht anwendbar |

### 22.3 Datenschutz (TISAX-DSC)

| Modul | Anforderung | Status | Nachweis/Kommentar |
|-------|-------------|--------|-------------------|
| DSC-01 | Datenschutzmanagement | ✅ | PII-Detection, Retention Manager |
| DSC-02 | Technisch-organisatorische Maßnahmen | ✅ | Encryption, RBAC, Audit-Logging |

---

## 23. ISO/IEC 27701 (Privacy Information Management)

### 23.1 PIMS-spezifische Anforderungen

| Klausel | Anforderung | Status | Nachweis/Kommentar |
|---------|-------------|--------|-------------------|
| 5.2 | Datenschutzpolitik | ⚠️ | Implizit in Sicherheitsdokumentation |
| 5.4 | Rollen und Verantwortlichkeiten | ✅ | RBAC, Governance-Dokumentation |
| 6.2 | Datenschutzrisikobewertung | ⚠️ | Threat Model, formale DPIA empfohlen |
| 6.5 | Datenschutzziele | ⚠️ | Nicht explizit dokumentiert |
| 7.2 | Einwilligung | N/A | Anwendungsabhängig |
| 7.3 | Datenminimierung | ✅ | PII-Detection, Retention Policies |
| 7.4 | Löschung | ✅ | Retention Manager, Auto-Purge |
| 7.5 | Datenqualität | ⚠️ | Keine explizite Datenqualitätsprüfung |

### 23.2 PII-Controller (Verantwortlicher)

| Klausel | Anforderung | Status | Nachweis/Kommentar |
|---------|-------------|--------|-------------------|
| A.7.2 | Bestimmung der Zwecke | N/A | Anwendungsabhängig |
| A.7.3 | Einholung von Einwilligungen | N/A | Anwendungsabhängig |
| A.7.4 | Betroffenenrechte | ✅ | Export-API, Lösch-API |

### 23.3 PII-Processor (Auftragsverarbeiter)

| Klausel | Anforderung | Status | Nachweis/Kommentar |
|---------|-------------|--------|-------------------|
| B.8.2 | Verarbeitung nur gemäß Auftrag | ✅ | Governance-Policies, RBAC |
| B.8.3 | Aufzeichnungspflichten | ✅ | Audit-Logging |
| B.8.4 | Unterauftragnehmer | N/A | Open-Source-Projekt |
| B.8.5 | Datentransfer | ⚠️ | TLS vorhanden, keine Transferregeln |

---

## 24. Common Criteria (ISO/IEC 15408)

### 24.1 Sicherheitsfunktionsklassen

| Klasse | Beschreibung | Status | Nachweis/Kommentar |
|--------|--------------|--------|-------------------|
| FAU | Security Audit | ✅ | 65+ Audit-Event-Typen, Encrypt-then-Sign |
| FCS | Cryptographic Support | ✅ | AES-256-GCM, SHA-256, RSA-2048+ |
| FDP | User Data Protection | ✅ | Encryption, RBAC, Retention |
| FIA | Identification & Authentication | ✅ | mTLS, Token-Auth, RBAC |
| FMT | Security Management | ✅ | Admin-APIs, Key Rotation |
| FPR | Privacy | ✅ | PII-Detection, Pseudonymisierung |
| FPT | Protection of TSF | ✅ | TLS, Input Validation |
| FRU | Resource Utilization | ⚠️ | Rate Limiting vorhanden |
| FTA | TOE Access | ✅ | Session Management, Timeout |
| FTP | Trusted Path/Channels | ✅ | TLS 1.3, mTLS |

### 24.2 Vertrauenswürdigkeitsklassen (EAL)

| EAL | Beschreibung | Status | Nachweis/Kommentar |
|-----|--------------|--------|-------------------|
| EAL1 | Functionally tested | ✅ | 303/303 Tests PASS, 85%+ Coverage |
| EAL2 | Structurally tested | ✅ | Code Coverage, Static Analysis |
| EAL3 | Methodically tested and checked | ⚠️ | Formale Testmethodik empfohlen |
| EAL4 | Methodically designed, tested, and reviewed | ⚠️ | Formaler Entwurf empfohlen |
| EAL5+ | Semiformally/Formally designed and tested | 📋 | Nicht angestrebt |

---

## 25. KRITIS-Verordnung (BSI-Kritisverordnung)

### 25.1 Sektoren und Anwendbarkeit

| Sektor | Anwendbarkeit | Nachweis/Kommentar |
|--------|---------------|-------------------|
| Energie | ⚠️ | Möglich bei Einsatz in Smart Grid |
| Gesundheit | ⚠️ | Möglich bei Krankenhaus-Datenverarbeitung |
| IT und Telekommunikation | ⚠️ | Möglich bei großen Installationen |
| Transport und Verkehr | ⚠️ | Möglich bei Verkehrsinfrastruktur |
| Wasser | ⚠️ | Möglich bei Wasserversorgung |
| Ernährung | ⚠️ | Möglich bei Lebensmittellogistik |
| Finanz- und Versicherungswesen | ⚠️ | Möglich bei Finanzdienstleistern |

### 25.2 Anforderungen (§ 8a BSIG)

| Anforderung | Status | Nachweis/Kommentar |
|-------------|--------|-------------------|
| Stand der Technik | ✅ | Aktuelle Verschlüsselung, Protokolle |
| Angemessene Sicherheitsmaßnahmen | ✅ | Defense in Depth |
| Erkennung von Angriffen | ⚠️ | Audit-Logging, keine IDS-Integration |
| Meldepflicht | ⚠️ | Kein formaler Prozess |
| Nachweis alle 2 Jahre | ⚠️ | Audit-Checkliste vorhanden |

---

## 26. Technische Audit-Punkte

### 19.1 Source-Code-Audit

| Prüfpunkt | Status | Nachweis/Kommentar |
|-----------|--------|-------------------|
| Static Analysis (SAST) | ✅ | clang-tidy, cppcheck |
| Dynamic Analysis (DAST) | ⚠️ | Fuzzing empfohlen |
| Secret Scanning | ✅ | Gitleaks |
| Dependency Scanning | ⚠️ | Trivy-Integration geplant |
| Code Coverage | ✅ | 85%+ |
| Memory Safety | ✅ | AddressSanitizer verfügbar |
| Thread Safety | ⚠️ | ThreadSanitizer verfügbar |

### 19.2 API-Sicherheit

| Prüfpunkt | Status | Nachweis/Kommentar |
|-----------|--------|-------------------|
| Input Validation | ✅ | JSON Schema, Sanitization |
| SQL/AQL Injection | ✅ | Parameterized Queries |
| Path Traversal | ✅ | Path Sanitization |
| SSRF-Schutz | ⚠️ | Nicht explizit geprüft |
| Rate Limiting | ✅ | Token Bucket |
| CORS | ✅ | Whitelist-basiert |
| Security Headers | ✅ | Alle relevanten Header |

### 19.3 Kryptographie-Audit

| Prüfpunkt | Status | Nachweis/Kommentar |
|-----------|--------|-------------------|
| Keine schwachen Algorithmen | ✅ | Nur AES-256, SHA-256+, RSA-2048+ |
| Sichere Zufallszahlen | ✅ | OpenSSL RAND |
| Key Storage | ✅ | Vault/HSM-Integration |
| Key Rotation | ✅ | Lazy Re-Encryption |
| Constant-Time Operations | ⚠️ | Nicht explizit verifiziert |

---

## 27. Empfehlungen und Handlungsbedarf

### 27.1 Kritischer Handlungsbedarf (Priorität 1)

| # | Befund | Empfehlung | BSI C5 Ref | Status |
|---|--------|------------|------------|--------|
| 1 | ~~Keine SECURITY.md~~ | ~~Erstellen mit Meldeprozess für Schwachstellen~~ | SIM-02 | ✅ Erledigt |
| 2 | ~~Kein formales Incident Response~~ | ~~IRP dokumentieren~~ | SIM-01 | ✅ Erledigt |
| 3 | ~~SBOM fehlt~~ | ~~Syft/Cosign für SBOM-Generierung~~ | SSO-02 | ✅ Erledigt |
| 4 | Penetrationstest ausstehend | Externes Penetration Testing beauftragen | OPS-07 | ⚠️ Offen (Guide erstellt) |

### 27.2 Hoher Handlungsbedarf (Priorität 2)

| # | Befund | Empfehlung | BSI C5 Ref | Status |
|---|--------|------------|------------|--------|
| 5 | ~~Passwortrichtlinie fehlt~~ | ~~Policy in Dokumentation aufnehmen~~ | IDM-06 | ✅ Erledigt |
| 6 | ~~Formale Risikobewertung fehlt~~ | ~~DPIA/Risikoanalyse durchführen~~ | OIS-03 | ✅ Erledigt |
| 7 | ~~Backup-Tests undokumentiert~~ | ~~Regelmäßige Restore-Tests planen~~ | OPS-09 | ✅ Erledigt |
| 8 | NTP-Validierung fehlt | Zeitquellen-Validierung implementieren | OPS-14 | ⚠️ Offen |

### 27.3 Mittlerer Handlungsbedarf (Priorität 3)

| # | Befund | Empfehlung | BSI C5 Ref |
|---|--------|------------|------------|
| 9 | Lizenzprüfung manuell | Automatische Lizenzprüfung (license-checker) | DEP-04 |
| ~~10~~ | ~~Malware-Scan bei Ingestion~~ | ~~ClamAV-Integration für Blob-Uploads~~ | ~~OPS-05~~ ✅ ERLEDIGT |
| 11 | Fuzzing-Tests fehlen | AFL++/libFuzzer für Parser integrieren | DEV-03 |
| 12 | SSRF-Schutz | Explizite URL-Validierung für externe Aufrufe | COS-03 |

---

## 28. Audit-Protokoll

### 28.1 Audit-Durchführung

| Feld | Wert |
|------|------|
| **Auditor** | [Name eintragen] |
| **Datum** | [Datum eintragen] |
| **Version ThemisDB** | [Version eintragen] |
| **Commit** | [Git-Commit-Hash] |
| **Scope** | Vollständiges Review (Sourcecode + Dokumentation) |

### 28.2 Audit-Ergebnis

| Kategorie | Erfüllungsgrad |
|-----------|----------------|
| **BSI C5** | ~92% |
| **ISO 27001** | ~90% |
| **DSGVO** | ~95% |
| **eIDAS** | ~95% |
| **SOC 2** | ~90% |
| **NIST CSF** | ~85% |
| **NIS2** | ~80% |
| **PCI DSS** | ~85% |
| **TISAX** | ~85% |
| **ISO 27701** | ~85% |
| **Common Criteria** | EAL2+ |
| **KRITIS** | ~85% |

### 28.3 Gesamtbewertung

**Status:** ✅ **Produktionsreif**

ThemisDB weist eine umfassende Sicherheitsarchitektur auf und erfüllt die kritischen Compliance-Anforderungen. Die wesentlichen Lücken wurden geschlossen:
- ✅ Informationssicherheitspolitik dokumentiert
- ✅ Passwortrichtlinie definiert
- ✅ Business Continuity Plan inkl. RAID-Redundanz
- ✅ Risikomanagement-Framework etabliert
- ✅ DPIA durchgeführt
- ✅ Incident Response Plan vorhanden
- ✅ SECURITY.md mit Meldeprozess
- ✅ SBOM generiert

**Verbleibende Empfehlungen:**
- ⚠️ Externes Penetration Testing durchführen (Guide vorhanden)
- ⚠️ NTP-Zeitquellen-Validierung implementieren

---

## 29. Anhänge

### A. Referenzierte Dokumente

| Dokument | Pfad |
|----------|------|
| Security Overview | `docs/security/overview.md` |
| Audit Checklist (kurz) | `docs/security/audit_checklist.md` |
| Compliance Review | `docs/security/security_compliance_review.md` |
| Encryption Strategy | `docs/security/encryption_strategy.md` |
| Key Management | `docs/security/key_management.md` |
| HSM Integration | `docs/security/hsm_integration.md` |
| PKI Signatures | `docs/security/pki_signatures.md` |
| Threat Model | `docs/security/threat_model.md` |
| Hardening Guide | `docs/security/hardening_guide.md` |
| RBAC | `docs/security/implementation_summary.md` |
| Audit Logging | `docs/features/audit_logging.md` |
| PII Detection | `docs/security/pii_detection.md` |
| Retention Management | `docs/security/audit_and_retention.md` |
| Deployment Guide | `docs/guides/deployment.md` |
| Memory Tuning | `docs/performance/memory_tuning.md` |
| **Informationssicherheitspolitik** | `docs/security/INFORMATION_SECURITY_POLICY.md` |
| **Passwortrichtlinie** | `docs/security/PASSWORD_POLICY.md` |
| **Risikomanagement-Framework** | `docs/security/RISK_MANAGEMENT_FRAMEWORK.md` |
| **Business Continuity Plan** | `docs/compliance/BUSINESS_CONTINUITY_PLAN.md` |
| **DPIA** | `docs/compliance/DPIA.md` |
| **Incident Response Plan** | `docs/security/INCIDENT_RESPONSE_PLAN.md` |
| **RAID-Redundanz-Architektur** | `docs/sharding/RAID_REDUNDANCY_ARCHITECTURE.md` |
| **Penetration Test Guide** | `docs/security/PENETRATION_TEST_GUIDE.md` |

### B. BSI C5 Kontrollbereiche

| Kürzel | Bereich | Kontrollen |
|--------|---------|------------|
| OIS | Organisation der Informationssicherheit | OIS-01 bis OIS-07 |
| HRS | Personelle Sicherheit | HRS-01 bis HRS-05 |
| AM | Asset-Management | AM-01 bis AM-05 |
| IDM | Identitäts- und Berechtigungsmanagement | IDM-01 bis IDM-11 |
| CRY | Kryptographie und Schlüsselmanagement | CRY-01 bis CRY-04 |
| COS | Kommunikationssicherheit | COS-01 bis COS-08 |
| PHY | Physische Sicherheit | PHY-01 bis PHY-08 |
| OPS | Betriebssicherheit | OPS-01 bis OPS-23 |
| DEV | Entwicklung und Beschaffung | DEV-01 bis DEV-08 |
| SSO | Steuerung von Lieferanten | SSO-01 bis SSO-05 |
| SIM | Störungsmanagement | SIM-01 bis SIM-05 |
| COM | Compliance | COM-01 bis COM-04 |

### C. Glossar

| Begriff | Definition |
|---------|------------|
| AES-256-GCM | Advanced Encryption Standard mit 256-bit Schlüssel und Galois/Counter Mode |
| BSI | Bundesamt für Sicherheit in der Informationstechnik |
| C5 | Cloud Computing Compliance Criteria Catalogue |
| DSGVO | Datenschutz-Grundverordnung (GDPR) |
| eIDAS | EU-Verordnung über elektronische Identifizierung und Vertrauensdienste |
| HSTS | HTTP Strict Transport Security |
| HSM | Hardware Security Module |
| KRITIS | Kritische Infrastrukturen (BSI-Kritisverordnung) |
| mTLS | Mutual TLS (Client-Zertifikat-Authentifizierung) |
| NIS2 | Network and Information Security Directive 2 |
| NIST CSF | NIST Cybersecurity Framework |
| PCI DSS | Payment Card Industry Data Security Standard |
| PII | Personally Identifiable Information |
| PIMS | Privacy Information Management System (ISO 27701) |
| PKCS#11 | Cryptoki - Standard für Kryptographie-Token |
| RBAC | Role-Based Access Control |
| SBOM | Software Bill of Materials |
| TISAX | Trusted Information Security Assessment Exchange |
| TLS | Transport Layer Security |

---

## 30. Versionierung

| Version | Datum | Autor | Änderungen |
|---------|-------|-------|------------|
| 1.0 | November 2025 | ThemisDB Team | Erstversion |
| 1.1 | November 2025 | ThemisDB Team | Erweitert um NIST CSF, NIS2, PCI DSS, TISAX, ISO 27701, Common Criteria, KRITIS |

---

**Letzte Aktualisierung:** November 2025  
**Dokumentverantwortlicher:** ThemisDB Compliance Team  
**Nächstes Review:** [Datum eintragen]
