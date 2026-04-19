# Docs PR Policy (ThemisDB)

Stand: 2026-04-18

## Zweck

Diese Policy definiert verbindliche Regeln fuer Pull Requests, die Dokumentation unter `docs/` aendern. Ziel ist ein stabiler, leicht navigierbarer Docs-Bereich mit klarer Trennung zwischen dauerhaftem Wissen und historischer Projekthistorie.

## GitHub Expectation

Nutzer erwarten auf GitHub:
- klare Einstiegsnavigation
- stabile Links
- sichtbare Projekt-Standards (`README`, `CONTRIBUTING`, `SECURITY`, `SUPPORT`)
- keine Ueberfrachtung des `docs/`-Root mit Sprint- oder Snapshot-Artefakten

## Geltungsbereich

Diese Regeln gelten fuer alle Markdown-Dateien unter `docs/`.

## Root-Regel (verbindlich)

Der `docs/`-Root enthaelt nur dauerhafte Navigations- und Steuerungsdokumente.

Neue Dateien im Root sind nur erlaubt, wenn sie:
- als langfristiger Einstieg dienen,
- mehrfach querverlinkt werden,
- und keine zeitgebundene Snapshot-Dokumentation sind.

## Entscheidungsbaum: Root vs. History

Pruefe fuer jede neue oder geaenderte Datei:

1. Ist die Datei ein dauerhafter Einstieg (Hub/Index/Policy)?
- Ja: Root oder passender Governance-Bereich.
- Nein: weiter mit 2.

2. Ist die Datei zeitgebunden (Complete/Status/Review/Final Summary, Implementierungsstand, Sprint-Ergebnis)?
- Ja: in `docs/implementation-history/` (passende Unterkategorie).
- Nein: weiter mit 3.

3. Wird die Datei aktiv als Referenz fuer Betrieb/Entwicklung genutzt?
- Ja: in passendem Fachordner (z. B. `operations/`, `security/`, `architecture/`).
- Nein: in `docs/archive/` oder `docs/implementation-history/`.

## Relevanz-Triage fuer bestehende Root-Dateien

Bei Aufraeumarbeiten gilt diese Reihenfolge:

1. Interne Referenzen zaehlen (docs-intern).
- `0` Referenzen: Kandidat fuer Verschiebung.
- `>0` Referenzen: zuerst Linkstrategie klaeren.

2. Datumssignal bewerten.
- Primaer: Git-Historie (`git log -1 --format=%cs -- <datei>`).
- Sekundaer: Dateisystem-Zeitstempel nur, wenn nicht vereinheitlicht.

3. Entscheidung dokumentieren.
- Verschoben/Behalten mit kurzer Begruendung in `DOCS_ORGANIZATION_PLAN.md`.

## Zielstruktur fuer historische Dokumente

- `docs/implementation-history/summaries/`
- `docs/implementation-history/phases/`
- `docs/implementation-history/reviews/`
- `docs/implementation-history/status-reports/`
- `docs/archive/` fuer unsortierte Alt- oder Hilfsdokumente

## PR-Checklist (Docs)

Jeder Docs-PR sollte beantworten:

- Zweck: Warum ist die Aenderung noetig?
- Ablage: Warum dieser Zielordner?
- Navigation: Welche Hub-/Index-Links wurden angepasst?
- Links: Wurden lokale Links geprueft?
- Historie: Falls verschoben, ist die alte Auffindbarkeit erhalten?
- Root-Hygiene: Wurde Root-Bloat vermieden?

## Link-Qualitaet

- Keine neuen Broken Links.
- Relative Links bevorzugen.
- Bei Moves: alte Verweise im gesamten `docs/`-Baum nachziehen.

## Ausnahmen

Kurzfristige Ausnahmen sind moeglich, muessen aber im PR begruendet werden mit:
- geplanter Rueckbauzeit,
- Zielordner nach Bereinigung,
- Verantwortlicher Person/Team.

## Review-Kriterien

Ein Docs-PR gilt als freigabefaehig, wenn:
- Strukturregeln eingehalten sind,
- Links konsistent sind,
- Root nicht weiter aufblaeht,
- und die Aenderung fuer externe GitHub-Leser nachvollziehbar ist.
