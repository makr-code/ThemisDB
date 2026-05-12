# ThemisDB – Finanzierungs- und Kostenplan 2027

**Stand:** 2026-05-12
**Zweck:** Dokumentation einer realistischen Reward-Verteilung für ein Kampagnenziel von **150.000 €** sowie einer **Gesamtkosten-Kalkulation 2027** für die Weiterentwicklung.

---

## 1) Reward-Verteilung für Zielsumme 150.000 €

### Anforderungen
- Jede Reward-Stufe enthält: **Titel**, **Beschreibung**, **von–bis €**, **Anzahl Rewards (Begrenzung)**.
- Die Verteilung ist auf Realismus ausgelegt (viele kleine, wenige große Pakete).
- Zielsumme: **150.000 €**.

### Reward-Tabelle

| # | Titel | Beschreibung | Von–Bis € | Anzahl Rewards (max.) | Kalkulierter Ø-Betrag | Potenzial je Stufe |
|---|---|---|---|---:|---:|---:|
| 1 | Community Backer | Nennung in der Supporter-Liste, Projekt-Updates | 10–49 € | 1.000 | 25 € | 25.000 € |
| 2 | Early Supporter | Exklusive Roadmap-Updates, Community-Rolle, Abstimmungen | 50–249 € | 400 | 100 € | 40.000 € |
| 3 | Pro Sponsor | Priorisierte Community-Support-Slots, 1x Q&A/Call pro Quartal | 250–999 € | 100 | 500 € | 50.000 € |
| 4 | Lead Sponsor | Logo/Name in Doku, strategischer Quartals-Review, Early-Access | ab 1.000 € | 35 | 1.000 € | 35.000 € |

### Summenkontrolle

- Stufe 1: 1.000 × 25 € = 25.000 €
- Stufe 2: 400 × 100 € = 40.000 €
- Stufe 3: 100 × 500 € = 50.000 €
- Stufe 4: 35 × 1.000 € = 35.000 €

**Gesamtsumme: 150.000 €**

### Hinweise zur Realistik

- Hohe Stückzahl nur im niedrigen Preisbereich.
- Mittlere und hohe Tiers sind limitiert, um Knappheit und Planbarkeit zu schaffen.
- Obergrenzen verhindern eine unplausible Verteilung auf Premium-Tiers.

---

## 2) Gesamtkosten-Kalkulation 2027 (Weiterentwicklung)

## Annahmen

- Betrachtungszeitraum: **01.01.2027–31.12.2027**
- Währung: **EUR**, Jahreswerte (netto, gerundet)
- Fokus: laufende Kosten für Entwicklung, Betrieb, Qualität und Sicherheit
- Teamgröße für Planung: **6 FTE Engineering + 1 FTE DevOps/QA + 1 FTE Produkt/PM (anteilig)**
- Kalkulationssatz Personal: **Ø 95.000 € pro FTE/Jahr** (inkl. Arbeitgebernebenkosten, Recruiting-/Onboarding-Anteil, Weiterbildungsbudget)

### Kostenübersicht 2027

| Kostenblock | Inhalt | Jahreskosten |
|---|---|---:|
| Personal (Weiterentwicklung) | Engineering, DevOps/QA, Produkt/PM-Anteil | 760.000 € |
| Hardware | Build-/Test-Workstations, Ersatzgeräte, Netzwerk, Speicher | 95.000 € |
| Software & Lizenzen | IDE, Security-Tools, Observability, PM-Tools, Doku-Tools | 68.000 € |
| KI-Tools | Copilot/LLM-Tooling, lokale Modell-Infrastruktur, API-Budgets | 74.000 € |
| Cloud/Hosting & CI | Artefakt-Storage, Runner, Container-Registry, Monitoring | 86.000 € |
| Strom & Energie | Server/Workstations/Lab, Kühlung-Anteil | 31.000 € |
| Security & Compliance | Audits, Pentests, Zertifizierungsaufwand, Policies | 54.000 € |
| Recht, Gründung & Steuerberatung | GmbH-Gründung, steuerliche Begleitung, Abschluss, Lohnbuchhaltung | 39.000 € |
| Büroflächen & Nebenkosten (Potsdam/Werder) | Miete, Nebenkosten, Facility, Business-Internet (Glasfaser) | 84.000 € |
| Reisen/Workshops/Enablement | Kundentermine, Partner-Workshops, Team-Schulungen | 28.000 € |
| Sonstiges & Reserve | Unvorhergesehenes, Preissteigerungen, Puffer | 44.000 € |

### Detaillierte Einzelposten 2027

#### Personal (760.000 €)

| Einzelposten | Menge | Einheitspreis | Jahressumme |
|---|---:|---:|---:|
| Software Engineering FTE | 6,0 | 100.000 € | 600.000 € |
| DevOps/QA FTE | 1,0 | 95.000 € | 95.000 € |
| Produkt/PM (anteilig) | 0,5 | 80.000 € | 40.000 € |
| Recruiting/Onboarding/Weiterbildung | pauschal | - | 25.000 € |
| **Blocksumme Personal** |  |  | **760.000 €** |

#### Hardware (95.000 €)

| Einzelposten | Menge | Einheitspreis | Jahressumme |
|---|---:|---:|---:|
| Entwickler-Workstations | 6 | 4.000 € | 24.000 € |
| CI-/Build-Server | 2 | 12.000 € | 24.000 € |
| GPU-Server (lokale KI-Workloads) | 1 | 28.000 € | 28.000 € |
| Netzwerk + NAS/Storage | 1 Paket | 11.000 € | 11.000 € |
| Ersatzgeräte/Peripherie | pauschal | - | 8.000 € |
| **Blocksumme Hardware** |  |  | **95.000 €** |

#### Software & Lizenzen (68.000 €)

| Einzelposten | Menge | Einheitspreis | Jahressumme |
|---|---:|---:|---:|
| IDE-/Entwicklungs-Lizenzen | 8 Seats | 1.000 € | 8.000 € |
| Security-Scanner/Code-Tools | 1 Paket | 12.000 € | 12.000 € |
| Observability/APM-Software | 1 Paket | 10.000 € | 10.000 € |
| Projekt-/Collaboration-Tools | 1 Paket | 8.000 € | 8.000 € |
| Dokumentations-Tooling | 1 Paket | 6.000 € | 6.000 € |
| Testautomations-/QA-Lizenzen | 1 Paket | 10.000 € | 10.000 € |
| Kommerzielle Runtime-/OS-Lizenzen | 1 Paket | 14.000 € | 14.000 € |
| **Blocksumme Software & Lizenzen** |  |  | **68.000 €** |

#### KI-Tools (74.000 €)

| Einzelposten | Menge | Einheitspreis | Jahressumme |
|---|---:|---:|---:|
| Copilot-/Coding-Assistenten | 8 Seats | 240 € | 1.920 € |
| LLM-API-Budget (Inference) | 12 Monate | 3.000 € | 36.000 € |
| Lokale Modell-Infrastruktur (Support/Artifacts) | 12 Monate | 1.500 € | 18.000 € |
| Prompt-Evaluierung/Guardrail-Tools | 12 Monate | 667 € | 8.000 € |
| Reranking-/Embedding-API-Budget | 12 Monate | 833 € | 10.000 € |
| Rundungs-/Pufferanteil | pauschal | - | 80 € |
| **Blocksumme KI-Tools** |  |  | **74.000 €** |

#### Cloud/Hosting & CI (86.000 €)

| Einzelposten | Menge | Einheitspreis | Jahressumme |
|---|---:|---:|---:|
| CI-Runner-Kapazität | 12 Monate | 2.167 € | 26.000 € |
| Artefakt-Storage/Container-Registry | 12 Monate | 1.000 € | 12.000 € |
| Staging-Compute-Ressourcen | 12 Monate | 1.500 € | 18.000 € |
| Monitoring-/Log-Storage | 12 Monate | 833 € | 10.000 € |
| Backup/Object-Storage | 12 Monate | 667 € | 8.000 € |
| Bandbreite/Egress | 12 Monate | 500 € | 6.000 € |
| Domains/Certs/DNS | 12 Monate | 500 € | 6.000 € |
| **Blocksumme Cloud/Hosting & CI** |  |  | **86.000 €** |

#### Strom & Energie (31.000 €)

| Einzelposten | Menge | Einheitspreis | Jahressumme |
|---|---:|---:|---:|
| Basisstrom Büro/Lab | 12 Monate | 1.000 € | 12.000 € |
| Server-/Rack-Strom | 12 Monate | 917 € | 11.000 € |
| Kühlung (anteilig) | 12 Monate | 500 € | 6.000 € |
| USV-/Wandlungsverluste | pauschal | - | 2.000 € |
| **Blocksumme Strom & Energie** |  |  | **31.000 €** |

#### Security & Compliance (54.000 €)

| Einzelposten | Menge | Einheitspreis | Jahressumme |
|---|---:|---:|---:|
| Externe Pentests | 2 | 9.000 € | 18.000 € |
| Compliance-Beratung | 1 Paket | 14.000 € | 14.000 € |
| Audit-/Zertifizierungsvorbereitung | 1 Paket | 10.000 € | 10.000 € |
| Incident-Response-Retainer | 12 Monate | 667 € | 8.000 € |
| Security-Schulungen | pauschal | - | 4.000 € |
| **Blocksumme Security & Compliance** |  |  | **54.000 €** |

#### Recht, Gründung & Steuerberatung (39.000 €)

| Einzelposten | Menge | Einheitspreis | Jahressumme |
|---|---:|---:|---:|
| GmbH-Gründung (Notar, Handelsregister, Gebühren) | 1 | 2.500 € | 2.500 € |
| IHK-/Kammerbeiträge (Mindestansatz) | 12 Monate | 100 € | 1.200 € |
| Steuerberatung laufend (USt, BWA, Meldungen) | 12 Monate | 1.500 € | 18.000 € |
| Jahresabschluss + Steuererklärungen | 1 | 9.000 € | 9.000 € |
| Lohnbuchhaltung / Payroll | 12 Monate | 450 € | 5.400 € |
| Rechtsberatung (Verträge, Datenschutz, AGB) | pauschal | - | 2.900 € |
| **Blocksumme Recht, Gründung & Steuerberatung** |  |  | **39.000 €** |

#### Büroflächen & Nebenkosten (Potsdam/Werder) (84.000 €)

| Einzelposten | Menge | Einheitspreis | Jahressumme |
|---|---:|---:|---:|
| Kaltmiete Bürofläche | 220 m² × 12 Monate | 16 € / m² | 42.240 € |
| Nebenkosten/Heizung | 220 m² × 12 Monate | 4,5 € / m² | 11.880 € |
| Stellplätze/Lageranteil | 6 × 12 Monate | 90 € | 6.480 € |
| Reinigung/Facility-Services | 12 Monate | 900 € | 10.800 € |
| Business-Internet (Glasfaser, symmetrisch) | 12 Monate | 450 € | 5.400 € |
| Büroeinrichtung/Abschreibung/Nachrüstung | pauschal | - | 7.200 € |
| **Blocksumme Büroflächen & Nebenkosten** |  |  | **84.000 €** |

#### Reisen/Workshops/Enablement (28.000 €)

| Einzelposten | Menge | Einheitspreis | Jahressumme |
|---|---:|---:|---:|
| Kunden-Onsite-Termine | 12 Trips | 1.000 € | 12.000 € |
| Partner-/Integrations-Workshops | 7 Termine | 1.000 € | 7.000 € |
| Fachkonferenzen/Schulungen | pauschal | - | 6.000 € |
| Interne Enablement-Offsites | pauschal | - | 3.000 € |
| **Blocksumme Reisen/Workshops/Enablement** |  |  | **28.000 €** |

#### Sonstiges & Reserve (44.000 €)

| Einzelposten | Menge | Einheitspreis | Jahressumme |
|---|---:|---:|---:|
| Preissteigerungs-/Inflationspuffer | pauschal | - | 20.000 € |
| Recht/Steuer/Vertragsnebenkosten | pauschal | - | 12.000 € |
| Reparaturen/ungeplante Ersatzausgaben | pauschal | - | 6.000 € |
| Allgemeine Projektreserve | pauschal | - | 6.000 € |
| **Blocksumme Sonstiges & Reserve** |  |  | **44.000 €** |

### Gesamtkosten 2027

**Gesamtbudget 2027: 1.363.000 €**

---

## 3) Kostenaufschlüsselung nach Typ

| Typ | Summe | Anteil |
|---|---:|---:|
| Personalkosten | 760.000 € | 55,8 % |
| Nicht-Personalkosten | 603.000 € | 44,2 % |
| **Gesamt** | **1.363.000 €** | **100 %** |

---

## 4) Einordnung Kampagnenziel vs. Weiterentwicklungskosten

- Geplantes Funding-Ziel (Rewards): **150.000 €**
- Gesamtkosten 2027: **1.363.000 €**
- Deckungsanteil durch 150.000 €: **11,0 %**
- Verbleibender Finanzierungsbedarf 2027: **1.213.000 €**

---

## 5) Standortsuche Büroflächen Potsdam / Werder (Havel)

### Suchprofil (Muss-Kriterien)

- Lage: Potsdam oder Werder (Havel), ÖPNV-nah
- Fläche: **180–260 m²** (Team + Besprechung + Technikraum)
- Internet: **Business-Glasfaser verfügbar oder kurzfristig schaltbar**
- Strom/Technik: getrennte Stromkreise, 19"-Rack-/Serverecke möglich
- Laufzeit: bevorzugt 24–36 Monate, Optionen zur Flächenerweiterung
- Budgetkorridor (warm inkl. Nebenkosten): **6.000–8.000 € / Monat**

### Recherche- und Anfragestruktur (ausführungsbereit)

| Kanal/Quelle | Region | Ziel | Prüfpunkt Glasfaser |
|---|---|---|---|
| Immobilienportale (Gewerbe) | Potsdam/Werder | verfügbare Büroeinheiten mit Grundriss/Preis | Verfügbarkeitsnachweis (Carrier/Adresscheck) anfordern |
| Lokale Makler mit Gewerbefokus | Potsdam/Werder | Off-Market-Flächen und kurzfristige Optionen | Zusicherung symmetrischer Bandbreite schriftlich anfragen |
| Coworking/Business Center | Potsdam | flexible Startfläche für 3–9 Monate | SLA, Upload-Rate und dedizierte Leitung prüfen |
| Wirtschaftsförderung/Stadtportale | Potsdam/Werder | Gewerbeparks, Entwicklungsflächen, Kontakte | Netzbetreiber und Hausanschlussstatus abfragen |

### Due-Diligence je Objekt (Checkliste)

- Glasfaser: Anbieter, max. Up/Down, SLA, Entstörzeit, Vertragslaufzeit
- Technische Infrastruktur: Rackfläche, Klimatisierung, Zugang außerhalb Bürozeiten
- Gebäude: Nebenkostenstruktur, Energiekennwert, Parken, Sicherheit, Zutrittskontrolle
- Vertragsrisiken: Staffelmiete, Indexierung, Umbauten, Sonderkündigungsrechte

### Hinweis zum aktuellen Recherche-Stand

Im aktuellen Agent-Umfeld ist externe DNS-/Web-Recherche eingeschränkt. Die Kostenpositionen sind daher bereits budgetiert; die konkrete Objekt-Auswahl erfolgt über die oben definierte Anfragestruktur mit dokumentierter Glasfaser-Bestätigung pro Objekt.

### Fazit

Das Reward-Ziel von 150.000 € ist als **Teilfinanzierung** für 2027 realistisch und sinnvoll, deckt jedoch nur einen begrenzten Anteil der gesamten Weiterentwicklungskosten. Durch die zusätzliche Berücksichtigung von GmbH-Gründung, steuerlicher Betreuung und Büroinfrastruktur steigt der nicht-personelle Kostenanteil. Für 2027 ist daher zusätzlich eine Kombination aus laufenden Einnahmen, Partnerfinanzierung und/oder Investitionsmitteln erforderlich.
