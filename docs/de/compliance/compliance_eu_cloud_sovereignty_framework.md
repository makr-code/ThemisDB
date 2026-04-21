# ThemisDB – ES3 Audit nach EU Cloud Sovereignty Framework

**Stand:** 21. April 2026  
**Version:** v1.8.2-rc  
**Kategorie:** 🔒 Compliance  
**Klassifizierung:** Vertraulich  
**Prüfstandard:** ES3 (EU Cloud Sovereignty Framework + KI als 9. Dimension)

---

## 📋 Ziel und Vorgehen

Dieser Bericht setzt die ES3-Anforderung um:

1. **Informationen zur Klassifizierung sammeln**
2. **Themis nach Klassifizierung auditieren**
3. **Ergebnisbericht verfassen**

Die Bewertung erfolgt je Dimension auf drei Ebenen:
- **Regulatorik**
- **Organisation**
- **Technologie**

Skala je Ebene: **1 (niedrig) bis 5 (hoch)**.  
Dimensionswert = Minimum aus den drei Ebenen.  
**Gesamtwert = Minimum-Prinzip über alle Dimensionen (schwächstes Glied).**

---

## 1. Klassifizierungsgrundlage (ES3)

### 1.1 Bewertungsdimensionen (8 + 1 KI)

| # | Dimension | Kurzdefinition |
|---|-----------|----------------|
| 1 | Strategische Souveränität | Unabhängigkeit von einzelnen Anbietern/Standorten |
| 2 | Rechtliche & jurisdiktionelle Aspekte | EU-Rechtsrahmen, Vertragslage, Drittland-Transfer |
| 3 | Daten | Klassifizierung, Schutz, Residency, Löschung |
| 4 | Operative Unabhängigkeit | Betriebsfähigkeit, Exit-Fähigkeit, Kontinuität |
| 5 | Lieferkette | Drittanbieter-, Dependency- und Subprozessor-Risiken |
| 6 | Technologie | Architektur, Portabilität, offene Standards, Interoperabilität |
| 7 | Sicherheit & Compliance | Security Controls, Nachweisführung, Auditierbarkeit |
| 8 | Ökologische Nachhaltigkeit | Transparenz zu Energie-/Betriebsindikatoren |
| 9 | Künstliche Intelligenz (ES3-spezifisch) | KI-Governance, Modell-/Prompt-/Datenkontrollen |

### 1.2 Verwendete Nachweise in ThemisDB

- `docs/de/compliance/compliance_full_checklist.md`
- `docs/de/compliance/compliance_vendor_assessment.md`
- `docs/de/compliance/compliance_dpia.md`
- `docs/de/compliance/compliance_risk_register.md`
- `docs/de/policies/policies_data_classification.md`
- `docs/de/features/features_compliance_governance.md`
- `src/governance/ROADMAP.md`

---

## 2. Audit-Ergebnis je Dimension (Regulatorik / Organisation / Technologie)

| Dimension | Regulatorik (1-5) | Organisation (1-5) | Technologie (1-5) | Dimensionswert (Minimum) | Kurzbegründung |
|-----------|-------------------|--------------------|-------------------|---------------------------|----------------|
| Strategische Souveränität | 3 | 3 | 4 | **3** | Multi-Provider-fähige Architektur und dokumentierte Optionen vorhanden, aber formalisierte Exit-Nachweise noch ausbaufähig |
| Rechtliche & jurisdiktionelle Aspekte | 4 | 3 | 3 | **3** | DSGVO/AVV-/Vendor-Kriterien klar dokumentiert; laufende formale Re-Assessments teils noch offen |
| Daten | 4 | 4 | 4 | **4** | 4-stufige Datenklassifizierung, Verschlüsselungs- und Retention-Policy sowie DPIA-Nachweise vorhanden |
| Operative Unabhängigkeit | 3 | 3 | 4 | **3** | Wiederanlauf-/Backup-Konzepte dokumentiert; regelmäßige formale DR-Übungen noch empfohlen |
| Lieferkette | 3 | 3 | 4 | **3** | SBOM/Vendor-Assessment vorhanden, kontinuierliche Re-Zertifizierungsprozesse teilweise manuell |
| Technologie | 4 | 3 | 4 | **3** | Starke modulare Architektur und offene Schnittstellen; Governance-Prozesse noch nicht vollständig automatisiert |
| Sicherheit & Compliance | 4 | 4 | 4 | **4** | Umfassende Kontrollabdeckung mit Audit- und Richtlinienbasis; offene P2/P3-Punkte dokumentiert |
| Ökologische Nachhaltigkeit | 2 | 2 | 2 | **2** | Nachhaltigkeitskennzahlen und belastbare Energie-/CO₂-Nachweise nicht systematisch dokumentiert |
| Künstliche Intelligenz | 3 | 3 | 4 | **3** | AI/ML-Governance und Impact-Assessment vorhanden; operativer Rollout kritischer Pfade läuft noch |

---

## 3. Gesamteinstufung nach Minimum-Prinzip

### 3.1 Rechenweg

- Minimum aller Dimensionswerte = **2**
- Schwächstes Glied = **Ökologische Nachhaltigkeit**

### 3.2 Gesamtergebnis (ES3)

> **ES3-Gesamteinstufung ThemisDB: Stufe 2/5**  
> (gemäß Minimum-Prinzip des EU Cloud Sovereignty Framework)

Interpretation: ThemisDB zeigt in mehreren Kernbereichen Reifegrad 3–4, wird aber durch die aktuell schwächste Dimension (ökologische Nachhaltigkeit) auf Gesamtniveau 2 begrenzt.

---

## 4. Priorisierte Maßnahmen zur Anhebung der Gesamteinstufung

### P1 – ökologische Nachhaltigkeit (direkter Hebel auf Gesamtwert)
- Messbare KPIs ergänzen: Energieverbrauch pro Workload, CO₂-Proxy pro Umgebung, Last-/Leistungsprofil.
- Auditfähige Evidenzablage aufbauen (Berichtsturnus, Verantwortlichkeiten, Revisionshistorie).
- Zielbild: **Dimension Nachhaltigkeit von 2 → 3**.

### P2 – formale Souveränitäts-/Exit-Fähigkeit
- Standardisierte Exit-/Portabilitäts-Runbooks inkl. Testnachweis ergänzen.
- Jurisdiktions- und Subprozessor-Review turnusmäßig mit evidenzbasierter Freigabe durchführen.

### P3 – KI-Dimension operationalisieren
- In `src/governance/ROADMAP.md` definierte ML/AI-Governance-Piloten (kritische Pfade) mit KPI-Baseline und Drill-Evidenz abschließen.

---

## 5. Audit-Traceability

| ES3-Anforderung | ThemisDB-Nachweis |
|-----------------|-------------------|
| Datenklassifizierung | `docs/de/policies/policies_data_classification.md` |
| Compliance-Checklisten (EU/ISO/BSI) | `docs/de/compliance/compliance_full_checklist.md` |
| Jurisdiktion & Vendor-Kontrollen | `docs/de/compliance/compliance_vendor_assessment.md` |
| Datenschutz-Folgenabschätzung | `docs/de/compliance/compliance_dpia.md` |
| Governance- und AI/ML-Kontrollen | `docs/de/features/features_compliance_governance.md`, `src/governance/ROADMAP.md` |

---

## 6. Verifizierung und externe Prüfung

Für die formale Verifizierung der ES3-Einstufung ist eine externe Prüfung mit Wirtschaftsprüfung (z. B. **BDO**) vorgesehen. Dieser Bericht dient als interne Vorbewertung und Evidenzbasis für die externe Validierung.

