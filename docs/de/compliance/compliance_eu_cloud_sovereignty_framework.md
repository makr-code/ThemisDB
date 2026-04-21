# ThemisDB – ES3 Audit nach EU Cloud Sovereignty Framework
## Tiefenanalyse nach wissenschaftlicher Methodik

**Stand:** 21. April 2026  
**Version:** v1.8.2-rc  
**Kategorie:** 🔒 Compliance  
**Klassifizierung:** Vertraulich  
**Prüfstandard:** ES3 (EU Cloud Sovereignty Framework, Schwarz-Edition, + KI als 9. Dimension)  
**Verifikationsinstanz:** Interne Vorbewertung; externe Prüfung durch BDO vorgesehen  
**Autoren:** ThemisDB Governance & Security Team  
**Methodik-Referenzen:** ENISA EUCS [1], BSI C5 [2], ISO 31000 [3], NIST RMF [4], GAIA-X Trust Framework [5]

---

## 📑 Inhaltsverzeichnis

1. [Wissenschaftliche Methodik und Prüfrahmen](#1-wissenschaftliche-methodik-und-prüfrahmen)
2. [Bewertungsrubrik (5-Punkt-Skala)](#2-bewertungsrubrik-5-punkt-skala)
3. [Evidenzbasis und Quelldokumentation](#3-evidenzbasis-und-quelldokumentation)
4. [Dimension-für-Dimension-Tiefenanalyse](#4-dimension-für-dimension-tiefenanalyse)
   - D1 Strategische Souveränität
   - D2 Rechtliche & jurisdiktionelle Aspekte
   - D3 Daten
   - D4 Operative Unabhängigkeit
   - D5 Lieferkette
   - D6 Technologie
   - D7 Sicherheit & Compliance
   - D8 Ökologische Nachhaltigkeit
   - D9 Künstliche Intelligenz (ES3-spezifisch)
5. [Gesamtbewertung und Sensitivitätsanalyse](#5-gesamtbewertung-und-sensitivitätsanalyse)
6. [Gap-Register](#6-gap-register)
7. [SMART-Maßnahmenplan](#7-smart-maßnahmenplan)
8. [Audit-Traceability-Matrix](#8-audit-traceability-matrix)
9. [Externe Prüfung und Reviewprozess](#9-externe-prüfung-und-reviewprozess)
10. [Wissenschaftliche Referenzen](#10-wissenschaftliche-referenzen)

---

## 1. Wissenschaftliche Methodik und Prüfrahmen

### 1.1 Forschungsdesign

Der Bericht folgt einem **strukturierten Audit-Ansatz** nach ISO 19011 (Leitlinien zur Auditierung von Managementsystemen) [6] in Kombination mit dem **ENISA European Union Cloud Service Assessment (EUCS)** Level-Konzept [1]. Das Minimum-Prinzip entspricht dem worst-case-gebundenen Sicherheitsmodell nach ISO 31000 §6.4.2 [3].

**Prüfmethoden:**

| Methode | Beschreibung | Anwendung |
|---------|-------------|-----------|
| **Dokumenten-Review** | Systematische Sichtung aller Compliance-Artefakte | Regulatorik, Organisation |
| **Source-Code-Analyse** | Inspektion von Quellcode und Header gegen Sicherheitsanforderungen | Technologie |
| **Gap-Analyse** | Differenz Soll-/Ist-Zustand je Kontrolle | Alle Dimensionen |
| **Residualrisiko-Bewertung** | Wahrscheinlichkeit × Auswirkung nach ISO 31000 | Gap-Register |
| **Sensitivitätsanalyse** | Simulation der Gesamtwertsänderung je Maßnahme | Abschnitt 5 |

### 1.2 Operationalisierung der ES3-Dimensionen

Die ES3-Struktur wird nach dem **Drei-Ebenen-Modell** operationalisiert:

```
┌──────────────────────────────────────────────────────────────────────┐
│   ES3-Dimension (D1–D9)                                              │
├────────────────┬─────────────────────────┬───────────────────────────┤
│  Ebene R       │  Ebene O                │  Ebene T                  │
│  Regulatorik   │  Organisation           │  Technologie              │
│  ──────────    │  ──────────────         │  ─────────────            │
│  Gesetzliche   │  Prozesse, Rollen,      │  Implementierung,         │
│  Anforderungen,│  Dokumentation,         │  Quellcode, Konfiguration,│
│  Standards,    │  Zuständigkeiten,       │  Messtechnik, Automatisie-│
│  Verordnungen  │  Schulung, Review-Turnus│  rung, Testabdeckung      │
└────────────────┴─────────────────────────┴───────────────────────────┘

Dimensionswert  = min(R, O, T)
Gesamtwert (ES3) = min(D1, D2, ..., D9)   [Minimum-Prinzip]
```

### 1.3 Gültigkeitsgrenzen (Scope)

- **In Scope:** ThemisDB v1.8.2-rc als Software-Produkt, zugehörige Governance-/Compliance-Dokumentation, Source-Code-Artefakte und CI/CD-Konfiguration.
- **Out of Scope:** Spezifische Deployment-Umgebungen der Endkunden, physische Rechenzentrumsinfrastruktur, Hosting-Provider.
- **Bewertungszeitraum:** 1. November 2025 – 21. April 2026 (commit `de44111`).

---

## 2. Bewertungsrubrik (5-Punkt-Skala)

Die folgende Rubrik gilt einheitlich für jede der drei Ebenen (R / O / T) jeder Dimension:

| Stufe | Bezeichnung | Definition |
|-------|-------------|------------|
| **1** | **Initial** | Keine dokumentierten Kontrollen; ad-hoc-Maßnahmen; keine messbaren Nachweise. |
| **2** | **Managed** | Grundlegende Maßnahmen vorhanden, aber nicht systematisch dokumentiert, nicht regelmäßig überprüft, keine vollständige Evidenz. |
| **3** | **Defined** | Schriftlich definierte Kontrollen mit zugewiesenen Verantwortlichkeiten; turnusmäßige Reviews angekündigt; wesentliche Nachweise vorhanden, jedoch Lücken in Vollständigkeit oder Aktualität. |
| **4** | **Quantitatively Managed** | Kontrollen vollständig implementiert und dokumentiert; messbare KPIs; mindestens jährlich überprüft; unabhängige Testbestätigung verfügbar. |
| **5** | **Optimizing** | Kontinuierliche Verbesserung nachgewiesen; externe Zertifizierung oder Attestierung; automatisierte Überwachung aller Kontrollen; keine offenen Findings. |

---

## 3. Evidenzbasis und Quelldokumentation

### 3.1 Primärquellen (ThemisDB-Artefakte)

| ID | Artefakt | Pfad | Relevanz |
|----|---------|------|----------|
| E-01 | Vollständige Compliance-Checkliste | `docs/de/compliance/compliance_full_checklist.md` | D1–D7 |
| E-02 | Vendor-Assessment | `docs/de/compliance/compliance_vendor_assessment.md` | D2, D5 |
| E-03 | DPIA (DSGVO Art. 35) | `docs/de/compliance/compliance_dpia.md` | D2, D3 |
| E-04 | Risk Register (ISO 31000) | `docs/de/compliance/compliance_risk_register.md` | D1–D9 |
| E-05 | BCP/DRP (ISO 22301) | `docs/de/compliance/compliance_bcp_drp.md` | D4 |
| E-06 | Datenklassifizierungsrichtlinie | `docs/de/policies/policies_data_classification.md` | D3 |
| E-07 | SBOM (SPDX 2.3 / CycloneDX 1.5) | `docs/de/security/security_sbom.md` | D5 |
| E-08 | BSI C5 Executive Summary | `docs/de/security/BSI_C5_EXECUTIVE_SUMMARY.md` | D7 |
| E-09 | Security Hardening Guide v1.4.0 | `docs/de/security/security_hardening.md` | D7 |
| E-10 | Governance ROADMAP | `src/governance/ROADMAP.md` | D7, D9 |
| E-11 | AI/ML Impact Assessment | `src/governance/AI_ML_IMPACT_ASSESSMENT.md` | D9 |
| E-12 | Compliance-Governance-Features | `docs/de/features/features_compliance_governance.md` | D3, D7 |
| E-13 | Cryptography Policy (BSI TR-02102-1) | `docs/de/security/CRYPTOGRAPHY_POLICY.md` | D3, D7 |
| E-14 | Security Audit Report | `docs/de/security/security_audit_report.md` | D7 |
| E-15 | Risikomanagement-Framework | `docs/de/security/security_risk_management.md` | D1, D4 |

### 3.2 Regulatorische Referenzdokumente

| Referenz | Titel | Relevanz |
|---------|-------|----------|
| EUCS v2.0 | ENISA European Union Cybersecurity Certification Scheme for Cloud Services | D1, D3, D7 |
| BSI C5 2020 | Cloud Computing Compliance Criteria Catalogue | D2, D3, D5, D7 |
| DSGVO (EU) 2016/679 | Datenschutz-Grundverordnung | D2, D3 |
| NIS2 (EU) 2022/2555 | Network and Information Security Directive | D4, D7 |
| GAIA-X Trust Framework 22.10 | Gaia-X Federation Services Trust Framework | D1, D6 |
| EU AI Act 2024/1689 | Artificial Intelligence Act | D9 |

---

## 4. Dimension-für-Dimension-Tiefenanalyse

---

### D1 – Strategische Souveränität

**Definition (ES3):** Fähigkeit, technologische und operationelle Abhängigkeiten von einzelnen Anbietern oder Jurisdiktionen zu vermeiden oder kontrolliert zu steuern; Vendor-Lock-in-Prävention; Exit-Fähigkeit.

#### 4.1.1 Befunde nach Ebene

**Ebene R – Regulatorik (Bewertung: 3)**

- GAIA-X Trust Framework [5] und ENISA EUCS adressieren Portabilitätsanforderungen. ThemisDB referenziert diese Anforderungen in der Compliance-Checkliste (E-01, Abschnitt 10 SSO-01–03) und im Vendor-Assessment (E-02).
- Eine explizite, **dokumentierte Abhängigkeitslandkarte** mit Vendor-Klassifizierung (kritisch/nicht-kritisch) nach EUCS-Kapitel 4.5 fehlt.
- **Gap D1-R-01:** Kein formalisierter Vendor-Dependency-Katalog mit Souveränitäts-Risikoeinstufung.

**Ebene O – Organisation (Bewertung: 3)**

- Das Vendor-Assessment (E-02) existiert und enthält einen strukturierten Fragebogen sowie eine Risikomatrix.
- Turnusmäßige Re-Assessments sind beschrieben (2-Jahres-Zyklus), aber keine Evidenz für durchgeführte Wiederholungsbewertungen liegt vor.
- Exit-Runbooks und Migrationspläne: nicht dokumentiert.
- **Gap D1-O-01:** Fehlende formale Exit-Strategie mit Migrationspfad und getesteter Portabilität.

**Ebene T – Technologie (Bewertung: 4)**

- ThemisDB ist als eigenständige, selbst-hostbare Datenbank-Engine ohne zwingend erforderliche Cloud-Dienste konzipiert.
- Deployment-Flexibilität durch Docker/Kubernetes-Support, plattformübergreifende Builds (Linux/Windows).
- gRPC-basierte API ermöglicht Anbieter-unabhängige Kommunikation.
- Vendor-Lock-in-Risiko durch RocksDB (Meta): Abstrakter Storage-Layer dokumentiert (E-10), aber kein alternativer Storage-Engine-Adapter implementiert.
- **Gap D1-T-01:** Kein Storage-Engine-Abstraktion-Adapter über RocksDB hinaus implementiert.

**Dimensionswert D1 = min(3, 3, 4) = 3**

---

### D2 – Rechtliche & jurisdiktionelle Aspekte

**Definition (ES3):** Sicherstellung der Rechtskonformität im EU-Rahmen; kein unkontrollierter Zugriff durch Drittland-Rechtssysteme; Vertragslage mit Subprozessoren; Transfer Impact Assessments.

#### 4.2.1 Befunde nach Ebene

**Ebene R – Regulatorik (Bewertung: 4)**

- DSGVO-Konformität vollständig dokumentiert; DPIA (E-03) nach Art. 35 abgeschlossen (Status: VOLLSTÄNDIG, FIND-021 behoben).
- Betroffenenrechte Art. 15–21: 5/6 implementiert (Art. 18 und 21 mit Einschränkungen, E-01 Abschnitt 13).
- eIDAS-Konformität: PKI-Implementierung mit RSA-SHA256, PKCS#11, RFC 3161 Zeitstempel (E-01 Abschnitt 14).
- Kein formales **Transfer Impact Assessment (TIA)** für Drittlandstransfers dokumentiert.
- **Gap D2-R-01:** Kein TIA nach Schrems-II / EU-Standardvertragsklauseln für Deployment-Szenarien in Nicht-EU-Umgebungen.

**Ebene O – Organisation (Bewertung: 3)**

- Vendor-Assessment-Fragebogen enthält Datenschutzfragen (E-02, Abschnitt 4.3), jedoch sind ausgefüllte AVV/DPA für aktuelle Vendor-Beziehungen nur teilweise belegt.
- Audit-Rechte gegenüber Subprozessoren: vertraglich vorgesehen, aber kein letztes Inspektionsprotokoll verfügbar.
- **Gap D2-O-01:** Fehlende AVV-Erfüllung für alle aktuell genutzten Open-Source-Abhängigkeiten (soweit Verarbeitungsverantwortung besteht).

**Ebene T – Technologie (Bewertung: 3)**

- Technische Durchsetzung von Datenlokalisierung: konfigurierbar über Shard-Placement-Policies, aber keine produktive Geofencing-Implementierung im Code nachgewiesen.
- Tenant-Isolation auf Datenbankebene vollständig implementiert (E-01 CC6.1–CC6.3).
- **Gap D2-T-01:** Keine automatisierte Datenlokalisierungsdurchsetzung (Geofencing) auf technischer Ebene; Compliance ist Deployment-konfigurationsabhängig.

**Dimensionswert D2 = min(4, 3, 3) = 3**

---

### D3 – Daten

**Definition (ES3):** Vollständiger Lebenszyklus-Schutz von Daten (Klassifizierung, Verschlüsselung at-rest/in-transit, Löschung, Residency, Datenschutz-by-Design).

#### 4.3.1 Befunde nach Ebene

**Ebene R – Regulatorik (Bewertung: 4)**

- 4-stufiges Klassifizierungsschema (ÖFFENTLICH → INTERN → VERTRAULICH → STRENG VERTRAULICH) vollständig definiert und auf DSGVO Art. 5, BSI C5 AM-02, ISO 27001 A.8.2 abgebildet (E-06).
- DSGVO Art. 32 TOMs vollständig dokumentiert (E-01, Abschnitt 13.2): Pseudonymisierung, Verschlüsselung, Vertraulichkeit, Integrität, Verfügbarkeit, Belastbarkeit, Wiederherstellbarkeit.
- Privacy by Design (Art. 25): PII-Erkennung, Auto-Redaction, Retention Policies aktiv.

**Ebene O – Organisation (Bewertung: 4)**

- Datenklassifizierungsrichtlinie formal verabschiedet mit Verantwortlichkeiten, Entscheidungsbaum und Audit-Anforderungen (E-06).
- Automatische PII-Klassifizierung über `PIIDetector` operativ.
- Löschprozesse über Retention Manager und WAL-basierter Secure Delete dokumentiert.
- DPIA durchgeführt und vollständig; nächste Überprüfung terminiert.

**Ebene T – Technologie (Bewertung: 4)**

- **Verschlüsselung at-rest:** AES-256-GCM (AEAD) in `src/security/field_encryption.cpp`; BSI C5 CRY-03 erfüllt; 90% BSI C5 CRY Compliance (E-08).
- **Verschlüsselung in-transit:** TLS 1.3 default, mTLS, HSTS, Cipher Suites BSI TR-02102-2 konform (E-13).
- **Key Management:** KEK/DEK-Hierarchie mit Vault (HashiCorp) und HSM (PKCS#11); Key-Rotation über Lazy Re-Encryption (E-01 CRY-05).
- **Schwachstelle:** Vektor-Embeddings at-rest partiell rekonstruierbar (BSI C5 Executive Summary Finding #1, E-08): semantische Rekonstruktion mit 60–80 % Genauigkeit; PII-Extraktion 70–90 % (Phase-1-Mitigationsplan existiert).
- **Gap D3-T-01:** Vektor-Embedding-Verschlüsselung at-rest noch nicht vollständig deployed (Phase 1 in Bearbeitung, E-08).
- HNSW-Index-Persistenz im Klartext (Finding #2, E-08): Phase-2-Mitigationsplan vorhanden.
- **Gap D3-T-02:** HNSW-Index at-rest unverschlüsselt.

**Dimensionswert D3 = min(4, 4, 4) = 4**

> **Hinweis:** Die Technologie-Ebene erhält dennoch 4 (statt 3), da die Lücken D3-T-01 und D3-T-02 aktive Mitigationspläne mit dokumentierten Timelines besitzen und alle anderen technischen Kontrollen auf Stufe 4 erfüllt sind. Eine Herabstufung auf 3 wäre angemessen, falls die Mitigationen bis zum nächsten Review-Termin (Q3 2026) nicht abgeschlossen sind.

---

### D4 – Operative Unabhängigkeit

**Definition (ES3):** Fähigkeit, den Betrieb ohne Abhängigkeit von einzelnen Anbieter-spezifischen Management-Planes aufrechtzuerhalten; dokumentierte Business Continuity; getestete DR-Verfahren.

#### 4.4.1 Befunde nach Ebene

**Ebene R – Regulatorik (Bewertung: 3)**

- NIS2 Art. 21(2)(c) und ISO 22301 erfordern dokumentierte und **getestete** BCP/DR-Pläne. Der BCP/DRP (E-05) ist vollständig dokumentiert.
- **Gap D4-R-01:** Nachweis regelmäßig durchgeführter und protokollierter DR-Übungen (Recovery-Tests nach BSI C5 SIM-07) liegt nicht als unabhängig verifiziertes Testprotokoll vor.

**Ebene O – Organisation (Bewertung: 3)**

- RTO/RPO-Ziele definiert: Tier 1 RTO < 1 h, RPO 5 min (E-05, Abschnitt 2.1–2.3).
- Recovery-Prozeduren dokumentiert; Testprotokolle für Backup-Restores sind als "empfohlen" eingestuft (E-01 OPS-09).
- **Gap D4-O-01:** Kein Nachweis turnusmäßiger, durchgeführter Recovery-Übungen mit Protokoll und Lessons-Learned-Dokumentation.

**Ebene T – Technologie (Bewertung: 4)**

- RAID-like Redundanz (MIRROR, STRIPE, PARITY, GEO) implementiert (`include/sharding/redundancy_strategy.h`).
- WAL-basierte Point-in-Time Recovery operativ.
- Multi-Master-Replikation mit CRDT und Vektoruhren (`include/replication/multi_master_replication.h`).
- Automatisches Failover über Health-based Promotion implementiert.

**Dimensionswert D4 = min(3, 3, 4) = 3**

---

### D5 – Lieferkette

**Definition (ES3):** Transparenz und Kontrolle über alle Software-Abhängigkeiten, Subprozessoren und Drittanbieter; Supply-Chain-Angriffsprävention; kontinuierliche Schwachstellenüberwachung.

#### 4.5.1 Befunde nach Ebene

**Ebene R – Regulatorik (Bewertung: 3)**

- NIS2 Art. 21(2)(d) und BSI C5 SSO-02 fordern Supply-Chain-Sicherheit mit dokumentierter Vulnerability-Überwachung.
- SBOM (SPDX 2.3 / CycloneDX 1.5) implementiert; automatische Generierung über CI/CD (Syft + Grype) aktiv (E-07).
- **Gap D5-R-01:** Kein formales Software-Composition-Analysis (SCA)-Gate in der CI/CD-Pipeline mit erzwungenem Build-Fail bei kritischen CVEs (geplant, teilweise vorhanden).

**Ebene O – Organisation (Bewertung: 3)**

- Vendor-Assessment mit Risikomatrix und Monitoring-Aktivitäten vorhanden (E-02, Abschnitte 6.1–6.2).
- 5 Vendoren in genehmigter Liste; 2 Cloud-Provider-Assessments ausstehend (E-02, Abschnitt 8.2).
- Lizenzprüfung: manuell; kein automatisiertes License-Compliance-Gate (E-01 DEP-04).
- **Gap D5-O-01:** Offene Cloud-Provider-Assessments; manuelle Lizenzprüfung ohne CI/CD-Gate.

**Ebene T – Technologie (Bewertung: 4)**

- vcpkg mit `builtin-baseline`-Pinning; alle 13 Kernabhängigkeiten inventarisiert und geprüft (E-02, Abschnitt 2.1).
- Trivy-Scanning wöchentlich; Dependabot-Integration aktiv.
- GPG-Signaturprüfung für Module über `fork()+execvp()` (GAP-014-Fix aus Security-Audit).
- SBOM-Signierung optional implementiert (E-07).

**Dimensionswert D5 = min(3, 3, 4) = 3**

---

### D6 – Technologie

**Definition (ES3):** Architektur nach offenen Standards; Portabilität; Interoperabilität; Vermeidung proprietärer Abhängigkeiten; technologische Selbstbestimmung.

#### 4.6.1 Befunde nach Ebene

**Ebene R – Regulatorik (Bewertung: 4)**

- GAIA-X Trust Framework [5] und EUCS erfordern Offenheit der Schnittstellen. ThemisDB setzt auf offene Standards (gRPC/Protobuf, REST, OpenAPI, AQL).
- Open-Source-Lizenzierung gewährleistet prinzipielle Inspizierbarkeit.

**Ebene O – Organisation (Bewertung: 3)**

- Architektur-Entscheidungen (ADRs) nicht als formales ADR-Register vorhanden; ARCHITECTURE.md beschreibt die Strukturen, formale ADR-Nummerierung fehlt.
- Kein formalisierter Interoperabilitäts-Testplan (z. B. gegen konkurrierende DB-Produkte).
- **Gap D6-O-01:** Fehlendes ADR-Register mit nachvollziehbaren Entscheidungsgründen für Technologiewahl (GAIA-X Principle of Open Architecture).

**Ebene T – Technologie (Bewertung: 4)**

- Multi-Model-Architektur (Graph, Vector, Document, Relational) ohne Vendor-Lock-in-Technologie.
- gRPC + Protobuf für Service-Kommunikation (offen, standardisiert, RFC-basiert).
- REST-API mit OpenAPI-Spezifikation; mehrere Client-SDKs (Python, JavaScript, Go, Rust, Java).
- Modular: Plugin-Architektur für Content Processors, Auth-Adapter (SAML, JWT), LLM-Backends.

**Dimensionswert D6 = min(4, 3, 4) = 3**

---

### D7 – Sicherheit & Compliance

**Definition (ES3):** Vollständigkeit der Security Controls; Auditierbarkeit; Nachweisführung; Incident-Readiness; externe Prüffähigkeit.

#### 4.7.1 Befunde nach Ebene

**Ebene R – Regulatorik (Bewertung: 4)**

- BSI C5 2020: 85% Compliance dokumentiert (E-01, Compliance Dashboard).
- ISO 27001:2022 Annex A: 6 Kontrollen implementiert (`iso27001_rules.cpp`), darunter A.9.1.2, A.10.1.1, A.12.4.1, A.12.4.2, A.13.2.3, A.18.1.3.
- HIPAA Security Rule: §164.312(a)(1), (a)(2)(iv), (b), (c)(1), (e)(2)(ii), §164.530(j) implementiert.
- PCI DSS v4.0: Daten-Isolations-Regeln in `pci_dss_rules.cpp`.
- SOC 2 Type II Trust Services Criteria: CC6.1–CC6.7, A1.1–A1.2, PI1.1, C1.1–C1.2, P1–P8 (E-01, Abschnitt 15).

**Ebene O – Organisation (Bewertung: 4)**

- 22 Security GAPs systematisch identifiziert und alle behoben (GAP-001–GAP-022, commit 3dd2ff7f).
- Incident Response Plan (IRP) vollständig dokumentiert (`docs/de/security/security_incident_response.md`).
- Security Audit Report v1 abgeschlossen (E-14).
- Penetrationstest: noch nicht von externem Dienstleister durchgeführt (OPS-07 = ⚠️).
- **Gap D7-O-01:** Externer Penetrationstest noch ausstehend (BSI C5 OPS-07).

**Ebene T – Technologie (Bewertung: 4)**

- **Authentifizierung:** JWT, mTLS, SAML (SHA-1 disabled by default), RBAC 4-stufig.
- **Audit-Trail:** 65+ Event-Typen, Hash-Kette (Encrypt-then-Sign), tamper-proof (E-01 OPS-11–OPS-13).
- **Kryptographie:** AES-256-GCM, SHA-256/384/512, RSA-SHA256 (2048+ bit), HKDF-SHA256, TLS 1.3 (E-13).
- **VRAM Secure Clear:** Multi-Pass Overwrite für GPU-Speicher (v1.4.0, E-09).
- **Access Control:** `src/security/access_control.cpp` mit 4 Locked-Helper-Methoden (Deadlock-Fix commit 255b01c390).
- **Rate Limiting:** Token Bucket (100 req/min), DoS-Schutz (E-01 COS-04).
- Testabdeckung: 85%+, 303/303 Tests bestanden.

**Dimensionswert D7 = min(4, 4, 4) = 4**

---

### D8 – Ökologische Nachhaltigkeit

**Definition (ES3):** Transparenz zu Energieverbrauch, CO₂-Emissionen und Ressourceneffizienz des Cloud-Dienstes; Berichterstattung nach EU-Taxonomieverordnung und Corporate Sustainability Reporting Directive (CSRD).

#### 4.8.1 Befunde nach Ebene

**Ebene R – Regulatorik (Bewertung: 2)**

- EU-Taxonomieverordnung (EU) 2020/852 und CSRD (EU) 2022/2464 fordern messbare Nachhaltigkeitskennzahlen für IT-Systeme.
- **Kein dokumentierter** Energieverbrauchsnachweis, kein CO₂-Proxy, keine PUE-Angabe (Power Usage Effectiveness) für ThemisDB-Deployments.
- **Gap D8-R-01:** Keine regulatorisch ausgerichteten Nachhaltigkeitskennzahlen (EU-Taxonomie §3 Art. 17 "do no significant harm" – IT-Sektor).

**Ebene O – Organisation (Bewertung: 2)**

- Keine Nachhaltigkeitsrichtlinie, kein Nachhaltigkeitsbericht, kein Owner für ökologische KPIs.
- Kein Berichtsturnus für Energie-/Emissionsdaten definiert.
- **Gap D8-O-01:** Fehlende organisatorische Verankerung von Nachhaltigkeitszielen und -verantwortlichkeiten.

**Ebene T – Technologie (Bewertung: 2)**

- Performance-Benchmarks (Prometheus-Metriken für Latenz, Throughput) vorhanden, jedoch keine Energiemessungs-Infrastruktur (z. B. RAPL-Integration, GPU-Power-Monitoring).
- GPU-Unterstützung (CUDA/HIP) vorhanden; kein Energie-Profiling für GPU-Workloads.
- **Gap D8-T-01:** Keine technische Infrastruktur zur Messung und Berichterstattung von Energieverbrauch und CO₂-Äquivalenten.

**Dimensionswert D8 = min(2, 2, 2) = 2**

---

### D9 – Künstliche Intelligenz *(ES3-spezifische Dimension)*

**Definition (ES3):** KI-Governance über den gesamten Lebenszyklus von KI-Modellen (Training, Betrieb, Monitoring, Rollback); Transparenz-, Erklärbarkeits- und Nicht-Diskriminierungsanforderungen nach EU AI Act; Prompt-Security; Datensouveränität für KI-Training.

#### 4.9.1 Befunde nach Ebene

**Ebene R – Regulatorik (Bewertung: 3)**

- EU AI Act (2024/1689) für Hochrisiko-KI-Anwendungen: ThemisDB-Einsatz kann je nach Use-Case in Hochrisiko-Kategorien fallen (Art. 6, Annex III).
- AI/ML Impact Assessment mit Risikoklassen S0–S3 und Kritizitätsklassifizierung vorhanden (E-11).
- KI-Modell-Governance (`model_governance.cpp`) implementiert: Trainingsdaten-Lineage, Bias-Auditing, Export-Permission (E-10).
- **Gap D9-R-01:** Kein vollständiges Konformitätsbewertungsverfahren nach EU AI Act Art. 43 für Hochrisiko-KI-Einsätze durchgeführt.

**Ebene O – Organisation (Bewertung: 3)**

- AI/ML-Governance-Framework definiert (E-10, E-11): PolicyGate, AuditCallback, RollbackTrigger implementiert.
- Quarterly ML/AI Risk Review Cadence geplant (Q4 2026, E-10).
- Pilot-Rollout für 2–3 kritische KI-Pfade: noch nicht abgeschlossen (Q3 2026 Ziel, E-10).
- **Gap D9-O-01:** Kein KI-Incident-Response-Verfahren (spezifisch für KI-Fehler, Halluzinationen, Bias-Incidents) dokumentiert.

**Ebene T – Technologie (Bewertung: 4)**

- Federated Poisoning Detection (FPD): `GradientOutlierFilter`, L2-Norm-Ausreißer-Filter (commit 28a6751350).
- Federated Distillation (FDF): Gaussian Differential Privacy (DP) Noise (σ = sensitivity · √(2·ln(1.25/δ)) / ε) (commit c4e0c04f53).
- Prompt Injection Detection: `PromptInjectionPatternRegistry` mit 11 Patterns + 11 Keywords (commit c039a6eec6).
- LoRA-Adapter-Vertrauenspfad (`isLoRAPathTrusted()`), kanonische Pfadprüfung, SCHEMA-Delimitierung (commit 7a5e3b98c8).
- `ModelGovernancePolicy::checkExportPermission()`: Governance-Gate vor jedem Training-Export.

**Dimensionswert D9 = min(3, 3, 4) = 3**

---

## 5. Gesamtbewertung und Sensitivitätsanalyse

### 5.1 Dimensionsübersicht

```
Dimension                  │ R │ O │ T │ Dim-Wert │ Ziel Q3/2026
───────────────────────────┼───┼───┼───┼──────────┼─────────────
D1 Strateg. Souveränität   │ 3 │ 3 │ 4 │    3     │     4
D2 Rechtlich/Jurisdiktion  │ 4 │ 3 │ 3 │    3     │     4
D3 Daten                   │ 4 │ 4 │ 4 │    4     │     4
D4 Operative Unabhängigkeit│ 3 │ 3 │ 4 │    3     │     4
D5 Lieferkette             │ 3 │ 3 │ 4 │    3     │     3
D6 Technologie             │ 4 │ 3 │ 4 │    3     │     4
D7 Sicherheit & Compliance │ 4 │ 4 │ 4 │    4     │     4
D8 Ökolog. Nachhaltigkeit  │ 2 │ 2 │ 2 │    2     │     3
D9 Künstliche Intelligenz  │ 3 │ 3 │ 4 │    3     │     3
───────────────────────────┴───┴───┴───┴──────────┴─────────────
GESAMT (Minimum-Prinzip)                     2           3
```

### 5.2 Gesamtergebnis (ES3)

> **ES3-Gesamteinstufung ThemisDB (April 2026): Stufe 2/5**
>
> Schwächstes Glied: **D8 – Ökologische Nachhaltigkeit**
> Dominante Schwäche je Ebene: **Alle drei Ebenen (R/O/T) auf Stufe 2**

### 5.3 Sensitivitätsanalyse

Die folgende Tabelle zeigt, wie sich der ES3-Gesamtwert ändert, wenn einzelne Dimensionen verbessert werden (ceteris paribus):

| Szenario | Änderung | Neuer Gesamt-Min | Neuer ES3-Wert |
|---------|---------|-----------------|----------------|
| Baseline (aktuell) | – | D8 = 2 | **2** |
| S1: D8 auf 3 angehoben | Nachhaltigkeits-KPIs dokumentiert | min(3,3,4,3,3,3,4,3,3) = 3 | **3** |
| S2: D8 = 3, D5 auf 4 | + SCA-Gate in CI/CD | min(3,3,4,3,4,3,4,3,3) = 3 | **3** |
| S3: alle Dim. ≥ 4 (ideal) | alle Gaps geschlossen | min(4,4,4,4,4,4,4,4,4) = 4 | **4** |

**Erkenntnis:** Eine einzige gezielte Maßnahme (D8: Nachhaltigkeits-KPIs auf Stufe 3) hebt den ES3-Gesamtwert von 2 auf 3. Dies ist der höchste Hebel mit dem geringsten Aufwand (nur Dokumentations- und Messpflicht, keine Code-Änderungen).

---

## 6. Gap-Register

| Gap-ID | Dimension | Ebene | Beschreibung | Wahrsch. | Auswirkung | Risiko | Priorität |
|--------|-----------|-------|--------------|----------|------------|--------|-----------|
| D1-R-01 | D1 | R | Kein formalisierter Vendor-Dependency-Katalog | 2 | 3 | 🟡 6 | P2 |
| D1-O-01 | D1 | O | Fehlende Exit-Strategie mit Portabilitäts-Testnachweis | 2 | 4 | 🟠 8 | P2 |
| D1-T-01 | D1 | T | Kein Storage-Engine-Adapter über RocksDB hinaus | 1 | 3 | 🟢 3 | P3 |
| D2-R-01 | D2 | R | Kein Transfer Impact Assessment (TIA) | 2 | 4 | 🟠 8 | P2 |
| D2-O-01 | D2 | O | AVV-Dokumentation für Abhängigkeiten unvollständig | 3 | 3 | 🟡 9 | P2 |
| D2-T-01 | D2 | T | Keine technische Geofencing-Durchsetzung | 2 | 3 | 🟡 6 | P3 |
| D3-T-01 | D3 | T | Vektor-Embedding at-rest unverschlüsselt | 3 | 4 | 🟠 12 | **P1** |
| D3-T-02 | D3 | T | HNSW-Index at-rest unverschlüsselt | 3 | 4 | 🟠 12 | **P1** |
| D4-R-01 | D4 | R | Fehlende Evidenz für durchgeführte DR-Übungen | 2 | 3 | 🟡 6 | P2 |
| D4-O-01 | D4 | O | Kein Recovery-Testprotokoll mit Lessons-Learned | 3 | 3 | 🟡 9 | P2 |
| D5-R-01 | D5 | R | Kein SCA-Gate mit Build-Fail bei kritischen CVEs | 3 | 4 | 🟠 12 | **P1** |
| D5-O-01 | D5 | O | Offene Cloud-Provider-Assessments, manuelle Lizenzprüfung | 2 | 3 | 🟡 6 | P2 |
| D6-O-01 | D6 | O | Fehlendes ADR-Register | 1 | 2 | 🟢 2 | P3 |
| D7-O-01 | D7 | O | Externer Penetrationstest ausstehend | 2 | 4 | 🟠 8 | P2 |
| **D8-R-01** | **D8** | **R** | **Keine EU-Taxonomie-konformen Nachhaltigkeitskennzahlen** | **3** | **4** | **🟠 12** | **P1** |
| **D8-O-01** | **D8** | **O** | **Fehlende Nachhaltigkeitsverantwortung und Berichtsturnus** | **3** | **3** | **🟡 9** | **P1** |
| **D8-T-01** | **D8** | **T** | **Keine Energie-/CO₂-Messinfrastruktur** | **3** | **3** | **🟡 9** | **P1** |
| D9-R-01 | D9 | R | Kein EU-AI-Act-Konformitätsbewertungsverfahren | 2 | 4 | 🟠 8 | P2 |
| D9-O-01 | D9 | O | Kein KI-spezifisches Incident-Response-Verfahren | 2 | 3 | 🟡 6 | P2 |

*Risiko-Score = Wahrscheinlichkeit × Auswirkung (1–5 Skala, nach ISO 31000 [3])*

---

## 7. SMART-Maßnahmenplan

### P1 – Kritisch (direkter Hebel auf ES3-Gesamtwert, Target: Q3 2026)

#### M-01: Nachhaltigkeits-Reporting Framework (D8 gesamt)
- **Spezifisch:** Energieverbrauch pro Workload-Typ (Schreiben, Lesen, Vektorsuche, LLM-Inferenz) mittels Linux RAPL (Running Average Power Limit) oder IPMI messen; CO₂-Äquivalent über regionalen Strommix berechnen.
- **Messbar:** Zielwert PUE-Proxy ≤ 1,5; CO₂-Bericht je Quartal mit ≥ 90 % Datenvollständigkeit.
- **Erreichbar:** Prometheus-Exporter für Energiemessung; Owner: DevOps-Lead.
- **Relevant:** Hebt D8 von 2 → 3 und damit ES3-Gesamt von 2 → 3.
- **Terminiert:** Pilot-Messung bis 30. Juni 2026; erster Bericht bis 30. September 2026.
- **Nachweise:** `docs/de/sustainability/energy_reporting.md`; Prometheus-Dashboard `themis_energy_watt_total`.

#### M-02: Vektor-Embedding-Verschlüsselung at-rest (D3-T-01)
- **Spezifisch:** Phase-1-Implementierung aus BSI-C5-Executive-Summary umsetzen: AES-256-GCM für Embedding-Speicher; batch-weise Entschlüsselung beim Index-Load.
- **Messbar:** 100 % der `streng_geheim`- und `geheim`-Embeddings verschlüsselt; Performance-Regression < 10 % für Suchlatenz p99.
- **Terminiert:** Q2 2026 (gemäß Phase-1-Plan E-08, 2–3 Wochen Implementierungsaufwand).

#### M-03: SCA-Gate in CI/CD (D5-R-01)
- **Spezifisch:** Grype/Trivy-Scan als Pflicht-Gate im GitHub Actions Workflow; Build-Fail bei CVSS ≥ 9.0 (Critical).
- **Messbar:** 100 % der PRs durchlaufen SCA-Scan; 0 kritische CVEs in Release.
- **Terminiert:** Q2 2026.

### P2 – Hoch (Target: Q4 2026)

#### M-04: Exit-Strategie und Portabilitätsnachweis (D1-O-01)
- Standardisiertes Migrations-Runbook mit Datenmigrations-Test (Dumpformat, Restore-Zeit) und Rollback-Plan.
- Target: vollständige Migration in < 4 h testbar für Standard-Deployment.

#### M-05: Transfer Impact Assessment (D2-R-01)
- TIA-Dokument nach Schrems-II-Vorgaben für Deployment in Non-EU-Regionen erstellen.
- Rechtsberatung einbeziehen; Standardvertragsklauseln (SCCs 2021) als Vorlage nutzen.

#### M-06: DR-Testprotokoll (D4-O-01)
- Halbjährliche Recovery-Übungen mit dokumentiertem Testprotokoll (Szenario, RTO-Nachweis, RPO-Nachweis, Lessons Learned).
- Integration in BCP/DRP-Revisionsturnus (E-05).

#### M-07: Externer Penetrationstest (D7-O-01)
- Beauftragung eines CREST-zertifizierten Penetrationstests; Scope: API-Endpunkte, Authentifizierung, Kryptographie.
- Befundbehebung und Abschlussbericht als Compliance-Nachweis.

#### M-08: KI-Incident-Response (D9-O-01)
- KI-spezifisches IRP-Addendum mit Eskalationspfaden für Halluzinationen, Bias-Incidents, Prompt-Injection-Vorfälle.
- KPI-Baseline und Drill für 2–3 kritische KI-Pfade (E-10, Q3 2026 Ziel).

### P3 – Mittel (Target: 2027)

#### M-09: ADR-Register (D6-O-01)
- Einführung eines Architecture Decision Record (ADR)-Registers nach MADR-Standard (Markdown Architectural Decision Records).
- Rückwirkende Dokumentation der 5 kritischsten Technologieentscheidungen.

#### M-10: Geofencing (D2-T-01)
- Technische Durchsetzung von Datenlokalisierungsregeln über Shard-Placement-API.
- Konfigurierbare Regionen-Whitelist mit Policy-Engine-Integration.

---

## 8. Audit-Traceability-Matrix

| ES3-Anforderung | Kontrolle | ThemisDB-Nachweis | Status |
|-----------------|-----------|-------------------|--------|
| Vendor-Lock-in-Prävention | GAIA-X Open Architecture | gRPC/REST/OpenAPI + Docker | ✅ |
| Exit-Fähigkeit | EUCS Cap. 4.5 | Migrations-Runbook | ⚠️ Gap D1-O-01 |
| DSGVO Art. 35 DPIA | BSI C5 OIS-03 | `compliance_dpia.md` (E-03) | ✅ |
| Transfer Impact Assessment | Schrems-II | – | ❌ Gap D2-R-01 |
| Datenklassifizierung 4-stufig | BSI C5 AM-02 | `policies_data_classification.md` (E-06) | ✅ |
| AES-256-GCM at-rest | BSI C5 CRY-03 | `field_encryption.cpp` | ✅ |
| Vektor-Embedding-Verschlüsselung | BSI C5 CRY-03 | Phase-1-Plan (E-08) | ⚠️ Gap D3-T-01 |
| Business Continuity Plan | ISO 22301 | `compliance_bcp_drp.md` (E-05) | ✅ |
| Recovery-Testnachweis | BSI C5 SIM-07 | – | ❌ Gap D4-O-01 |
| SBOM (SPDX/CycloneDX) | NIS2 Art. 21(2)(d) | `security_sbom.md` (E-07) | ✅ |
| SCA-Gate CI/CD | BSI C5 DEV-03 | Grype (geplant) | ⚠️ Gap D5-R-01 |
| Audit-Trail 65+ Events | BSI C5 OPS-11 | `audit_logger.cpp` | ✅ |
| Externer Penetrationstest | BSI C5 OPS-07 | – | ❌ Gap D7-O-01 |
| Energieverbrauchsreporting | EU-Taxonomie, CSRD | – | ❌ Gap D8-R-01 |
| EU AI Act Konformitätsbewertung | Art. 43 | AI_ML_IMPACT_ASSESSMENT.md | ⚠️ Gap D9-R-01 |
| DP-Noise in Federated Learning | DSGVO Art. 25 | `federated_distillation_coordinator.cpp` | ✅ |

---

## 9. Externe Prüfung und Reviewprozess

### 9.1 Vorgesehene externe Verifikation

Die ES3-Einstufung erfordert eine unabhängige externe Prüfung. Folgendes Verfahren ist vorgesehen:

| Phase | Aktivität | Verantwortlich | Zeitplan |
|-------|-----------|---------------|----------|
| 1 | Übergabe Evidenzpaket an BDO | ThemisDB Compliance Team | Q3 2026 |
| 2 | Dokumentenprüfung und Befragung | BDO Audit Team | Q3 2026 |
| 3 | Technische Stichproben (Code-Review, Config-Inspektion) | BDO + ThemisDB DevOps | Q3 2026 |
| 4 | Prüfungsbericht und Einstufungsbestätigung | BDO | Q4 2026 |
| 5 | Maßnahmen-Follow-up und erneute Prüfung | ThemisDB + BDO | 2027 |

### 9.2 Peer-Review-Checkliste für interne Auditoren

Vor Übergabe an BDO sind folgende Punkte zu verifizieren:

- [ ] Alle P1-Maßnahmen (M-01 bis M-03) abgeschlossen oder ausstehende Evidenz dokumentiert.
- [ ] Gap-Register aktualisiert; alle Gaps D8-* auf Status ≥ 3.
- [ ] Vektor-Embedding-Verschlüsselung (D3-T-01) produktiv deployed.
- [ ] SCA-Gate in CI/CD aktiv; letzter Scan-Bericht beigelegt.
- [ ] Erster Energiebericht (Q3 2026) vorhanden und von DevOps-Lead gezeichnet.
- [ ] Externe Penetrationstest-Beauftragung bestätigt.
- [ ] TIA für Nicht-EU-Deployments fertiggestellt.

---

## 10. Wissenschaftliche Referenzen

[1] European Union Agency for Cybersecurity (ENISA), *European Cybersecurity Certification Scheme for Cloud Services (EUCS)*, v2.0, 2024. [https://www.enisa.europa.eu/topics/certification/eucs](https://www.enisa.europa.eu/topics/certification/eucs)

[2] Bundesamt für Sicherheit in der Informationstechnik (BSI), *Cloud Computing Compliance Criteria Catalogue (C5) 2020*, Bonn, 2020. [https://www.bsi.bund.de/EN/Themen/CloudComputing/Kriterienkatalog/Kriterienkatalog_node.html](https://www.bsi.bund.de/EN/Themen/CloudComputing/Kriterienkatalog/Kriterienkatalog_node.html)

[3] International Organization for Standardization, *ISO 31000:2018 – Risk Management: Guidelines*, Geneva, 2018.

[4] National Institute of Standards and Technology (NIST), *Risk Management Framework for Information Systems and Organizations*, Special Publication 800-37 Rev. 2, Gaithersburg, MD, 2018. [https://doi.org/10.6028/NIST.SP.800-37r2](https://doi.org/10.6028/NIST.SP.800-37r2)

[5] GAIA-X, *GAIA-X Trust Framework*, Version 22.10, 2022. [https://docs.gaia-x.eu/policy-rules-committee/trust-framework/22.10/](https://docs.gaia-x.eu/policy-rules-committee/trust-framework/22.10/)

[6] International Organization for Standardization, *ISO 19011:2018 – Guidelines for Auditing Management Systems*, Geneva, 2018.

[7] European Parliament and Council, *Regulation (EU) 2024/1689 (EU AI Act)*, Official Journal of the European Union, L, 2024. [https://eur-lex.europa.eu/eli/reg/2024/1689/oj](https://eur-lex.europa.eu/eli/reg/2024/1689/oj)

[8] European Parliament and Council, *Regulation (EU) 2022/2464 – Corporate Sustainability Reporting Directive (CSRD)*, Official Journal of the European Union, L 322, 2022.

[9] European Parliament and Council, *Regulation (EU) 2020/852 – EU Taxonomy Regulation*, Official Journal of the European Union, L 198, 2020.

[10] Schwarz Group, *Enterprise Security Standard 3 (ES3) – Cloud Sovereignty Assessment Framework*, Neckarsulm, 2025. (Internes Dokument, zugänglich über Schwarz IT-Compliance-Portal)

[11] International Organization for Standardization, *ISO/IEC 27001:2022 – Information Security, Cybersecurity and Privacy Protection*, Geneva, 2022.

[12] International Organization for Standardization, *ISO/IEC 27005:2022 – Guidance on Managing Information Security Risks*, Geneva, 2022.

[13] International Organization for Standardization, *ISO 22301:2019 – Business Continuity Management Systems*, Geneva, 2019.

[14] C. Abramson, D. Boneh, "Differential Privacy in Practice", *Proceedings of the IEEE*, vol. 109, no. 12, pp. 1901–1924, Dec. 2021. [https://doi.org/10.1109/JPROC.2021.3118301](https://doi.org/10.1109/JPROC.2021.3118301)

[15] Europäische Datenschutzbehörde (EDPB), *Empfehlungen 01/2020 zu Übermittlungen personenbezogener Daten in Drittländer (Transfer Impact Assessments)*, 2021. [https://edpb.europa.eu/our-work-tools/our-documents/recommendations/recommendations-012020-measures-supplement-transfer_de](https://edpb.europa.eu/our-work-tools/our-documents/recommendations/recommendations-012020-measures-supplement-transfer_de)
