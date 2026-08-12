# Documentation Organization Plan

Stand: 2026-04-18

## Zielbild

`docs/` ist inhaltlich in Modulen und sprachspezifischen Bereichen organisiert. Der Root soll nur als klarer Einstieg dienen.

## Governance-Referenz

- Verbindliche PR-Regeln fuer Docs-Aenderungen: `governance/DOCS_PR_POLICY.md`

## Verbindliche Root-Dateien

- `README.md` (Startseite)
- `00_DOCUMENTATION_INDEX.md` (stabiler Master-Index)
- `DOCUMENTATION_HUB.md` (rollenbasiert)
- `CATEGORY_INDEX.md` (themenbasiert)
- `DOCS_ORGANIZATION_PLAN.md` (Regeln)

## Verzeichnisprinzip

- Sprachraeume: `de/`, `en/`
- Lernpfade: `tutorials/`, `use-cases/`, `knowledge-base/`, `certification/`
- Betrieb: `en/deployment/`, `en/operations/`, `replication/`, `production/`
- Querschnitt: `architecture/`, `security/`, `compliance/`, `observability/`
- Historie/Altmaterial: `ARCHIVED/`, `archive/`, `implementation-history/`

## Aufraeumregeln

- Keine neuen `tmp_*.md` Dateien im Root.
- Dateien mit Build-/Test-/Security-Output (`build_*.txt`, `test_*.txt`, `scout_cves_*.sarif`, `sec_block.txt`) sind keine Leitdokumente und duerfen nicht als redaktionelle Quelle behandelt werden.
- Implementierungszusammenfassungen und Ad-hoc-Review-Dumps nicht im Root halten.
- Neue Root-Dateien nur, wenn sie langfristiger Navigation dienen.
- Dokumente mit kurzer Halbwertszeit in `archive/` oder modulspezifische Unterordner verschieben.
- Relevanz-Triage immer mit zwei Signalen: interne Referenzen + Datum.
- Wenn Dateisystem-Zeitstempel vereinheitlicht sind, fuer Datumsbewertung Git-Historie (`git log -1 --format=%cs`) verwenden.

## Root-Artefakt-Regel (Build/Test/Security)

- `tmp_*.md` (Arbeitsnotizen) -> `docs/archive/tmp-notes/`
- `build_*.txt` (Build-Ausgaben) -> `logs/archive/`
- `test_*.txt` (Test-Ausgaben) -> `tests/outputs/` bzw. `tests/outputs/archive/`
- `scout_cves_*.sarif` (Scan-Rohdaten) -> `docs/audit-framework/evidence/<version>/scans/`
- `sec_block.txt` (temporäre Security-Hinweise) -> `docs/ARCHIVED/root-drafts/`

Detail-Inventar und aktuelle Triage siehe:
- `docs/de/reports/ROOT_ARTEFAKT_INVENTAR_2026-05.md`

## Durchgefuehrte Bereinigung (2026-04-18)

- `tmp_issue_*.md` aus dem Root nach `archive/tmp-notes/` verschoben.
- Navigationsdateien auf konsistente, valide Einstiegslinks reduziert.
- Doppelte und veraltete Linklisten in den Haupt-Hubs entfernt.
- 28 Root-Dateien mit Muster `*IMPLEMENTATION_SUMMARY*.md` nach `implementation-history/summaries/` verschoben.
- Ausgewaehlte Referenzen in bestehenden Docs auf die neuen Pfade aktualisiert.
- 13 Root-Dateien aus der Phase-Status-Welle (`PHASE*_COMPLETE|STATUS|PROGRESS`) nach `implementation-history/phases/` verschoben.
- Betroffene Verweise in `PRODUCTION_HARDENING_SUMMARY.md` auf die neuen Ziele angepasst.
- 14 Root-Dateien aus der Review-Welle (`*_REVIEW*.md`, `*CODE_REVIEW*.md`, finale Review-Reports) nach `implementation-history/reviews/` verschoben.
- Referenzen in `docs/ci-cd/README.md`, `docs/en/llm/CUDA_KERNEL_IMPLEMENTATION.md` und `docs/troubleshooting/core_troubleshooting.md` auf neue Ziele umgestellt.
- 24 Root-Dateien aus der Status/Complete-Welle nach `implementation-history/status-reports/` verschoben (Kriterium: 0 interne Referenzen + historisches Git-Datum).
- `RESET_COMPLETE.md` verbleibt vorerst im Root (juengstes Git-Datum in dieser Welle).
- 17 unreferenzierte Dokumentationsprozess-Snapshots nach `governance/documentation-history/` verschoben.
- 10 unreferenzierte Build-/Tooling-Snapshots nach `build-guide/` verschoben.

## Naechste Schritte

- Verbleibende Root-Snapshots regelmaessig erneut triagieren (Referenzen + Git-Datum), bevor sie verschoben werden.
- Link-Check in CI fuer die vier Root-Navigationsdateien etablieren.

## Aktuelle Root-Leitlinie

Im Root verbleiben bevorzugt:
- Einstieg, Navigation und Governance
- wenige zentrale Runbooks oder Hubs mit aktiver Querverlinkung
- keine abgeschlossenen Snapshot-Berichte ohne Referenzen
