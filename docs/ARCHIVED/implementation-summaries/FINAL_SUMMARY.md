# Documentation Governance Structure - Final Summary

## Zielstatus

Die Meta-Bereiche sind strukturell geklärt und für QA/Governance eindeutig klassifiziert.

## Schnelle Zuordnung für AI und Maintainer

| Pfad | Klassifikation | Kurzregel |
|---|---|---|
| `docs/_generated/**` | Generiert | Nicht manuell pflegen, nur reproduzierbar erzeugen |
| `docs/_standards/**` | Redaktionell/Standard | Manuelle Pflege mit Reviewpflicht |
| `docs/issues/**` | Issue-Artefakt | Planungs- und Trackingdokumente, keine Rohreports |
| `docs/reviews/**` | Review-Artefakt | Signierte Review-Ergebnisse und Findings |
| `docs/reports/**` | Report-Artefakt | Ergebnisberichte, Lifecycle/Archivierung verpflichtend |
| `docs/website/**` | Redaktioneller Content | Marketing-/Website-Texte, keine Build-Outputs |

## Zentraler QA-/Review-Referenzpunkt

Alle Meta-Bereiche verweisen auf:

- `docs/DOCUMENTATION_MERGE_PROTOCOL.md`
- `docs/DOCUMENTATION_REVIEW_GUIDELINES.md`
- `docs/PR_DOCUMENTATION_CHECKLIST.md`

## Report-Lifecycle (Kurzfassung)

- Datum + Scope je Report verpflichtend
- Quartalsweise Bereinigung in `docs/reports/`
- Veraltete Reports nach `docs/archive/` verschieben
- Nach Verschiebung Link-Check ausführen

## Betroffene Governance-Dateien

- `docs/_generated/README.md`
- `docs/_standards/README.md`
- `docs/issues/README.md`
- `docs/reviews/README.md`
- `docs/reports/README.md`
- `docs/website/README.md`
- `docs/DOCUMENTATION_MERGE_PROTOCOL.md`
- `docs/FINAL_REPORT.md`
