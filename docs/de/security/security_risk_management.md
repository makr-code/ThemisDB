# ThemisDB - Risikomanagement-Framework

**Version:** v1.3.0  
**Stand:** 6. April 2026  
**Klassifizierung:** Intern  
**BSI C5 Referenz:** OIS-03  
**ISO 27001 Referenz:** 6.1, 8.2  
**Kategorie:** 🔒 Security

---

## 📑 Inhaltsverzeichnis

- [1. Einleitung](#1-einleitung)
- [2. Risikomanagement-Prozess](#2-risikomanagement-prozess)
- [3. Risiko-Identifikation](#3-risiko-identifikation)
- [4. Risiko-Analyse](#4-risiko-analyse)
- [5. Risikobewertung](#5-risikobewertung)

---

## 1. Einleitung

### 1.1 Zweck

Dieses Dokument definiert das Risikomanagement-Framework für ThemisDB, einschließlich der Methodik zur Identifikation, Bewertung und Behandlung von Informationssicherheitsrisiken.

### 1.2 Geltungsbereich

- ThemisDB-Software und -Infrastruktur
- Entwicklungsprozesse
- Betriebsumgebungen
- Zulieferer und Abhängigkeiten

### 1.3 Referenzen

- ISO/IEC 27005:2022 - Information security risk management
- BSI-Standard 200-3 - Risikoanalyse auf der Basis von IT-Grundschutz
- NIST SP 800-30 - Guide for Conducting Risk Assessments

---

## 2. Risikomanagement-Prozess

### 2.1 Prozessübersicht

```
┌─────────────────────────────────────────────────────────────┐
│                    RISIKOMANAGEMENT-PROZESS                 │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐     │
│  │   KONTEXT   │───►│IDENTIFIKATION│───►│  ANALYSE   │     │
│  └─────────────┘    └─────────────┘    └─────────────┘     │
│         │                                     │             │
│         │                                     ▼             │
│         │          ┌─────────────┐    ┌─────────────┐      │
│         │          │   MONITOR   │◄───│  BEWERTUNG  │      │
│         │          └─────────────┘    └─────────────┘      │
│         │                 │                  │             │
│         │                 │                  ▼             │
│         │                 │          ┌─────────────┐       │
│         └─────────────────┼─────────►│ BEHANDLUNG  │       │
│                           │          └─────────────┘       │
│                           │                  │             │
│                           │                  ▼             │
│                           │          ┌─────────────┐       │
│                           └─────────►│KOMMUNIKATION│       │
│                                      └─────────────┘       │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 Rollen und Verantwortlichkeiten

| Rolle | Verantwortlichkeiten |
|-------|---------------------|
| **Risk Owner** | Entscheidung über Risikobehandlung |
| **Risk Manager** | Koordination des Risikomanagements |
| **Security Lead** | Fachliche Risikobewertung |
| **Asset Owner** | Identifikation asset-spezifischer Risiken |

---

## 3. Risiko-Identifikation

### 3.1 Asset-Kategorien

| Kategorie | Beispiele | Kritikalität |
|-----------|-----------|--------------|
| **Daten** | Kundendaten, Konfiguration, Keys | Kritisch |
| **Software** | ThemisDB Core, APIs, SDKs | Hoch |
| **Infrastruktur** | Server, Storage, Netzwerk | Hoch |
| **Dokumentation** | Source Code, Docs, Credentials | Mittel |
| **Personal** | Entwickler, Maintainer | Mittel |

### 3.2 Bedrohungskatalog

#### 3.2.1 Technische Bedrohungen

| ID | Bedrohung | Beschreibung |
|----|-----------|--------------|
| T-01 | SQL/AQL Injection | Einschleusen von Schadcode über Queries |
| T-02 | Authentication Bypass | Umgehung der Authentifizierung |
| T-03 | Privilege Escalation | Unberechtigte Rechteerweiterung |
| T-04 | Data Exfiltration | Unberechtigte Datenextraktion |
| T-05 | Denial of Service | Verfügbarkeitsangriffe |
| T-06 | Cryptographic Failures | Schwache oder fehlerhafte Kryptographie |
| T-07 | Insecure Deserialization | Angriffe über Deserialisierung |
| T-08 | SSRF | Server-Side Request Forgery |
| T-09 | Path Traversal | Zugriff auf unerlaubte Dateipfade |
| T-10 | Memory Corruption | Buffer Overflow, Use-After-Free |

#### 3.2.2 Operative Bedrohungen

| ID | Bedrohung | Beschreibung |
|----|-----------|--------------|
| O-01 | Konfigurationsfehler | Unsichere Standardkonfiguration |
| O-02 | Patch-Management | Ungepatchte Schwachstellen |
| O-03 | Backup-Verlust | Datenverlust durch Backup-Fehler |
| O-04 | Insider-Bedrohung | Böswilliger oder nachlässiger Mitarbeiter |
| O-05 | Supply Chain | Kompromittierte Dependencies |

#### 3.2.3 Umweltbedrohungen

| ID | Bedrohung | Beschreibung |
|----|-----------|--------------|
| E-01 | Hardware-Ausfall | Server-, Storage-, Netzwerkausfall |
| E-02 | Stromausfall | Unterbrechung der Stromversorgung |
| E-03 | Naturkatastrophe | Brand, Überschwemmung, Erdbeben |
| E-04 | Pandemie | Personelle Engpässe |

### 3.3 Schwachstellen-Kategorien

| Kategorie | Beispiele |
|-----------|-----------|
| **Software** | Bugs, Design-Fehler, fehlende Validierung |
| **Konfiguration** | Schwache Defaults, offene Ports |
| **Prozess** | Fehlende Reviews, unklare Zuständigkeiten |
| **Personal** | Fehlendes Wissen, Social Engineering |
| **Physisch** | Ungesicherter Zugang, fehlende Redundanz |

---

## 4. Risiko-Analyse

### 4.1 Bewertungskriterien

#### 4.1.1 Eintrittswahrscheinlichkeit

| Stufe | Bezeichnung | Beschreibung | Häufigkeit |
|-------|-------------|--------------|------------|
| 1 | Sehr niedrig | Extrem unwahrscheinlich | < 1x / 10 Jahre |
| 2 | Niedrig | Unwahrscheinlich | 1x / 5 Jahre |
| 3 | Mittel | Möglich | 1x / Jahr |
| 4 | Hoch | Wahrscheinlich | 1x / Quartal |
| 5 | Sehr hoch | Sehr wahrscheinlich | 1x / Monat |

#### 4.1.2 Schadensauswirkung

| Stufe | Bezeichnung | Finanziell | Reputation | Compliance |
|-------|-------------|------------|------------|------------|
| 1 | Vernachlässigbar | < €1.000 | Keine | Keine |
| 2 | Gering | €1.000-10.000 | Lokal | Minor |
| 3 | Mittel | €10.000-100.000 | Regional | Aufsicht |
| 4 | Hoch | €100.000-1M | National | Bußgeld |
| 5 | Kritisch | > €1M | International | Straf |

### 4.2 Risikomatrix

|  | **Auswirkung** |||||
|--|--|--|--|--|--|
| **Wahrscheinlichkeit** | 1-Vernachl. | 2-Gering | 3-Mittel | 4-Hoch | 5-Kritisch |
| **5-Sehr hoch** | Mittel | Mittel | Hoch | Kritisch | Kritisch |
| **4-Hoch** | Niedrig | Mittel | Mittel | Hoch | Kritisch |
| **3-Mittel** | Niedrig | Niedrig | Mittel | Mittel | Hoch |
| **2-Niedrig** | Niedrig | Niedrig | Niedrig | Mittel | Mittel |
| **1-Sehr niedrig** | Niedrig | Niedrig | Niedrig | Niedrig | Mittel |

### 4.3 Risikoklassen

| Klasse | Beschreibung | Behandlung |
|--------|--------------|------------|
| **Kritisch** | Sofortiges Handeln erforderlich | Sofort mitigieren oder vermeiden |
| **Hoch** | Priorisierte Behandlung | Innerhalb 30 Tagen behandeln |
| **Mittel** | Geplante Behandlung | Innerhalb 90 Tagen behandeln |
| **Niedrig** | Akzeptanz oder Monitoring | Beobachten, dokumentieren |

---

## 5. Risikobewertung

### 5.1 Identifizierte Risiken

| ID | Risiko | Bedrohung | Schwachstelle | W | A | Stufe |
|----|--------|-----------|---------------|---|---|-------|
| R-01 | AQL Injection | T-01 | Unvalidierte Eingaben | 3 | 4 | Mittel |
| R-02 | Auth Bypass | T-02 | Token-Schwächen | 2 | 5 | Mittel |
| R-03 | Datenleck | T-04 | Fehlende Verschlüsselung | 2 | 5 | Mittel |
| R-04 | DoS | T-05 | Fehlende Rate Limits | 3 | 3 | Mittel |
| R-05 | Key Exposure | T-06 | Unsichere Speicherung | 2 | 5 | Mittel |
| R-06 | Supply Chain | O-05 | Ungepatchte Dependencies | 3 | 4 | Mittel |
| R-07 | Backup-Verlust | O-03 | Fehlende Backup-Tests | 2 | 4 | Mittel |
| R-08 | Insider | O-04 | Überprivilegierung | 2 | 4 | Mittel |
| R-09 | Hardware-Ausfall | E-01 | Single Point of Failure | 3 | 4 | Mittel |
| R-10 | Datenkorruption | T-10 | Memory-Bugs | 2 | 4 | Mittel |

### 5.2 Detaillierte Risikobewertungen

#### R-01: AQL Injection

| Attribut | Bewertung |
|----------|-----------|
| **Bedrohung** | Einschleusen von Schadcode über AQL-Queries |
| **Schwachstelle** | Unzureichende Eingabevalidierung |
| **Wahrscheinlichkeit** | 3 (Mittel) - API öffentlich zugänglich |
| **Auswirkung** | 4 (Hoch) - Datenexfiltration möglich |
| **Risikostufe** | Mittel |
| **Bestehende Kontrollen** | Parameterized Queries, Input Validation |
| **Empfohlene Kontrollen** | WAF, zusätzliches Fuzzing |

#### R-06: Supply Chain Attack

| Attribut | Bewertung |
|----------|-----------|
| **Bedrohung** | Kompromittierte Abhängigkeit |
| **Schwachstelle** | Verzögerte Updates, fehlende SBOM-Prüfung |
| **Wahrscheinlichkeit** | 3 (Mittel) - Angriffe auf Open-Source nehmen zu |
| **Auswirkung** | 4 (Hoch) - Code-Ausführung möglich |
| **Risikostufe** | Mittel |
| **Bestehende Kontrollen** | vcpkg mit gepinnter Baseline |
| **Empfohlene Kontrollen** | Automatische Vulnerability-Scans, SBOM |

---

## 6. Risikobehandlung

### 6.1 Behandlungsoptionen

| Option | Beschreibung | Anwendung |
|--------|--------------|-----------|
| **Vermeiden** | Risiko vollständig eliminieren | Deaktivierung unsicherer Features |
| **Mitigieren** | Wahrscheinlichkeit/Auswirkung reduzieren | Sicherheitskontrollen implementieren |
| **Transferieren** | Risiko auf Dritte übertragen | Versicherung, SLA |
| **Akzeptieren** | Risiko bewusst tragen | Dokumentierte Akzeptanz |

### 6.2 Behandlungsplan

| Risiko | Behandlung | Maßnahme | Verantwortlich | Frist |
|--------|------------|----------|----------------|-------|
| R-01 | Mitigieren | WAF-Integration prüfen | Security Lead | Q1 2026 |
| R-02 | Mitigieren | Token-Rotation implementieren | Dev Team | Q1 2026 |
| R-04 | Mitigieren | Rate Limiting ✅ implementiert | Dev Team | ✅ Erledigt |
| R-05 | Mitigieren | Vault/HSM Integration ✅ | Dev Team | ✅ Erledigt |
| R-06 | Mitigieren | Trivy-Integration | DevOps | Q1 2026 |
| R-07 | Mitigieren | Automatische Backup-Tests | DBA | Q1 2026 |
| R-09 | Mitigieren | HA/Replication ✅ | Dev Team | ✅ Erledigt |

### 6.3 Risikoakzeptanz

Folgende Risiken werden bewusst akzeptiert:

| Risiko | Begründung | Akzeptiert durch | Datum |
|--------|------------|------------------|-------|
| Keine akzeptierten Risiken derzeit | - | - | - |

---

## 7. Risiko-Monitoring

### 7.1 Key Risk Indicators (KRI)

| KRI | Beschreibung | Schwellenwert | Häufigkeit |
|-----|--------------|---------------|------------|
| Failed Login Attempts | Fehlgeschlagene Anmeldeversuche | > 100/h | Real-time |
| CVE Count | Bekannte Schwachstellen in Dependencies | > 0 Critical | Täglich |
| Audit Log Anomalies | Ungewöhnliche Aktivitäten | Baseline +50% | Real-time |
| Backup Success Rate | Erfolgreiche Backups | < 99% | Täglich |
| Incident Count | Sicherheitsvorfälle | > 0 Critical/Monat | Monatlich |

### 7.2 Review-Zyklus

| Review-Typ | Häufigkeit | Teilnehmer | Output |
|------------|------------|------------|--------|
| Operatives Review | Monatlich | Security Lead, Ops | KRI-Report |
| Strategisches Review | Quartalsweise | Management | Risiko-Dashboard |
| Vollständige Neubewertung | Jährlich | alle Stakeholder | Aktualisiertes Register |
| Ad-hoc Review | Bei Incidents | Incident Team | Incident-Report |

---

## 8. Compliance-Mapping

| Risikomanagement-Element | BSI C5 | ISO 27001 | NIST CSF |
|--------------------------|--------|-----------|----------|
| Risiko-Identifikation | OIS-03 | 6.1.2 | ID.RA-1 |
| Risiko-Analyse | OIS-03 | 8.2 | ID.RA-2 |
| Risiko-Bewertung | OIS-03 | 8.2 | ID.RA-3 |
| Risiko-Behandlung | OIS-03 | 6.1.3 | ID.RA-6 |
| Risiko-Akzeptanz | OIS-03 | 6.1.3 | ID.RM-1 |

---

## 9. Anhänge

### A. Risiko-Register-Template

```csv
ID,Bezeichnung,Kategorie,Bedrohung,Schwachstelle,Wahrscheinlichkeit,Auswirkung,Stufe,Behandlung,Status,Owner,Frist
R-XX,Beschreibung,Kategorie,T-XX,Schwachstelle,1-5,1-5,Stufe,Behandlung,Status,Name,YYYY-MM-DD
```

### B. Referenzierte Dokumente

| Dokument | Pfad |
|----------|------|
| Threat Model | `docs/security/threat_model.md` |
| Incident Response Plan | `docs/security/INCIDENT_RESPONSE_PLAN.md` |
| Security Policy | `docs/security/INFORMATION_SECURITY_POLICY.md` |
| Audit Checklist | `docs/FULL_AUDIT_CHECKLIST.md` |

---

## 10. Änderungshistorie

| Version | Datum | Autor | Änderungen |
|---------|-------|-------|------------|
| 1.0 | Dezember 2025 | ThemisDB Team | Erstversion |

---

**Letzte Aktualisierung:** Dezember 2025  
**Nächstes Review:** März 2026  
**Dokumentverantwortlicher:** ThemisDB Security Team
