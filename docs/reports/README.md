# docs/reports

## Zweck
`docs/reports/` enthält Ergebnis- und Nachweisdokumente (Statusberichte, Ergebniszusammenfassungen, technische Reports).

## Pflegeverantwortung
- **Owner:** jeweilige Deliverable-Owner (Feature/Projekt)
- **Governance:** Documentation Governance Maintainer (Ablage, Lifecycle, Archivierung)

## Abgrenzung: Generiert vs. manuell
- **Manuell (hier):** kuratierte Abschluss-/Statusberichte
- **Teil-generiert (hier erlaubt):** konsolidierte Auswertungen mit redaktioneller Einordnung
- **Voll-generiert (nicht hier):** Rohartefakte nach `docs/_generated/`

## Archivierungs- und Aufräumregeln (verbindlich)
1. Reports müssen Datum und Scope im Titel oder Kopfbereich tragen.
2. Pro Themenbereich bleibt genau ein aktiver "aktueller" Report im Hauptpfad; ältere Stufenberichte gelten als archivierungsfähig.
3. Reports ohne aktive Verlinkung aus laufenden Issues/Roadmaps werden quartalsweise geprüft.
4. Veraltete Reports werden per `git mv` gemäß `docs/DOCUMENTATION_ARCHIVAL_PROCESS.md` nach `docs/archive/` verschoben (inkl. Archivhinweis).
5. Link-Integrität ist nach jedem Move Pflicht (z. B. `python3 scripts/link-check.py --internal-only docs/reports/README.md docs/FINAL_REPORT.md docs/FINAL_SUMMARY.md`).

## Unterordner
- `performance/` – Performance-spezifische Reports

_Letzte Governance-Aktualisierung: 2026-05-13._
