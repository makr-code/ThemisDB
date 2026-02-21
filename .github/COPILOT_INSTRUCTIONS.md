# Copilot Instructions for Roadmap-Driven Implementation

Diese Regeln steuern, wie Copilot in diesem Repository aus `ROADMAP.md` und `future_enhancement.md` produktive Implementierungen erzeugt.

## 0) Governance & Standards Reference

**WICHTIG**: Alle Issues und PRs MÜSSEN der `.github/GOVERNANCE.md` entsprechen.

Siehe: [GOVERNANCE.md](.github/GOVERNANCE.md) für verbindliche Standards zu:
- Label-Schema (`area:*`, `priority:*`, `type:*`, `status:*`)
- Milestone-Struktur (Quartals- und Version-basiert)
- Relationships (Parent/Child, Blocking Dependencies)
- Metadaten-Standards (Title, Description, Assignee)

### 0.1 Mandatory Issue/PR Metadata

Jedes neue Issue MUSS folgende Metadaten haben:
- `area:*` Label (z.B. `area:core`, `area:aql`, `area:acceleration`, `area:query`, `area:storage`, etc.)
- `priority:*` Label (critical, high, medium, low)
- `type:*` Label (feature, bug, test, documentation, refactor, chore)
- `status:*` Label (open, in_progress, blocked, ready, review)
- **Milestone** (Quartal Q1-Q4 2026 oder Version v1.x.x)
- **Clear title** (< 60 Zeichen, prägnant)
- **Detailed description** mit Kontext, Ziel und Akzeptanzkriterien

### 0.2 Relationship Management

Bei jeder Issue-Erstellung/Bearbeitung prüfen:
- Gibt es **parent epic** oder **blocking issues**?
- Soll diese Issue mit einer **roadmap task** verknüpft werden?
- Verwende GitHub Issue Links: `Relates to #123`, `Depends on #456`, `Fixes #789`, `Blocks #101`
- Dokumentiere Relationships in Issue-Description

---

## 1) Ziel

Roadmap-Einträge müssen so konkret sein, dass Copilot **produktiven Sourcecode** statt Stub/Rumpf erzeugen kann.

## 2) Pflichtstruktur für `ROADMAP.md` je Modul

Jede Modul-Roadmap MUSS diese Abschnitte enthalten:

1. `## Current Status`
2. `## In Progress` und/oder `## Planned Features`
3. `## Implementation Phases` mit `### Phase 1 ... ### Phase N`
4. `## Production Readiness Checklist`
5. `## Known Issues & Limitations`
6. `## Breaking Changes` (falls relevant)

### 2.1 Checkbox-Status

- `[ ]` offen
- `[~]` in Bearbeitung
- `[x]` erledigt
- `[I]` Issue vorhanden
- `[P]` Pull Request vorhanden
- `[?]` Human question/blockiert
- `[!]` unklarer/zu prüfender Zustand

### 2.2 Aufgabenformat (Pflicht)

Jede umzusetzende Aufgabe muss als Checkbox vorliegen und nach Möglichkeit einen Target-Hinweis haben:

- `- [ ] <konkrete technische Aufgabe> (Target: <Milestone/Quartal>)`

Beispiel:

- `- [ ] CUDA geospatial distance and containment kernels (Target: Q3 2026)`

### 2.3 Verknüpfung mit Issues/PRs (NEU)

Jede roadmap task SOLLTE mindestens ein Issue haben: