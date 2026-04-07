# ThemisDB – Risk Register

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🔒 Compliance  
**Klassifizierung:** Vertraulich  
**Basis:** ISO 31000, ISO 27005, BSI Standard 200-3, NIST RMF

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#-übersicht)
- [Risiko-Kategorien](#1-risiko-kategorien)
- [Bewertungsmethodik](#2-risiko-bewertungsmethodik)

---

## 📋 Übersicht

Das Risk Register dokumentiert alle identifizierten Risiken für ThemisDB, deren Bewertung, Behandlungsmaßnahmen und Status. Es dient als zentrales Instrument für das Risikomanagement gemäß ISO 27001 und BSI IT-Grundschutz.

---

## 1. Risiko-Kategorien

| Kategorie | Beschreibung | Anzahl Risiken |
|-----------|--------------|----------------|
| **TECH** | Technische Risiken | 8 |
| **SEC** | Sicherheitsrisiken | 10 |
| **OPS** | Operationelle Risiken | 6 |
| **COM** | Compliance-Risiken | 5 |
| **BUS** | Geschäftsrisiken | 4 |
| **Gesamt** | | **33** |

---

## 2. Risiko-Bewertungsmethodik

### 2.1 Wahrscheinlichkeit

| Stufe | Bezeichnung | Häufigkeit | Wert |
|-------|-------------|------------|------|
| 1 | Sehr Niedrig | < 1x pro 10 Jahre | 1 |
| 2 | Niedrig | 1x pro 1-10 Jahre | 2 |
| 3 | Mittel | 1x pro Jahr | 3 |
| 4 | Hoch | 1x pro Monat | 4 |
| 5 | Sehr Hoch | 1x pro Woche | 5 |

### 2.2 Auswirkung

| Stufe | Bezeichnung | Beschreibung | Wert |
|-------|-------------|--------------|------|
| 1 | Unbedeutend | Kaum messbare Auswirkung | 1 |
| 2 | Gering | Begrenzte Störung, schnell behebbar | 2 |
| 3 | Moderat | Spürbare Beeinträchtigung | 3 |
| 4 | Erheblich | Signifikante Störung, hoher Aufwand | 4 |
| 5 | Katastrophal | Existenzbedrohend, massiver Schaden | 5 |

### 2.3 Risiko-Score

```
Risiko-Score = Wahrscheinlichkeit × Auswirkung

Score 1-4:   🟢 Niedrig     - Akzeptieren
Score 5-9:   🟡 Mittel      - Überwachen
Score 10-14: 🟠 Hoch        - Behandeln
Score 15-25: 🔴 Kritisch    - Sofort behandeln
```

---

## 3. Technische Risiken (TECH)

| ID | Risiko | W | A | Score | Status | Maßnahmen | Owner |
|----|--------|---|---|-------|--------|-----------|-------|
| TECH-001 | RocksDB Korruption | 2 | 5 | 🟠 10 | Behandelt | WAL, Checksums, Backup | Ops |
| TECH-002 | Speicherüberlauf (OOM) | 3 | 3 | 🟡 9 | Behandelt | Memory Limits, Monitoring | Ops |
| TECH-003 | Performance-Degradation | 3 | 3 | 🟡 9 | Behandelt | Compaction, Tuning | Dev |
| TECH-004 | Netzwerkausfall | 3 | 4 | 🟠 12 | Behandelt | Reconnect, Retry, HA | Ops |
| TECH-005 | Hardware-Ausfall | 2 | 4 | 🟡 8 | Behandelt | Backup, RAID, DR | Ops |
| TECH-006 | Third-Party Dependency Bug | 3 | 3 | 🟡 9 | Überwacht | SBOM, Updates | Dev |
| TECH-007 | Skalierungsprobleme | 2 | 3 | 🟡 6 | Überwacht | Sharding (geplant) | Dev |
| TECH-008 | Kompatibilitätsprobleme | 2 | 2 | 🟢 4 | Akzeptiert | API Versioning | Dev |

---

## 4. Sicherheitsrisiken (SEC)

| ID | Risiko | W | A | Score | Status | Maßnahmen | Owner |
|----|--------|---|---|-------|--------|-----------|-------|
| SEC-001 | Unbefugter Datenzugriff | 3 | 5 | 🔴 15 | Behandelt | RBAC, mTLS, Audit | Sec |
| SEC-002 | Ransomware/Malware | 3 | 5 | 🔴 15 | Behandelt | Backup, IRP, Isolation | Sec |
| SEC-003 | DDoS-Angriff | 4 | 3 | 🟠 12 | Behandelt | Rate Limiting, WAF | Ops |
| SEC-004 | SQL/AQL Injection | 2 | 4 | 🟡 8 | Behandelt | Input Validation, Prepared | Dev |
| SEC-005 | Credential Theft | 3 | 4 | 🟠 12 | Behandelt | Token, Rotation, Vault | Sec |
| SEC-006 | Insider Threat | 2 | 4 | 🟡 8 | Behandelt | RBAC, Audit, LoD | Sec |
| SEC-007 | Zero-Day Vulnerability | 2 | 5 | 🟠 10 | Überwacht | SBOM, Monitoring, Updates | Sec |
| SEC-008 | Key Compromise | 2 | 5 | 🟠 10 | Behandelt | HSM, Rotation, Split Keys | Sec |
| SEC-009 | Session Hijacking | 2 | 4 | 🟡 8 | Behandelt | Token Expiry, TLS | Dev |
| SEC-010 | Supply Chain Attack | 2 | 5 | 🟠 10 | Behandelt | SBOM, Signed Deps, Review | Sec |

---

## 5. Operationelle Risiken (OPS)

| ID | Risiko | W | A | Score | Status | Maßnahmen | Owner |
|----|--------|---|---|-------|--------|-----------|-------|
| OPS-001 | Backup-Versagen | 2 | 5 | 🟠 10 | Behandelt | Backup-Tests, Monitoring | Ops |
| OPS-002 | Konfigurationsfehler | 3 | 3 | 🟡 9 | Behandelt | IaC, Review, Rollback | Ops |
| OPS-003 | Unzureichendes Monitoring | 2 | 3 | 🟡 6 | Behandelt | Prometheus, Alerting | Ops |
| OPS-004 | Key-Person-Risiko | 3 | 3 | 🟡 9 | Überwacht | Dokumentation, Training | HR |
| OPS-005 | Fehlende Updates | 3 | 4 | 🟠 12 | Behandelt | Patch-Management | Ops |
| OPS-006 | Manueller Fehler | 4 | 3 | 🟠 12 | Behandelt | Automation, Checklisten | Ops |

---

## 6. Compliance-Risiken (COM)

| ID | Risiko | W | A | Score | Status | Maßnahmen | Owner |
|----|--------|---|---|-------|--------|-----------|-------|
| COM-001 | DSGVO-Verstoß | 2 | 5 | 🟠 10 | Behandelt | DPIA, PII-Detection, Löschung | Legal |
| COM-002 | Unzureichende Dokumentation | 2 | 3 | 🟡 6 | Behandelt | Audit-Checkliste, Docs | Compliance |
| COM-003 | Audit-Versagen | 2 | 4 | 🟡 8 | Behandelt | Vorbereitung, Dashboard | Compliance |
| COM-004 | Meldepflicht-Verletzung | 2 | 4 | 🟡 8 | Behandelt | IRP, Prozesse | Legal |
| COM-005 | Lizenz-Verletzung | 2 | 3 | 🟡 6 | Behandelt | License Check, SBOM | Legal |

---

## 7. Geschäftsrisiken (BUS)

| ID | Risiko | W | A | Score | Status | Maßnahmen | Owner |
|----|--------|---|---|-------|--------|-----------|-------|
| BUS-001 | Reputationsschaden | 2 | 4 | 🟡 8 | Überwacht | Security, PR-Plan | Mgmt |
| BUS-002 | Vendor Lock-in (RocksDB) | 2 | 3 | 🟡 6 | Akzeptiert | Abstraktion, Alternativen | Dev |
| BUS-003 | Wettbewerbsdruck | 3 | 3 | 🟡 9 | Überwacht | Innovation, Features | Mgmt |
| BUS-004 | Wirtschaftliche Faktoren | 2 | 3 | 🟡 6 | Akzeptiert | Diversifikation | Mgmt |

---

## 8. Risiko-Heatmap

```
              AUSWIRKUNG
              1    2    3    4    5
           ┌────┬────┬────┬────┬────┐
         5 │    │    │    │    │SEC1│  W
           ├────┼────┼────┼────┼────┤  A
         4 │    │    │OPS6│SEC3│    │  H
           ├────┼────┼────┼────┼────┤  R
         3 │    │    │TEC2│OPS4│SEC2│  S
           ├────┼────┼────┼────┼────┤  C
         2 │    │TEC8│COM5│SEC6│TEC1│  H
           ├────┼────┼────┼────┼────┤  E
         1 │    │    │    │    │    │  I
           └────┴────┴────┴────┴────┘  N
                                       L.
```

---

## 9. Top 10 Risiken

| Rang | ID | Risiko | Score | Status |
|------|-------|--------|-------|--------|
| 1 | SEC-001 | Unbefugter Datenzugriff | 🔴 15 | Behandelt |
| 2 | SEC-002 | Ransomware/Malware | 🔴 15 | Behandelt |
| 3 | SEC-003 | DDoS-Angriff | 🟠 12 | Behandelt |
| 4 | SEC-005 | Credential Theft | 🟠 12 | Behandelt |
| 5 | OPS-005 | Fehlende Updates | 🟠 12 | Behandelt |
| 6 | OPS-006 | Manueller Fehler | 🟠 12 | Behandelt |
| 7 | TECH-004 | Netzwerkausfall | 🟠 12 | Behandelt |
| 8 | SEC-007 | Zero-Day Vulnerability | 🟠 10 | Überwacht |
| 9 | SEC-008 | Key Compromise | 🟠 10 | Behandelt |
| 10 | SEC-010 | Supply Chain Attack | 🟠 10 | Behandelt |

---

## 10. Risiko-Behandlung

### 10.1 Behandlungsoptionen

| Option | Beschreibung | Anwendung |
|--------|--------------|-----------|
| **Vermeiden** | Risiko-Quelle eliminieren | Wenn möglich und wirtschaftlich |
| **Vermindern** | Wahrscheinlichkeit/Auswirkung reduzieren | Standard-Ansatz |
| **Übertragen** | Risiko an Dritte (Versicherung, Outsourcing) | Bei spezifischen Risiken |
| **Akzeptieren** | Risiko bewusst eingehen | Bei geringem Risiko-Score |

### 10.2 Behandlungsplan

| ID | Risiko | Maßnahme | Verantwortlich | Frist | Status |
|----|--------|----------|----------------|-------|--------|
| SEC-001 | Unbef. Zugriff | RBAC Review, Pen-Test | Security | Q1 2026 | 🟡 |
| SEC-002 | Ransomware | Backup-Tests, DR-Übung | Ops | Q1 2026 | 🟡 |
| SEC-007 | Zero-Day | SBOM-Monitoring, Alerts | Security | Laufend | ✅ |
| OPS-004 | Key-Person | Doku, Cross-Training | HR | Q2 2026 | 🟡 |

---

## 11. Risiko-Indikatoren (KRIs)

| KRI | Beschreibung | Schwellenwert | Aktuell |
|-----|--------------|---------------|---------|
| **Fehlgeschlagene Logins** | > 100 pro Stunde | 100 | ✅ < 10 |
| **Kritische Vulnerabilities** | > 0 | 0 | ✅ 0 |
| **Backup-Fehler** | > 1 pro Woche | 1 | ✅ 0 |
| **Ungeplante Ausfallzeit** | > 1h pro Monat | 1h | ✅ 0 |
| **Unbehandelte Alerts** | > 5 | 5 | ✅ 0 |
| **Offene High-Risk Findings** | > 0 | 0 | ✅ 0 |

---

## 12. Review und Aktualisierung

### 12.1 Review-Zyklus

| Aktivität | Frequenz | Verantwortlich |
|-----------|----------|----------------|
| KRI-Monitoring | Wöchentlich | Security |
| Risiko-Review | Monatlich | Risk Manager |
| Vollständige Bewertung | Vierteljährlich | Risk Committee |
| Externe Prüfung | Jährlich | Auditor |

### 12.2 Änderungsprotokoll

| Datum | Änderung | Autor |
|-------|----------|-------|
| Nov 2025 | Erstversion | ThemisDB Team |

---

## 13. Anhänge

### A. Risiko-Eigentümer

| Rolle | Verantwortlichkeiten |
|-------|---------------------|
| **Security** | Sicherheitsrisiken, Penetration Testing |
| **Dev** | Technische Risiken, Code-Qualität |
| **Ops** | Operationelle Risiken, Infrastruktur |
| **Legal** | Compliance-Risiken, Verträge |
| **HR** | Personalrisiken |
| **Mgmt** | Geschäftsrisiken, Budget |

### B. Referenzen

| Dokument | Pfad |
|----------|------|
| Security Audit Report | `docs/reports/SECURITY_AUDIT_REPORT.md` |
| Threat Model | `docs/security/threat_model.md` |
| DPIA | `docs/compliance/DPIA.md` |
| BCP/DRP | `docs/compliance/BCP_DRP.md` |
| IRP | `docs/security/INCIDENT_RESPONSE_PLAN.md` |

---

**Letzte Aktualisierung:** November 2025  
**Dokumentverantwortlicher:** ThemisDB Risk Management  
**Nächstes Review:** [Datum + 3 Monate]
