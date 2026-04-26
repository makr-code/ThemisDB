# ThemisDB - Informationssicherheitspolitik

**Version:** v1.3.0  
**Stand:** 6. April 2026  
**Klassifizierung:** Intern  
**Genehmigt durch:** ThemisDB Security Team  
**Kategorie:** 🔒 Security

---

## 📑 Inhaltsverzeichnis

- [1. Zweck und Geltungsbereich](#1-zweck-und-geltungsbereich)
- [2. Grundsätze der Informationssicherheit](#2-grundsätze-der-informationssicherheit)
- [3. Organisatorische Sicherheit](#3-organisatorische-sicherheit)
- [4. Zugriffskontrolle](#4-zugriffskontrolle)
- [5. Kryptographie](#5-kryptographie)
- [6. Datenschutz](#6-datenschutz)
- [7. Betriebssicherheit](#7-betriebssicherheit)

---

## 1. Zweck und Geltungsbereich

### 1.1 Zweck

Diese Informationssicherheitspolitik (ISP) definiert die grundlegenden Prinzipien, Ziele und Verantwortlichkeiten für die Informationssicherheit bei der Entwicklung, dem Betrieb und der Nutzung von ThemisDB.

### 1.2 Geltungsbereich

Diese Politik gilt für:
- Alle Entwickler und Mitwirkenden am ThemisDB-Projekt
- Alle Betreiber und Administratoren von ThemisDB-Instanzen
- Alle Anwendungen und Systeme, die ThemisDB nutzen
- Alle Daten, die in ThemisDB gespeichert oder verarbeitet werden

### 1.3 Referenzierte Standards

| Standard | Version | Beschreibung |
|----------|---------|--------------|
| BSI C5 | 2020 | Cloud Computing Compliance Criteria Catalogue |
| ISO/IEC 27001 | 2022 | Informationssicherheitsmanagementsystem |
| ISO/IEC 27002 | 2022 | Leitfaden für Informationssicherheitsmaßnahmen |
| DSGVO | 2016/679 | Datenschutz-Grundverordnung |
| NIST CSF | 2.0 | Cybersecurity Framework |

---

## 2. Grundsätze der Informationssicherheit

### 2.1 Schutzziele

ThemisDB verpflichtet sich zur Gewährleistung der drei grundlegenden Schutzziele:

| Schutzziel | Definition | Implementierung in ThemisDB |
|------------|------------|----------------------------|
| **Vertraulichkeit** | Schutz vor unbefugtem Zugriff | RBAC, AES-256-GCM Verschlüsselung, mTLS |
| **Integrität** | Schutz vor unbefugter Änderung | Hash-Ketten, RSA-SHA256 Signaturen, MVCC |
| **Verfügbarkeit** | Gewährleistung der Nutzbarkeit | Backup/Recovery, Point-in-Time Recovery, WAL |

### 2.2 Zusätzliche Schutzziele

| Schutzziel | Definition | Implementierung in ThemisDB |
|------------|------------|----------------------------|
| **Authentizität** | Nachweis der Echtheit | mTLS, Token-Authentifizierung, PKI |
| **Nichtabstreitbarkeit** | Nachweisbarkeit von Aktionen | Audit-Logging, tamper-proof Logs |
| **Zurechenbarkeit** | Zuordnung zu Verursacher | User-ID in Audit-Logs, Session-Tracking |

### 2.3 Security by Design

ThemisDB folgt dem Prinzip "Security by Design":

1. **Defense in Depth** - Mehrschichtige Sicherheitskontrollen
2. **Least Privilege** - Minimale notwendige Berechtigungen
3. **Fail Secure** - Sicherer Fehlerzustand
4. **Zero Trust** - Keine implizite Vertrauensstellung
5. **Secure Defaults** - Sichere Standardkonfiguration

---

## 3. Organisatorische Sicherheit

### 3.1 Rollen und Verantwortlichkeiten

#### 3.1.1 Entwicklungsrollen

| Rolle | Verantwortlichkeiten |
|-------|---------------------|
| **Security Lead** | Gesamtverantwortung für Sicherheitsarchitektur |
| **Core Maintainer** | Code Review, Sicherheitsentscheidungen |
| **Contributor** | Einhaltung sicherer Entwicklungspraktiken |
| **Reviewer** | Sicherheits-Reviews bei PRs |

#### 3.1.2 Betriebsrollen (RBAC)

| Rolle | Berechtigungen | Beschreibung |
|-------|---------------|--------------|
| **admin** | Vollzugriff | Systemadministration, Key-Rotation |
| **operator** | data:read/write/delete | Tagesbetrieb |
| **analyst** | data:read, audit:view | Reporting, Analyse |
| **readonly** | data:read | Nur Lesezugriff |

### 3.2 Sicherheitsbewusstsein

Alle Mitwirkenden müssen:
- Die CONTRIBUTING.md-Richtlinien kennen und befolgen
- Security Best Practices verstehen
- Sicherheitsvorfälle unverzüglich melden
- An Sicherheitsschulungen teilnehmen (falls angeboten)

---

## 4. Zugriffskontrolle

### 4.1 Authentifizierung

**Unterstützte Mechanismen:**
- mTLS (Mutual TLS) mit Client-Zertifikaten
- Token-basierte Authentifizierung (JWT/API-Keys)
- HSM-Integration (PKCS#11) für Schlüsselverwaltung

**Anforderungen:**
- TLS 1.3 (TLS 1.2 als Fallback)
- Starke Cipher Suites (ECDHE-RSA-AES256-GCM-SHA384)
- Zertifikatspinning für kritische Verbindungen

### 4.2 Autorisierung

**RBAC-Prinzipien:**
- Rollenbasierte Zugriffskontrolle (4-stufige Hierarchie)
- Ressourcenbasierte Berechtigungen
- Wildcard-Unterstützung (`*:*`)
- Regelmäßige Berechtigungsüberprüfung

### 4.3 Passwortrichtlinie

Siehe: `docs/security/PASSWORD_POLICY.md`

---

## 5. Kryptographie

### 5.1 Verschlüsselungsstandards

| Anwendung | Algorithmus | Schlüssellänge | Status |
|-----------|-------------|----------------|--------|
| Data-at-Rest | AES-256-GCM | 256 bit | Pflicht |
| Data-in-Transit | TLS 1.3 | - | Pflicht |
| Signaturen | RSA-SHA256 | 2048+ bit | Pflicht |
| Hashing | SHA-256/384/512 | - | Pflicht |
| Key Derivation | HKDF-SHA256 | - | Pflicht |

### 5.2 Schlüsselmanagement

**Key Provider:**
1. **MockKeyProvider** - Nur für Entwicklung
2. **HSMKeyProvider** - PKCS#11 HSM-Integration
3. **VaultKeyProvider** - HashiCorp Vault

**Key Rotation:**
- Regelmäßige Schlüsselrotation empfohlen (90 Tage)
- Lazy Re-Encryption für unterbrechungsfreien Betrieb
- Audit-Logging aller Schlüsseloperationen

### 5.3 Verbotene Algorithmen

Die Verwendung folgender Algorithmen ist untersagt:
- MD5 (außer für Legacy-Kompatibilität, nicht sicherheitskritisch)
- SHA-1 (für Signaturen)
- DES, 3DES
- RC4
- RSA < 2048 bit
- TLS < 1.2

---

## 6. Datenschutz

### 6.1 Datenklassifizierung

| Stufe | Verschlüsselung | Retention | Beispiele |
|-------|-----------------|-----------|-----------|
| `offen` | Optional | 30 Tage | Öffentliche Daten |
| `vs-nfd` | Pflicht | 365 Tage | Interne Dokumente |
| `geheim` | Pflicht | 90 Tage | Personendaten |
| `streng_geheim` | Pflicht | 30 Tage | Hochsensible Daten |

### 6.2 PII-Behandlung

- Automatische PII-Erkennung (7 Mustertypen)
- Field-Level Encryption für sensitive Felder
- Retention Policies mit automatischer Löschung
- Data Export für Auskunftsrechte

### 6.3 DSGVO-Compliance

Implementierte Betroffenenrechte:
- ✅ Art. 15: Auskunftsrecht (Data Export API)
- ✅ Art. 16: Berichtigung (Entity Update API)
- ✅ Art. 17: Löschung (Retention Manager)
- ⚠️ Art. 18: Einschränkung (Governance-Flags)
- ✅ Art. 20: Datenübertragbarkeit (JSON/CSV Export)

---

## 7. Betriebssicherheit

### 7.1 Änderungsmanagement

- Alle Änderungen über Git mit Versionierung
- Pull Request Reviews erforderlich
- CHANGELOG.md für Änderungshistorie
- Semantic Versioning für Releases

### 7.2 Logging und Monitoring

**Audit-Logging:**
- 65+ Event-Typen
- Encrypt-then-Sign Pattern
- Hash-Kette für Manipulationsschutz
- SIEM-Integration (Syslog, Splunk HEC)

**Monitoring:**
- Prometheus Metrics Export
- Health Checks
- Performance Metriken

### 7.3 Backup und Recovery

- RocksDB Checkpoints für konsistente Backups
- Point-in-Time Recovery mit WAL
- Dokumentierte Restore-Prozeduren
- Regelmäßige Backup-Tests (siehe BCP)

---

## 8. Incident Response

### 8.1 Meldewege

Sicherheitsvorfälle sind zu melden an:
- **E-Mail:** service@themisdb.org
- **GitHub:** Security Advisories (privat)
- **SECURITY.md:** Meldeprozess dokumentiert

### 8.2 Incident Response Plan

Siehe: `docs/security/INCIDENT_RESPONSE_PLAN.md`

### 8.3 Eskalationsstufen

| Stufe | Beschreibung | Reaktionszeit | Beispiel |
|-------|--------------|---------------|----------|
| **Kritisch** | Aktive Ausnutzung | < 4 Stunden | Datenleck, RCE |
| **Hoch** | Hohe Ausnutzbarkeit | < 24 Stunden | Auth-Bypass |
| **Mittel** | Begrenzte Auswirkung | < 1 Woche | Info-Disclosure |
| **Niedrig** | Geringe Auswirkung | < 1 Monat | Best Practice |

---

## 9. Compliance

### 9.1 Regulatorische Anforderungen

ThemisDB unterstützt Compliance mit:
- BSI C5 (~85%)
- ISO/IEC 27001 (~80%)
- DSGVO (~90%)
- eIDAS (~95%)
- SOC 2 (~85%)
- HIPAA (falls anwendbar)
- PCI DSS (falls anwendbar)

### 9.2 Audit und Nachweis

- Vollständige Audit-Checkliste: `docs/FULL_AUDIT_CHECKLIST.md`
- Compliance-Dashboard: `docs/COMPLIANCE_DASHBOARD.md`
- Regelmäßige Self-Assessments empfohlen

---

## 10. Ausnahmen

### 10.1 Ausnahmeprozess

Ausnahmen von dieser Politik erfordern:
1. Dokumentierte Begründung
2. Risikobewertung
3. Genehmigung durch Security Lead
4. Befristung und Review-Datum
5. Kompensationsmaßnahmen

### 10.2 Bekannte Ausnahmen

| Ausnahme | Begründung | Kompensation | Review |
|----------|------------|--------------|--------|
| MockKeyProvider in Dev | Entwicklungseffizienz | Nur lokale Entwicklung | Laufend |

---

## 11. Überprüfung und Aktualisierung

### 11.1 Review-Zyklus

Diese Politik wird mindestens jährlich überprüft oder bei:
- Signifikanten Sicherheitsvorfällen
- Wesentlichen Änderungen an ThemisDB
- Neuen regulatorischen Anforderungen
- Änderungen in der Bedrohungslandschaft

### 11.2 Änderungshistorie

| Version | Datum | Autor | Änderungen |
|---------|-------|-------|------------|
| 1.0 | Dezember 2025 | ThemisDB Team | Erstversion |

---

## 12. Kontakt

**Security-Fragen:**
- GitHub Issues (nicht-vertraulich)
- Security Advisories (vertraulich)
- service@themisdb.org

**Dokumentation:**
- Repository: https://github.com/makr-code/ThemisDB
- Wiki: https://github.com/makr-code/ThemisDB/wiki

---

**Genehmigung:**

| Rolle | Name | Datum |
|-------|------|-------|
| Security Lead | [Name] | [Datum] |
| Project Lead | [Name] | [Datum] |

---

**Letzte Aktualisierung:** Dezember 2025  
**Nächstes Review:** Dezember 2026
