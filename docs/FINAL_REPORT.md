# Documentation Governance Structure - Final Report

**Stand:** 2026-05-13
**Scope:** `docs/_generated/`, `docs/_standards/`, `docs/issues/`, `docs/reviews/`, `docs/reports/`, `docs/website/`

---

## 1) Ergebnisüberblick

Die Meta-Bereiche der Dokumentation sind nun mit klarer QA-/Governance-Zuordnung dokumentiert:

- Zweck pro Bereich ist beschrieben
- Pflegeverantwortung pro Bereich ist benannt
- Generierte und manuell gepflegte Inhalte sind getrennt
- Review- und Qualitätskriterien sind zentral referenziert
- Archivierungs- und Aufräumregeln für Reports sind definiert

---

## 2) Verantwortungs- und Strukturmatrix

| Bereich | Hauptzweck | Inhaltstyp | Pflegeverantwortung |
|---|---|---|---|
| `docs/_generated/` | Maschinell erzeugte Doku-Artefakte | Generiert | Docs Automation Maintainer |
| `docs/_standards/` | Vorlagen, Schemas, Schreibstandards | Manuell | Documentation Governance Maintainer |
| `docs/issues/` | Issue-nahe Planungs- und Arbeitsdokumente | Manuell | Modul-/Themen-Owner + Docs Governance |
| `docs/reviews/` | Fachreview-/Code-Review-Artefakte | Manuell (signiert) | Review Leads / Architekturteam |
| `docs/reports/` | Ergebnis- und Statusreports | Manuell + teil-generiert | Deliverable-Owner + Docs Governance |
| `docs/website/` | Redaktionelle Website-Inhalte | Manuell | Product Marketing / Documentation Team |

---

## 3) Zentrale QA- und Review-Referenzen

- `docs/DOCUMENTATION_REVIEW_GUIDELINES.md`
- `docs/SYSTEMATISCHER_REVIEWPLAN.md`
- `docs/PR_DOCUMENTATION_CHECKLIST.md`
- `docs/de/development/SOURCE_CODE_AUDIT.md`
- `docs/audit-framework/AUDIT_RUNBOOK.md`
- `docs/DOCUMENTATION_MERGE_PROTOCOL.md`

Diese Referenzen sind in den betroffenen Meta-README-Dateien verankert.

---

## 4) Archivierungs- und Aufräumregeln für Reports

Verbindlich für `docs/reports/`:

1. Reports enthalten Datum + Scope.
2. Pro Themenbereich bleibt ein aktiver Hauptreport im Hauptpfad.
3. Quartalsweise Prüfung auf veraltete, nicht mehr aktiv verlinkte Reports.
4. Archivierung über `git mv` nach `docs/archive/` gemäß `docs/DOCUMENTATION_ARCHIVAL_PROCESS.md`.
5. Nach Verschiebungen ist ein Link-Check verpflichtend.

---

## 5) Durchgeführter Doku-Audit (für dieses Issue)

**Prüfumfang (Dateien):**

- `docs/_generated/README.md`
- `docs/_standards/README.md`
- `docs/issues/README.md`
- `docs/reviews/README.md`
- `docs/reports/README.md`
- `docs/website/README.md`
- `docs/FINAL_REPORT.md`
- `docs/FINAL_SUMMARY.md`
- `docs/DOCUMENTATION_MERGE_PROTOCOL.md`

**Prüfart:** Dokumentationsaudit gegen die oben genannten Checklisten/Runbooks.
