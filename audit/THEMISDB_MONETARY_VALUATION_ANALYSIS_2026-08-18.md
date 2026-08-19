# ThemisDB Monetäre Bewertung 2026-08-18 (wissenschaftlich strukturierte Fassung)

**Klassifikation:** Vertraulich  
**Scope:** Finanzierungs-/M&A-Entscheidungsunterlage  
**Source of Truth:** `/audit/**`

---

## Abstract

Diese Studie quantifiziert den monetären Wert von ThemisDB in zwei Zuständen (**Stand jetzt** vs. **production-ready**) anhand eines reproduzierbaren Modells aus (a) TCO-ROI, (b) Replikationskosten mit/ohne KI-Entwicklung, (c) Bewertungs-Multiplikatoren und (d) Governance-/Execution-Discounts.  
Die Ergebnisse zeigen einen **EV-Lift von +€26M bis +€47M** beim Übergang von „Stand jetzt“ zu „production-ready“ sowie eine signifikante **Cost-to-Recreate-Reduktion um 60%–73,3%** durch KI-beschleunigte Entwicklung.  
Zusätzlich erweitert diese Fassung die rein quantitative Sicht um eine **Build-vs-Buy-Interpretation**, eine **Kundensegment- und Einsatzfalllogik**, eine **No-Go-Abgrenzung** sowie eine **strukturierte Forschungsagenda** für die nächste Vertiefungsstufe.

---

## 1. Forschungsfragen

1. Wie hoch ist der monetäre Wert von ThemisDB im aktuellen Zustand im Vergleich zu einem production-ready Zustand?
2. Wie groß ist der wirtschaftliche Vorteil (ROI, Payback) gegenüber alternativen Architekturansätzen?
3. Wie unterscheiden sich Replikationskosten „ohne KI“ vs. „mit KI“ methodisch und quantitativ?
4. Wann ist für Dritte Lizenzkauf rationaler als Eigenbau oder Open-Source-Patchwork?
5. Welche Kundensegmente und Einsatzfelder weisen den höchsten ökonomischen und strategischen Fit auf?
6. Welche Aussagen des Berichts sind bereits intern belastbar, und wo ist zusätzliche externe Validierung erforderlich?

---

## 2. Datenbasis und Quellenprovenienz

## 2.1 Primärquellen (Repository-intern)

1. `/home/runner/work/ThemisDB/ThemisDB/docs/de/THEMISDB_MONETARY_VALUATION_ANALYSIS.confidential.md`
2. `/home/runner/work/ThemisDB/ThemisDB/docs/de/strategie/FINANZIERUNG_UND_KOSTENPLAN_2027.md`
3. `/home/runner/work/ThemisDB/ThemisDB/docs/de/deployment/PRICING_MODEL_v1.3.5.confidential.md`
4. `/home/runner/work/ThemisDB/ThemisDB/audit/PRODUCTION_READINESS_ASSESSMENT_2026-08-18.md`
5. `/home/runner/work/ThemisDB/ThemisDB/audit/AUDIT_SUMMARY_2026-08-18.md`
6. `/home/runner/work/ThemisDB/ThemisDB/audit/MATURITY_REPORT_2026-08.md`

## 2.2 Übernommene Messwerte

| Messbereich | Basiswert(e) |
|---|---|
| TCO Szenario A (SMB, 5y) | ThemisDB €550k, AWS €1.975k, Azure €1.915k, GCP €1.715k |
| TCO Szenario B (Enterprise, 5y) | ThemisDB €2.090k, AWS €10.500k, Azure €11.100k, GCP €9.000k |
| TCO Szenario C (KRITIS, 5y) | ThemisDB €550k, OSS-Patchwork €1.450k |
| TCO Szenario D (Military, 5y) | ThemisDB €1.300k, Legacy €4.100k, OSS+Integrator €2.650k |
| Replikationsaufwand | Traditionell 30.000h @ €80/h; KI-beschleunigt 8.000–12.000h @ €80/h |
| Kostenstruktur 2027 | Personal €760k, Nicht-Personal €603k, KI-Tools €74k, Gesamt €1.363M |

---

## 3. Methodik

## 3.1 Bewertungsrahmen

Die monetäre Bewertung erfolgt als Multi-Methoden-Ansatz:

1. **Wirtschaftlichkeitsmodell (TCO/ROI/Payback)** je Einsatzszenario  
2. **Replikationskostenmodell** (Engineering-Hours und Vollkostenmodell)  
3. **EV-Bridge** zwischen „Stand jetzt“ und „production-ready“ über Discount-Reduktion

## 3.2 Formale Definitionen

Für jedes Vergleichsszenario \(s\):

- \( Savings_s = TCO_{Alt,s} - TCO_{Themis,s} \)
- \( BenefitCost_s = Savings_s / TCO_{Themis,s} \)
- \( ROI_s = (Savings_s - TCO_{Themis,s}) / TCO_{Themis,s} \)
- \( Payback_s = TCO_{Themis,s} / (Savings_s / 5) \)

Replikationskosten:

- \( Cost_{trad} = Hours_{trad} \times Rate \)
- \( Cost_{ai} = Hours_{ai} \times Rate \)
- \( Savings_{ai} = Cost_{trad} - Cost_{ai} \)

Vollkostenmodell:

- \( FTEYears = Hours / 1600 \)
- \( PersonnelCost = FTEYears \times 95.000 \)
- \( OverheadFactor = 1 + (603.000 / 760.000) = 1{,}793 \)
- \( FullCost = PersonnelCost \times OverheadFactor \)

KI-Tool-Break-even:

- \( Hours_{BE} = 74.000 / 80 = 925 \) Stunden/Jahr

---

## 4. Ergebnisse I — ROI und Payback

## 4.1 Szenario A (SMB, 5 Jahre)

| Vergleich | 5Y Savings | Benefit/Cost | ROI (5Y) | Payback |
|---|---:|---:|---:|---:|
| vs AWS | €1,425M | 2,59× | 159,1% | 1,93 Jahre |
| vs Azure | €1,365M | 2,48× | 148,2% | 2,01 Jahre |
| vs GCP | €1,165M | 2,12× | 111,8% | 2,36 Jahre |

## 4.2 Szenario B (Enterprise, 5 Jahre)

| Vergleich | 5Y Savings | Benefit/Cost | ROI (5Y) | Payback |
|---|---:|---:|---:|---:|
| vs AWS | €8,410M | 4,02× | 302,4% | 1,24 Jahre |
| vs Azure | €9,010M | 4,31× | 331,1% | 1,16 Jahre |
| vs GCP | €6,910M | 3,31× | 230,6% | 1,51 Jahre |

## 4.3 Szenario C (KRITIS, 5 Jahre)

| Vergleich | 5Y Savings | Benefit/Cost | ROI (5Y) | Payback |
|---|---:|---:|---:|---:|
| vs OSS-Patchwork | €0,900M | 1,64× | 63,6% | 3,06 Jahre |

## 4.4 Szenario D (Military, 5 Jahre)

| Vergleich | 5Y Savings | Benefit/Cost | ROI (5Y) | Payback |
|---|---:|---:|---:|---:|
| vs Legacy DefTech | €2,800M | 2,15× | 115,4% | 2,32 Jahre |
| vs OSS+Integrator | €1,350M | 1,04× | 3,8% | 4,81 Jahre |

### Befund 1

Der stärkste ökonomische Hebel liegt im Enterprise-Szenario (B) mit kurzer Kapitalrückflusszeit (~1,2–1,5 Jahre) und hohen Benefit/Cost-Werten.

---

## 5. Ergebnisse II — Replikationskosten mit/ohne KI

## 5.1 Engineering-Hour-Modell

| Modell | Stunden | Satz | Direkte Kosten |
|---|---:|---:|---:|
| Ohne KI (traditionell) | 30.000h | €80/h | €2,400M |
| Mit KI (untere Grenze) | 8.000h | €80/h | €0,640M |
| Mit KI (obere Grenze) | 12.000h | €80/h | €0,960M |

**Direkter KI-Effekt:**
- absolute Einsparung: **€1,440M–€1,760M**
- relative Einsparung: **60,0%–73,3%**

## 5.2 Vollkostenmodell (auf Basis 2027-Kostenstruktur)

| Modell | FTE-Jahre | Personalkosten | Vollkosten (inkl. Overhead) |
|---|---:|---:|---:|
| Ohne KI (30.000h) | 18,75 | €1,781M | **€3,195M** |
| Mit KI (8.000h) | 5,00 | €0,475M | **€0,852M** |
| Mit KI (12.000h) | 7,50 | €0,713M | **€1,278M** |

**Vollkosten-Effekt:** Einsparung **€1,917M–€2,343M**.

## 5.3 KI-Tooling-ROI (Kosten-Nutzenschwelle)

- KI-Tooling-Budget: €74.000/Jahr
- Break-even: 925h/Jahr
- Bereits geringe Produktivitätssteigerungen >925h/Jahr machen die KI-Kosten positiv.

### Befund 2

Das Replikationskostenmodell bestätigt einen strukturellen KI-Kostenvorteil; dadurch verschiebt sich der Moat von „reiner Entwicklungsaufwand“ hin zu „Compliance-, Integrations- und Vertriebsmoat“.

---

## 6. Ergebnisse III — EV-Bridge „Stand jetzt“ vs. „production-ready“

| Zustand | EV-Korridor (wahrscheinlich) |
|---|---:|
| Stand jetzt | €52M – €118M |
| Production-ready | €78M – €165M |
| **Absoluter Lift** | **+€26M bis +€47M** |
| **Relativer Lift** | **~39% bis ~50%** |

### Befund 3

Der größte kurzfristige Bewertungshebel ist nicht Feature-Ausbau, sondern die Reduktion von Governance-/Execution-Discounts (Sign-off, Nachweise, operativer Produktionsnachweis).

---

## 7. Unsicherheitsanalyse

## 7.1 Unsicherheitsquellen

1. TCO-Tabellen basieren auf dokumentierten Szenarioannahmen (nicht auf ex-post Zahlungsströmen).
2. Replikationsstunden (30.000h bzw. 8.000–12.000h) sind modellierte Bandbreiten.
3. EV-Spannen sind markt- und term-sheet-sensitiv (Multiple-Regime, Discount-Mechanik).

## 7.2 Auswirkungen auf Ergebnisrobustheit

- **Robust:** Richtung der Effekte (ThemisDB ROI positiv in allen Kernszenarien; KI reduziert Replikationskosten stark).
- **Mittel robust:** absolute EV-Punkte (abhängig von Governance-Closure-Timing und kommerzieller Traktion).
- **Sensitiv:** Military/strategische Prämienfälle.

---

## 8. Validität und Limitationen

1. Keine externen Primärdaten (Marktpreise) in dieser Fassung; Fokus strikt auf repository-interne Evidenz.
2. Unit-Economics (NRR, Win-Rate, Sales-Cycle) sind im Repo nur teilweise dokumentiert.
3. TCO-Szenarien enthalten modellhafte Strukturkosten und sind für Due-Diligence weiter zu kalibrieren.

---

## 9. Implikationen für Entscheidungsträger

1. **Finanzierung:** Production-ready-Pfad priorisieren, da EV-Lift unmittelbar ist.
2. **Go-to-Market:** Enterprise- und KRITIS-Fälle priorisieren (beste Risiko-Rendite-Profile).
3. **Produktstrategie:** KI-Effizienz aktiv monetär ausweisen, aber Moat auf Compliance + Operational Evidence verankern.

---

## 10. Reproduzierbarkeit (Audit-Trail)

Alle im Bericht ausgewiesenen Rechenergebnisse sind direkt aus den Tabellenwerten in den genannten Quellen hergeleitet; die Formeln sind in Abschnitt 3.2 vollständig offen gelegt.

---

## 11. Schlussfolgerung

Unter den dokumentierten Repository-Annahmen ist ThemisDB sowohl im Stand jetzt als auch production-ready monetär tragfähig.  
Der dominante Werthebel ist der Übergang in den governance-seitig vollständig freigegebenen Produktionszustand; der dominante Kostenhebel ist KI-beschleunigte Entwicklung.

---

## 12. Make, Buy or Rebuild?

## 12.1 Kernaussage

Aus den vorliegenden Zahlen folgt: Für die Mehrheit potenzieller Käufer ist **Lizenzkauf** wirtschaftlich rationaler als **Eigenbau** oder **Open-Source-Patchwork**, sobald mindestens einer der folgenden Faktoren relevant ist:

1. **Regulatorischer oder operativer Zwang zu On-Prem / Air-Gap / Souveränität**
2. **Hohe Integrations- und Verfügbarkeitsanforderungen**
3. **Zeitkritische Markteinführung**
4. **Fehlende interne Plattformmannschaft mit DB-, AI-, Security- und Compliance-Kompetenz**

Der Eigenbau wird erst dann rational, wenn ein Dritter gleichzeitig

- sehr große interne Engineering-Kapazität,
- einen langen Planungshorizont,
- proprietäre Sonderanforderungen,
- und die Bereitschaft zur Übernahme von Betriebs-, Compliance- und Zertifizierungsrisiken

mitbringt.

## 12.2 Build-vs-Buy-Interpretation aus den Replikationskosten

Die Replikationskosten liegen nach dem internen Modell bei:

- **ohne KI:** €2,400M direkte Kosten bzw. **€3,195M Vollkosten**
- **mit KI:** €0,640M–€0,960M direkte Kosten bzw. **€0,852M–€1,278M Vollkosten**

Demgegenüber stehen dokumentierte Lizenzpreisanker von:

- **Community:** €0
- **Enterprise:** €5.000/Jahr Basissubskription zzgl. Add-ons
- **Hyperscaler/OEM:** €250.000/Jahr Basispaket

Selbst unter KI-beschleunigter Entwicklung bleibt damit die Kostenrelation asymmetrisch: Die Lizenzkosten liegen für die meisten Einsatzprofile deutlich unter den modellierten Vollkosten eines Nachbaus. Der Bericht stützt daher die These, dass KI zwar den **Code-Erstellungsaufwand** reduziert, nicht aber den **Systemintegrations-, Hardening-, Sicherheits-, Zertifizierungs- und Vertriebsaufwand** eliminiert.

## 12.3 Entscheidungsmatrix

| Situation | Präferenz | Begründung |
|---|---|---|
| KRITIS / Behörden / Defense / Air-Gap | **Buy** | Souveränität, Auditierbarkeit und schnelleres Hardening dominieren |
| Regulierte Enterprise mit On-Prem-AI-Bedarf | **Buy / Hybrid** | Time-to-value und Integrationsvorteil stärker als Eigenbau-These |
| Sehr großer Plattformanbieter mit eigener DB-/AI-Organisation | **Build / Hybrid** | Eigenbau nur bei sehr hoher Skalierung und Sonderanforderungen plausibel |
| Kostengetriebene Standard-Cloud-Workloads ohne Compliance-Druck | **Nicht primär ThemisDB** | Hyperscaler-/Managed-Angebote ökonomisch und organisatorisch einfacher |

## 12.4 Mit KI vs. ohne KI

**Ohne KI** erscheint der Nachbau für die meisten Dritten ökonomisch unattraktiv.  
**Mit KI** sinkt der reine Replikationsaufwand stark, aber die eigentlichen Hürden verschieben sich auf:

- produktionsreife Architekturentscheidungen,
- Sicherheits- und Betriebsprozesse,
- Compliance-Nachweise,
- Domänenwissen für KRITIS / Defense / Souveränität,
- Beschaffung, Integration und Supportfähigkeit.

Damit wird der Moat kleiner auf der Ebene „Code schreiben“, aber größer auf der Ebene „vertrauenswürdig betreiben und verkaufen“.

---

## 13. Potenzielle Kunden und Beschaffungslogik

## 13.1 Priorisierte Zielsegmente

| Segment | Primärer Schmerz | Kaufmotiv | Beschaffungslogik |
|---|---|---|---|
| **KRITIS-Betreiber** | Cloud-/Souveränitätsgrenzen, Audit- und Verfügbarkeitsdruck | Compliance, Air-Gap, Kostenkontrolle | formaler Beschaffungsprozess, hoher Nachweisbedarf |
| **Behörden / öffentliche Verwaltung** | Datensouveränität, EU-/nationaler Kontrollanspruch | europäische On-Prem-Alternative | Vergabe-/Rahmenvertragslogik |
| **Verteidigung / sicherheitsnahe Integratoren** | abgeschottete Netze, Multi-Domain-Datenlage | Air-Gap + Resilienz + AI lokal | lange Sales-Zyklen, hohe Vertragswerte |
| **Industrie 4.0 / Edge-Produktion** | Latenz, Netztrennung, OT/IT-Nähe | lokale KI und Multi-Model-Datenhaltung | Pilot → Werksrollout |
| **Regulierte Enterprise** | Cloud-Risiko, Governance, Integrationskomplexität | TCO-Vorteil plus On-Prem-AI | IT-/Security-getriebene Kaufentscheidung |
| **OEM-/Plattformpartner** | Bedarf an integrierbarer Daten-/AI-Schicht | OEMisierung und Differenzierung | strategische Partnerschaft |
| **AI-on-Prem-Workloads** | lokale Inferenz, Datenresidenz | Vermeidung externer API-/Cloud-Abhängigkeit | projektbezogene technische Bewertung |

## 13.2 Wer zahlt am wahrscheinlichsten?

Die höchste kurzfristige Zahlungsbereitschaft ist dort zu erwarten, wo **Nicht-Kauf** oder **falsche Architekturwahl** operative oder regulatorische Kosten erzeugt. Das spricht zuerst für:

1. KRITIS
2. Behörden / öffentliche Verwaltung
3. Verteidigung / sicherheitsnahe Integratoren
4. regulierte Enterprise-Kunden mit On-Prem-AI-Anforderungen

SMB- und generische Cloud-Segmente bleiben zwar preislich adressierbar, sind aber im Vergleich weniger attraktiv, weil dort die Differenzierung schwächer und die Alternativen einfacher konsumierbar sind.

---

## 14. Empfohlene Einsatzfelder

## 14.1 Use-Case-Matrix

| Einsatzfeld | Fit | Warum |
|---|---|---|
| **Air-Gap-Datenplattform mit lokaler AI** | **MUSS-FIT** | ThemisDB adressiert genau die Kombination aus On-Prem, Multi-Model und AI |
| **KRITIS-/Sicherheitslagebild / Entscheidungsunterstützung** | **MUSS-FIT** | regulatorischer und operativer Druck begünstigt lokale integrierte Plattformen |
| **Industrie- / Edge-Analytics mit lokaler Inferenz** | **SOLL-FIT** | stark bei Latenz, Souveränität und gemischten Datenmodellen |
| **Regulierte Enterprise-Wissens- und Retrieval-Systeme** | **SOLL-FIT** | Mehrwert, wenn Retrieval, Governance und lokale Modelle kombiniert werden |
| **Allgemeine Standard-Web-Apps mit rein relationalem Profil** | **NICHT-PRIMÄR-FIT** | spezialisierte oder gemanagte Standarddatenbanken meist einfacher |
| **rein cloud-native, nicht regulierte Commodity-Workloads** | **NO-GO / NIEDRIGER FIT** | Hyperscaler-Ökosysteme gewinnen bei Einfachheit und Betrieb |

## 14.2 Edition-Fit

| Einsatzmuster | Empfohlene Edition |
|---|---|
| Evaluierung, Entwicklerzugang, kleine Pilotierung | **Community** |
| regulierte Produktion, Security-/Audit-Bedarf, bis 100 Knoten | **Enterprise** |
| OEM, Multi-Region, hohe Integrations- und SLA-Anforderungen | **Hyperscaler / OEM** |

---

## 15. Wettbewerbsalternativen und Economic Switching Logic

## 15.1 Reale Alternativen

Die praktische Konkurrenz für ThemisDB besteht nicht nur aus anderen Datenbanken, sondern aus vier Beschaffungsalternativen:

1. **Eigenbau**
2. **Open-Source-Patchwork**
3. **Hyperscaler-/Managed-Services**
4. **klassische Spezialdatenbanken / Einzelprodukte**

## 15.2 Economic Switching Logic

Lizenzkauf wird wirtschaftlich klar besser, wenn

- mehrere Datenmodelle zusammengeführt werden müssen,
- AI lokal und nicht API-basiert laufen soll,
- ein souveräner oder abgeschotteter Betrieb gefordert ist,
- Audit- und Betriebsverantwortung nicht auf mehrere Produkte fragmentiert werden soll,
- und die Opportunitätskosten verspäteter Markteinführung relevant sind.

Eigenbau oder Patchwork bleibt nur dann plausibel, wenn der Käufer

- bestehende Plattformbausteine bereits besitzt,
- keine kurze Time-to-Market braucht,
- den Integrationsaufwand organisatorisch absorbieren kann,
- und nicht primär von regulatorischer Nachweisfähigkeit lebt.

## 15.3 Strategische Lesart

Der stärkste strukturelle Vorteil von ThemisDB liegt nicht in einem einzelnen Feature, sondern in der **Bündelung** von:

- Multi-Model-Datenhaltung,
- lokaler AI/LLM-Nutzung,
- Air-Gap-/On-Prem-Betrieb,
- Sicherheits- und Auditierbarkeitsargumenten,
- sowie einer Lizenzlogik, die deutlich unter den Nachbaukosten liegt.

---

## 16. Adoptionsbarrieren, No-Go-Use-Cases und Einwände

## 16.1 Relevante Adoptionsbarrieren

1. **Zertifizierungs- und Compliance-Status** ist kaufentscheidend, insbesondere in KRITIS-/Behördenumfeldern.
2. **Vendor-Risk** bleibt relevant, solange externe Referenzkunden, belastbare SLA-Historie und längerfristige Betriebsnachweise noch wachsen.
3. **Integrationsaufwand** kann bei tief eingebetteten Enterprise-Landschaften hoch sein.
4. **Talent- und Betriebsmodell** auf Kundenseite kann die Einführung bremsen.
5. **Roadmap-Risiko** wirkt dort, wo Käufer sehr langfristige Standardisierung verlangen.

## 16.2 Wann ThemisDB noch nicht die richtige Wahl ist

ThemisDB ist nicht die ökonomisch beste Standardantwort für:

- unregulierte Commodity-SaaS-Workloads,
- rein relationale Anwendungen ohne AI-/Souveränitätsbedarf,
- Teams, die maximale Bequemlichkeit in Managed-Cloud-Diensten priorisieren,
- Kunden mit extrem niedriger Bereitschaft zu On-Prem-/Betriebsverantwortung.

## 16.3 Typische Einwände

| Einwand | Implikation für den Bericht |
|---|---|
| „Mit KI kann man das doch nachbauen.“ | Gegenüberstellung von Code-Replikation vs. Betriebs-/Compliance-Reife ausbauen |
| „Wir nutzen einfach mehrere Open-Source-Produkte.“ | Integrations-, Audit- und Betriebsfragmentierung quantitativ verdeutlichen |
| „Hyperscaler sind organisatorisch einfacher.“ | Nur für nicht-regulierte Cloud-Workloads uneingeschränkt richtig |
| „Wir warten auf mehr Marktbelege.“ | Externe Validierungsagenda explizit machen |

---

## 17. Proof Burden, Evidenzlücken und Forschungsagenda

## 17.1 Welche Aussagen sind intern gut gestützt?

Relativ gut intern gestützt sind aktuell:

- TCO-Richtungseffekte je Szenario
- Replikationskostenbandbreiten mit und ohne KI
- Preisanker der Editionen
- produktionsreifebezogene Argumentation via Audit- und Readiness-Artefakte

## 17.2 Welche Aussagen brauchen zusätzliche externe Validierung?

Externe Validierung ist insbesondere nötig für:

1. reale Zahlungsbereitschaft je Kundensegment
2. Vergleichspreise und Total-Cost alternativer Marktangebote
3. Beschaffungslogiken in Behörden- und Defense-Segmenten
4. Referenz-Deployments, SLA-Historien und Conversion-Raten
5. Markt-Multiples, Exit-Prämien und Sovereignty-Premiums außerhalb der Repository-Sicht

## 17.3 Umfassender Rechercheplan mit Subagents

| Subagent | Fokus | Kernoutput |
|---|---|---|
| **A — Build-vs-Buy Economics** | Lizenzkosten vs. Nachbau vs. Patchwork | Entscheidungsbaum nach Größe, Kritikalität, Compliance, Integrationsgrad |
| **B — Customer Segmentation & ICP** | Käuferprofile und Schmerzpunkte | priorisierte ICP-Liste mit Kaufmotiven und Beschaffungsweg |
| **C — Use-Case & Deployment Fit** | sinnvolle Einsatzfelder | Muss-/Soll-/Nicht-Fit-Matrix |
| **D — Competitive Alternatives** | reale Alternativen | Vergleich nach Kosten, Lock-in, Compliance, Air-Gap, AI |
| **E — Pricing & Willingness to Pay** | Preisarchitektur und Zahlungsbereitschaft | Preisanker, objections, mögliche Segmentpreise |
| **F — Compliance / Sovereignty / Procurement** | Regulierung als Kaufhebel | Beschaffungsargumentation für KRITIS, Behörden, Defense |
| **G — Replicability & Defensibility** | replizierbare vs. nicht replizierbare Moats | Moat-Matrix |
| **H — Adoption Risks & Objections** | Nichtkaufgründe | Risk Register plus Gegenmaßnahmen |
| **I — Evidence Validation** | Belastbarkeit der Aussagen | Evidenzklassen und externe Validierungsagenda |

## 17.4 Pflichtfragen für jeden Subagent

Jeder Teilstrang sollte dieselben Mindestfragen beantworten:

1. Zielsegment / Stakeholder
2. Kaufanlass und Problem
3. Alternative Vorgehensweisen
4. Wirtschaftliche Entscheidungskriterien
5. Risiken und Einwände
6. Required Evidence
7. Relevanz für Pricing, Go-to-Market und Bewertung

## 17.5 Reihenfolge der Recherche

1. Build-vs-Buy
2. Kundensegmente
3. Einsatzfelder
4. Wettbewerbsalternativen
5. Pricing / Willingness to Pay
6. Compliance / Procurement
7. Defensibility
8. Evidenzlücken und Red-Team-Gegenargumente

## 17.6 Erwartete Endartefakte

- überarbeiteter Entscheidungsbericht
- ICP- und Use-Case-Matrix
- Build-vs-Buy-Decision-Framework
- Risiko-/Einwandkapitel
- Liste offener externer Validierungen
- Management Summary für Investoren, Käufer und Vertrieb

---

## 18. Verdichtete Handlungsempfehlung

1. **Lizenz statt Nachbau** sollte als Default-Narrativ für regulierte, sicherheitskritische und souveränitätsgetriebene Käufer geführt werden.
2. **Build-vs-Buy** muss im nächsten Bericht nicht nur kostenbasiert, sondern auch zeit-, governance- und risikobasiert argumentiert werden.
3. **Go-to-Market** sollte zuerst auf KRITIS, Behörden, Defense und regulierte Enterprise-Segmente ausgerichtet werden.
4. **No-Go-Transparenz** erhöht Glaubwürdigkeit: Der Bericht sollte offen benennen, dass generische Commodity-Cloud-Workloads derzeit keine Primärdomäne von ThemisDB sind.
5. **Externe Validierung** ist die nächste Pflichtstufe, damit aus einer intern belastbaren Bewertungsunterlage eine investoren- und vertriebsfeste Marktthese wird.
