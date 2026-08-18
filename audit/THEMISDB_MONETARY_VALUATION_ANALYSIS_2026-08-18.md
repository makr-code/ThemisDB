# ThemisDB Monetäre Bewertung 2026-08-18 (wissenschaftlich strukturierte Fassung)

**Klassifikation:** Vertraulich  
**Scope:** Finanzierungs-/M&A-Entscheidungsunterlage  
**Source of Truth:** `/audit/**`

---

## Abstract

Diese Studie quantifiziert den monetären Wert von ThemisDB in zwei Zuständen (**Stand jetzt** vs. **production-ready**) anhand eines reproduzierbaren Modells aus (a) TCO-ROI, (b) Replikationskosten mit/ohne KI-Entwicklung, (c) Bewertungs-Multiplikatoren und (d) Governance-/Execution-Discounts.  
Die Ergebnisse zeigen einen **EV-Lift von +€26M bis +€47M** beim Übergang von „Stand jetzt“ zu „production-ready“ sowie eine signifikante **Cost-to-Recreate-Reduktion um 60%–73,3%** durch KI-beschleunigte Entwicklung.

---

## 1. Forschungsfragen

1. Wie hoch ist der monetäre Wert von ThemisDB im aktuellen Zustand im Vergleich zu einem production-ready Zustand?
2. Wie groß ist der wirtschaftliche Vorteil (ROI, Payback) gegenüber alternativen Architekturansätzen?
3. Wie unterscheiden sich Replikationskosten „ohne KI“ vs. „mit KI“ methodisch und quantitativ?

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

