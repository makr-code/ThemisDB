# docs/_generated

## Zweck
`docs/_generated/` enthält ausschließlich generierte Dokumentationsartefakte (Indexe, Exporte, Maschinen-Outputs).

## Pflegeverantwortung
- **Owner:** Docs Automation Maintainer (ThemisDB Core Team)
- **Änderungen:** Nur durch Generatoren/Skripte oder reproduzierbare Pipeline-Läufe
- **Manuelle Edits:** Nur für klar dokumentierte Hotfixes, danach zeitnah erneut generieren

## Abgrenzung: Generiert vs. manuell
- **Generiert (hier):** `primary_index.json`
- **Manuell gepflegt (nicht hier):** Vorlagen/Standards in `docs/_standards/`, redaktionelle Inhalte in `docs/website/`

## Hinweise
- Änderungen in diesem Ordner sollten mit den übergeordneten Architektur- und Sicherheitsrichtlinien des Projekts abgestimmt werden.
- Für tieferliegende Teilbereiche existieren ggf. zusätzliche README- und Moduldokumente.
- Der Ordner wird automatisiert befüllt (z. B. via `tools/primary_docs_indexer.py`) und dient als Datenquelle für Doku-Checks.
- Inhalte aus `docs/_generated/**` sind kein direkter MkDocs-Publish-Einstieg; siehe `docs/README-DOCUMENTATION.md` für den Build-/Publish-Flow.
## QA-/Review-Anbindung
- Validierung über zentrale Review-Kriterien in `docs/DOCUMENTATION_MERGE_PROTOCOL.md`
- Fachliche Review-Leitlinien in `docs/DOCUMENTATION_REVIEW_GUIDELINES.md`

_Letzte Governance-Aktualisierung: 2026-05-13._
