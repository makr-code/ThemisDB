# AI Developer Documentation Guide

Stand: 2026-05-31
Status: verbindlich fuer AI-unterstuetzte Entwicklerdokumentation

## 1) Zweck

Diese Richtlinie definiert fuer AI-Assistenten und Maintainer:
- wie Entwicklerdokumentation strukturiert ist,
- welche Information in welche Datei gehoert,
- wie Aenderungen validiert und gepflegt werden.

Ziel: keine inhaltlichen Dopplungen, kein Drift zwischen Code und Doku, klare Source-of-Truth pro Dokumenttyp.

## 2) Geltungsbereich

Gilt fuer entwicklernahe Modul-Dokumentation in:
- `src/<module>/`
- `include/<module>/`
- `tests/`
- `benchmarks/`

Nicht primaer betroffen:
- Enduser-/Admin-Dokumentation in `docs/` (siehe `PUBLIC_DOCUMENTATION_7PHASE_TEMPLATE.md`)
- Forschungsdokumente in `research/`

## 3) Source-of-Truth Matrix

| Datei | Rolle | Erlaubter Inhalt | Nicht hier ablegen |
|---|---|---|---|
| `README.md` | Einstieg ins Modul | Scope, Hauptkomponenten, Build/Test-Startpunkte, schnelle Orientierung | historische Changelogs, offene Backlog-Listen |
| `ARCHITECTURE.md` | Technischer Aufbau | Komponenten, Daten-/Kontrollfluss, Schnittstellen, Design-Entscheidungen | Release-Historie, operative Checklisten |
| `ROADMAP.md` | Zukunftsplanung | offene/in-progress Aufgaben, Ziele, Phasen fuer kommende Arbeit | erledigte Task-Archive |
| `FUTURE_ENHANCEMENTS.md` | mittelfristige Erweiterungen | Ideen, Optionen, Research-Richtung, harte Constraints | erledigte Implementierungsprotokolle |
| `CHANGELOG.md` | Historie | bereits gelieferte Aenderungen, Abschlussprotokolle, Versionsverlauf | offene Planung |
| `SECURITY.md` | Security-Realitaet | Threat Model, Controls, Grenzen, verifizierte Schutzmechanismen | Marketing-Claims ohne Codebezug |
| `AUDIT.md` | Pruefstatus | Audit-Findings, Nachweise, offene Auditpunkte | langfristige Featureplanung |
| `PRODUCTION_REQUIREMENTS.md` | verbindliche Prod-Regeln | MUST/MUST NOT, harte Betriebsbedingungen, Fail-Closed-Regeln | optionale Zukunftsfeatures |
| `PERFORMANCE_EXPECTATIONS.md` | Leistungsziele | messbare Ziele, Bench-Gates, SLO/SLA-relevante Grenzwerte | unvalidierte Schaetzungen ohne Messbezug |
| `MODULE_GAPS.md` | Gap-Scan-Output | automatisierte Ist-Luecken und Prioritaeten | manuell gepflegte Narrative |

Hinweis:
- `MODULE_GAPS.md` ist scanner-getrieben (read-only im Normalfall).
- Erledigte Roadmap-Bloecke werden ins `CHANGELOG.md` uebernommen, nicht in `ROADMAP.md` archiviert.

## 4) Pflege-Regeln fuer AI-Assistenten

1. Vor Doku-Aenderung zuerst gegen Sourcecode validieren (Symbole, APIs, Build-Flags, Laufzeitverhalten).
2. Bei Issue-/PR-Referenzen nur belastbare, repo-korrekte IDs verwenden.
3. Wenn ein Punkt abgeschlossen ist: aus `ROADMAP.md` entfernen und in `CHANGELOG.md` dokumentieren.
4. `ROADMAP.md` und `FUTURE_ENHANCEMENTS.md` bleiben zukunftsorientiert (nur offen/in progress).
5. Keine doppelte Wahrheit: Inhalte nur einmal als primaere Quelle fuehren.
6. Unklare Aussagen markieren oder entfernen, statt spekulativ zu formulieren.

Verbindliches Gate:
- Ohne Sourcecode-Abgleich darf keine Entwicklerdokumentation als "validiert" markiert werden.
- Aussagen ohne Codebeleg gelten als ungueltig und muessen entfernt, umformuliert oder mit offenem TODO-Marker versehen werden.

## 5) Minimaler Update-Workflow

1. Scope festlegen: welche Datei ist die Source-of-Truth fuer die geplante Aenderung.
2. Validieren:
   - Codepfad/Symbol pruefen (Pflicht)
   - optional Issue/PR pruefen
3. Editieren:
   - nur in der zustaendigen Datei
   - bei abgeschlossenen Punkten Changelog aktualisieren
4. Konsistenzcheck:
   - keine erledigten Punkte mehr in `ROADMAP.md`
   - keine offenen Punkte als erledigt im `CHANGELOG.md`
5. Nachweis aktualisieren:
   - Datum/Statuszeilen und Referenzen anpassen

## 6) Modul-Arbeitsanweisung (verbindlich pro bearbeitetem Modul)

Die folgenden Schritte sind fuer jedes betroffene Modul separat auszufuehren (z. B. `src/core`, `src/server`, `src/storage`).

1. Modul-Scope fixieren:
   - Welche Dateien im Modul werden aktualisiert?
   - Welche Datei ist je Aussage die primaere Source-of-Truth laut Matrix?
2. Sourcecode-Abgleich durchfuehren (Pflicht):
   - Symbole/Funktionen/Adapter gegen reale Dateien im Modul pruefen.
   - Laufzeitverhalten nur dokumentieren, wenn im Code oder Tests belegbar.
   - Build-Flags/Feature-Gates gegen CMake- oder Header-Definitionen pruefen.
3. Aussagen klassifizieren:
   - verifiziert: mit direktem Codebeleg.
   - offen: noch nicht belegbar, als offen markieren.
   - entfernen: historisch, spekulativ oder unbelegt.
4. Zukunft/Historie sauber trennen:
   - erledigt nach `CHANGELOG.md`.
   - offen/in progress in `ROADMAP.md` oder `FUTURE_ENHANCEMENTS.md`.
5. Modulabschluss dokumentieren:
   - kurzer Nachweisblock mit geprueften Kernsymbolen und Dateien.
   - Datum des letzten Sourcecode-Abgleichs setzen.

### 6.1 Pflicht-Nachweisblock je Modul

Bei Review oder PR muss pro bearbeitetem Modul ein Nachweis in dieser Form vorliegen:

```markdown
## Sourcecode Verification (Module: <module>)

- Scope-Dateien:
  - <path>
  - <path>
- Gepruefte Symbole/Verhalten:
  - <symbol or behavior> -> <source file>
  - <symbol or behavior> -> <source file>
- Gepruefte Build-/Feature-Guards:
  - <flag or adapter gate> -> <source file>
- Ergebnis:
  - [ ] Alle Aussagen belegt
  - [ ] Offene Punkte als offen markiert
  - [ ] Unbelegte Aussagen entfernt
```

Hinweis:
- Der Nachweisblock ist ein Merge-Gate fuer AI-unterstuetzte Entwicklerdoku.
- "Keine Aenderung noetig" ist nur zulaessig, wenn trotzdem ein kurzer Sourcecode-Abgleich dokumentiert wurde.

## 7) Review-Checkliste (kurz)

- Ist der Inhalt in der richtigen Datei laut Matrix?
- Ist jede relevante Aussage gegen Code oder belastbare Quelle verifiziert?
- Sind erledigte Punkte aus der Zukunftsplanung entfernt?
- Ist der Changelog fuer abgeschlossene Arbeit aktualisiert?
- Sind offene Punkte weiterhin klar als offen markiert?
- Ist pro bearbeitetem Modul ein Sourcecode-Nachweisblock vorhanden?

## 8) Verbindliche Referenzen

- `docs/DOCUMENTATION_REVIEW_GUIDELINES.md`
- `docs/PR_DOCUMENTATION_CHECKLIST.md`
- `docs/_standards/DOC_TEMPLATE.md`
- `docs/_standards/PUBLIC_DOCUMENTATION_7PHASE_TEMPLATE.md`