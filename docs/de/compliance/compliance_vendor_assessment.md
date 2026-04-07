# ThemisDB – Third-Party & Vendor Security Assessment

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🔒 Compliance  
**Klassifizierung:** Vertraulich  
**Basis:** ISO 27001 (A.15), BSI C5 (SSO), NIS2, SOC 2

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#-übersicht)
- [Anwendungsbereich](#1-anwendungsbereich)
- [Software-Abhängigkeiten](#2-software-abhängigkeiten-sbom)

---

## 📋 Übersicht

Dieses Dokument beschreibt den Prozess zur Bewertung der Sicherheit von Drittanbietern und Lieferanten, die mit ThemisDB in Verbindung stehen. Es umfasst die Bewertung von Software-Abhängigkeiten, Cloud-Diensten und externen Partnern.

---

## 1. Anwendungsbereich

### 1.1 Betroffene Kategorien

| Kategorie | Beschreibung | Risikostufe |
|-----------|--------------|-------------|
| **Software-Abhängigkeiten** | vcpkg, npm, pip Pakete | Hoch |
| **Cloud-Dienste** | Hosting, Backup, CDN | Hoch |
| **Entwicklungstools** | CI/CD, IDE, Analyse | Mittel |
| **Externe APIs** | Integration, Services | Mittel |
| **Support/Beratung** | Externe Dienstleister | Niedrig |

### 1.2 Ausgenommen

- Interne Entwicklungen
- Open-Source ohne externe Abhängigkeit
- Einmalige Tools ohne Datenzugriff

---

## 2. Software-Abhängigkeiten (SBOM)

### 2.1 Aktuelle Abhängigkeiten

| Paket | Version | Lizenz | Risiko | Status |
|-------|---------|--------|--------|--------|
| **rocksdb** | 8.x | Apache-2.0 | ⚠️ Hoch | ✅ Geprüft |
| **openssl** | 3.x | Apache-2.0 | ⚠️ Hoch | ✅ Geprüft |
| **boost** | 1.83+ | BSL-1.0 | 🟡 Mittel | ✅ Geprüft |
| **simdjson** | 3.x | Apache-2.0 | 🟢 Niedrig | ✅ Geprüft |
| **nlohmann-json** | 3.x | MIT | 🟢 Niedrig | ✅ Geprüft |
| **spdlog** | 1.x | MIT | 🟢 Niedrig | ✅ Geprüft |
| **tbb** | 2021.x | Apache-2.0 | 🟢 Niedrig | ✅ Geprüft |
| **curl** | 8.x | MIT | ⚠️ Hoch | ✅ Geprüft |
| **yaml-cpp** | 0.8+ | MIT | 🟢 Niedrig | ✅ Geprüft |
| **arrow** | 14.x | Apache-2.0 | 🟡 Mittel | ✅ Geprüft |
| **hnswlib** | latest | Apache-2.0 | 🟢 Niedrig | ✅ Geprüft |
| **gtest** | 1.14+ | BSD-3 | 🟢 Niedrig | ✅ Geprüft |
| **benchmark** | 1.x | Apache-2.0 | 🟢 Niedrig | ✅ Geprüft |

### 2.2 Bewertungskriterien für Dependencies

| Kriterium | Gewichtung | Beschreibung |
|-----------|------------|--------------|
| **Wartung** | 25% | Letzte Updates, Maintainer-Aktivität |
| **Sicherheit** | 30% | CVE-Historie, Security-Advisories |
| **Lizenz** | 15% | Kompatibilität, Einschränkungen |
| **Verbreitung** | 15% | Community-Größe, Adoption |
| **Alternativen** | 15% | Verfügbarkeit von Ersatz |

### 2.3 Automatisierte Überwachung

| Tool | Zweck | Frequenz |
|------|-------|----------|
| **Grype** | Vulnerability Scanning | Bei jedem Build |
| **SBOM Workflow** | Dependency Tracking | Bei Release |
| **Trivy** | Container/FS Scan | Wöchentlich |
| **GitHub Dependabot** | Update-Benachrichtigung | Automatisch |

---

## 3. Cloud-Dienste Assessment

### 3.1 Potenzielle Cloud-Provider

| Provider | Dienst | Zertifizierungen | Bewertung |
|----------|--------|------------------|-----------|
| **AWS** | Hosting, S3 | SOC 2, ISO 27001, C5 | ✅ Geeignet |
| **Azure** | Hosting, Blob | SOC 2, ISO 27001, C5 | ✅ Geeignet |
| **GCP** | Hosting, GCS | SOC 2, ISO 27001 | ✅ Geeignet |
| **Hetzner** | Hosting | ISO 27001, DSGVO | ⚠️ Bedingt |
| **DigitalOcean** | Hosting | SOC 2 | ⚠️ Bedingt |

### 3.2 Bewertungskriterien für Cloud-Dienste

| Kriterium | Erforderlich | Optional |
|-----------|--------------|----------|
| **SOC 2 Type II** | ✅ | - |
| **ISO 27001** | ✅ | - |
| **BSI C5** | - | ✅ |
| **DSGVO-Konformität** | ✅ | - |
| **EU-Rechenzentrum** | ✅ | - |
| **Verschlüsselung at-rest** | ✅ | - |
| **Verschlüsselung in-transit** | ✅ | - |
| **MFA-Support** | ✅ | - |
| **Audit-Logs** | ✅ | - |
| **SLA > 99.9%** | ✅ | - |

### 3.3 AVV (Auftragsverarbeitungsvertrag)

| Anforderung | Prüfpunkt |
|-------------|-----------|
| Art. 28 DSGVO | AVV vorhanden? |
| Subunternehmer | Liste und Genehmigung? |
| Technische Maßnahmen | TOMs dokumentiert? |
| Audit-Rechte | Inspektionsrecht vereinbart? |
| Datenlöschung | Prozess nach Ende? |
| Meldepflichten | Incident-Notification? |

---

## 4. Vendor Assessment Fragebogen

### 4.1 Allgemeine Informationen

| Feld | Antwort |
|------|---------|
| **Firmenname** | [Eintragen] |
| **Kontaktperson** | [Eintragen] |
| **Datum der Bewertung** | [Eintragen] |
| **Bewertet durch** | [Eintragen] |
| **Dienst/Produkt** | [Eintragen] |
| **Datenzugriff** | Ja / Nein |
| **Daten-Klassifizierung** | [Öffentlich/Intern/Vertraulich/Streng Geheim] |

### 4.2 Sicherheitsfragen

| # | Frage | Antwort | Nachweis |
|---|-------|---------|----------|
| 1 | Besitzt der Anbieter ISO 27001 Zertifizierung? | | |
| 2 | Besitzt der Anbieter SOC 2 Type II Attestierung? | | |
| 3 | Werden regelmäßige Penetrationstests durchgeführt? | | |
| 4 | Gibt es ein dokumentiertes Incident Response Verfahren? | | |
| 5 | Werden Daten verschlüsselt (at-rest und in-transit)? | | |
| 6 | Gibt es Multi-Factor Authentication? | | |
| 7 | Werden Audit-Logs geführt und aufbewahrt? | | |
| 8 | Gibt es ein Vulnerability Management Programm? | | |
| 9 | Wie werden Mitarbeiter in Security geschult? | | |
| 10 | Gibt es ein Business Continuity Plan? | | |

### 4.3 Datenschutzfragen

| # | Frage | Antwort | Nachweis |
|---|-------|---------|----------|
| 1 | Wo werden die Daten gespeichert (Land/Region)? | | |
| 2 | Werden Daten in Drittländer übertragen? | | |
| 3 | Gibt es einen AVV/DPA? | | |
| 4 | Wie wird das Recht auf Löschung umgesetzt? | | |
| 5 | Werden Subunternehmer eingesetzt? | | |
| 6 | Wie werden Datenschutzverletzungen gemeldet? | | |

### 4.4 Bewertungsergebnis

| Kategorie | Punkte (0-10) | Gewichtung | Score |
|-----------|---------------|------------|-------|
| Sicherheitszertifizierungen | | 25% | |
| Technische Maßnahmen | | 25% | |
| Datenschutz-Compliance | | 20% | |
| Incident Response | | 15% | |
| Vertragliche Absicherung | | 15% | |
| **Gesamt** | | 100% | |

**Bewertungsskala:**
- 80-100%: ✅ Genehmigt
- 60-79%: ⚠️ Bedingt genehmigt (Auflagen)
- < 60%: ❌ Nicht genehmigt

---

## 5. Risikobewertung Drittanbieter

### 5.1 Risiko-Matrix

| Risiko | Wahrscheinlichkeit | Auswirkung | Score |
|--------|-------------------|------------|-------|
| Datenverlust durch Anbieter | Niedrig | Hoch | 🟡 Mittel |
| Sicherheitsvorfall beim Anbieter | Mittel | Hoch | 🟠 Hoch |
| Vendor Lock-in | Niedrig | Mittel | 🟢 Niedrig |
| Compliance-Verstoß durch Anbieter | Niedrig | Hoch | 🟡 Mittel |
| Dienst-Ausfall | Mittel | Mittel | 🟡 Mittel |
| Supply Chain Attack | Niedrig | Sehr Hoch | 🟠 Hoch |

### 5.2 Mitigationsmaßnahmen

| Risiko | Maßnahme |
|--------|----------|
| Datenverlust | Verschlüsselung, eigene Backups |
| Sicherheitsvorfall | AVV mit Meldepflicht, Audit-Rechte |
| Vendor Lock-in | Abstraktions-Layer, Alternativen evaluieren |
| Compliance-Verstoß | Regelmäßige Audits, Zertifikate prüfen |
| Dienst-Ausfall | Multi-Provider, Fallback-Strategien |
| Supply Chain Attack | SBOM, Signaturprüfung, Monitoring |

---

## 6. Laufende Überwachung

### 6.1 Monitoring-Aktivitäten

| Aktivität | Frequenz | Verantwortlich |
|-----------|----------|----------------|
| SBOM Vulnerability Scan | Wöchentlich | Security |
| Zertifikat-Gültigkeit prüfen | Vierteljährlich | Compliance |
| Vendor Security News | Laufend | Security |
| AVV-Review | Jährlich | Legal |
| Re-Assessment | Alle 2 Jahre | Compliance |

### 6.2 Eskalation

| Trigger | Aktion |
|---------|--------|
| Kritische Vulnerability | Sofortige Bewertung, ggf. Abschaltung |
| Sicherheitsvorfall beim Vendor | IRP aktivieren, Kommunikation |
| Zertifikat abgelaufen | Eskalation an Management |
| AVV-Verletzung | Rechtliche Prüfung |

---

## 7. Dokumentation

### 7.1 Erforderliche Unterlagen

| Dokument | Aufbewahrung |
|----------|--------------|
| Vendor Assessment Report | 5 Jahre |
| AVV/DPA | Vertragsdauer + 3 Jahre |
| Zertifikate (Kopien) | Laufend aktuell |
| Audit-Berichte | 5 Jahre |
| Korrespondenz | 3 Jahre |

### 7.2 Archiv

| Vendor | Assessment-Datum | Score | Status |
|--------|------------------|-------|--------|
| [Template] | [Datum] | [%] | [Status] |

---

## 8. Aktuelle Vendor-Liste

### 8.1 Genehmigte Anbieter

| Vendor | Kategorie | Risk Level | Letztes Assessment | Nächstes Review |
|--------|-----------|------------|-------------------|-----------------|
| RocksDB (Facebook) | Dependency | Mittel | Nov 2025 | Nov 2026 |
| OpenSSL Foundation | Dependency | Hoch | Nov 2025 | Mai 2026 |
| Boost C++ Libraries | Dependency | Niedrig | Nov 2025 | Nov 2026 |
| Intel TBB | Dependency | Niedrig | Nov 2025 | Nov 2026 |
| HashiCorp Vault | Security | Hoch | Nov 2025 | Mai 2026 |

### 8.2 Ausstehende Assessments

| Vendor | Kategorie | Priorität | Geplant |
|--------|-----------|-----------|---------|
| [Cloud Provider] | Hosting | Hoch | Bei Bedarf |
| [Backup Provider] | Backup | Hoch | Bei Bedarf |

---

## 9. Anhänge

### A. Checkliste für neue Anbieter

- [ ] Business Case dokumentiert
- [ ] Datenschutz-Prüfung durchgeführt
- [ ] Security Assessment ausgefüllt
- [ ] AVV/DPA unterzeichnet
- [ ] Zertifikate geprüft
- [ ] Risikobewertung dokumentiert
- [ ] Genehmigung eingeholt
- [ ] In Vendor-Liste aufgenommen

### B. Referenzen

| Dokument | Pfad |
|----------|------|
| SBOM | `docs/security/SBOM.md` |
| DPIA | `docs/compliance/DPIA.md` |
| Risk Register | `docs/compliance/RISK_REGISTER.md` |
| Audit Checklist | `docs/FULL_AUDIT_CHECKLIST.md` |

---

**Letzte Aktualisierung:** November 2025  
**Dokumentverantwortlicher:** ThemisDB Procurement/Security  
**Nächstes Review:** [Datum + 12 Monate]
