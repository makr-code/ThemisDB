# Copilot Instructions for Roadmap-Driven Implementation

Diese Regeln steuern, wie Copilot in diesem Repository aus `ROADMAP.md` und `future_enhancement.md` produktive Implementierungen erzeugt.

## 0) Governance & Standards

**WICHTIG**: Alle Issues und PRs MÜSSEN der `.github/GOVERNANCE.md` entsprechen.

Siehe: [GOVERNANCE.md](.github/GOVERNANCE.md) für verbindliche Standards zu:
- Label-Schema (`area:*`, `priority:*`, `type:*`, `status:*`)
- Milestone-Struktur (Quartals- und Version-basiert)
- Relationships (Parent/Child, Blocking Dependencies)
- Metadaten-Standards (Title, Description, Assignee)

### 0.1 Mandatory Issue/PR Metadata

Jedes neue Issue MUSS folgende Metadaten haben:
- `area:*` Label (z.B. `area:core`, `area:aql`, `area:acceleration`, `area:query`)
- `priority:*` Label (critical, high, medium, low)
- `type:*` Label (feature, bug, test, documentation, refactor)
- `status:*` Label (open, in_progress, blocked, ready)
- **Milestone** (Quartal oder Version)
- **Clear title** (< 60 Zeichen)
- **Detailed description** mit Kontext und Akzeptanzkriterien

### 0.2 Relationship Management

Bei der Issue-Erstellung prüfen:
- Gibt es **parent epic** oder **blocking issues**?
- Soll diese Issue mit einer **roadmap task** verknüpft werden?
- Verwende GitHub-Links: `Relates to #123`, `Depends on #456`, `Fixes #789`

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

### 2.3 Verknüpfung mit Issues/PRs

Jede roadmap task SOLLTE mindestens ein Issue haben:
- `- [ ] <task> (Target: Q3 2026) - Issue: #1234, PR: #5678`

## 3) Pflichtstruktur für `future_enhancement.md`

Wenn vorhanden, MUSS die Datei pro Modul klare, implementierbare Hinweise enthalten:

```markdown
## <module-name>

### Scope
- ...

### Design Constraints
- ...

### Required Interfaces
- ...

### Implementation Notes
- ...

### Test Strategy
- ...

### Performance Targets
- ...

### Security / Reliability
- ...
```

Regel: keine vagen Formulierungen wie „improve", „optimize" ohne messbares Ziel.

## 4) Qualitätsanforderungen für Roadmap-Items

Jedes implementierbare Item soll enthalten:

- betroffene Subsysteme/Dateien/Namespaces
- erwartetes Laufzeitverhalten
- Fehlerfälle und Validierung
- Testanforderungen (Unit/Integration)
- messbare Performance-Ziele (wo relevant)
- Kompatibilitäts-/Migrationshinweise

## 5) Phasenmodell (verbindlich)

`## Implementation Phases` muss mindestens folgende Phasen abdecken:

- Phase 1: Design / API-Vertrag
- Phase 2: Core-Implementierung
- Phase 3: Fehlerbehandlung & Edge Cases
- Phase 4: Tests
- Phase 5: Performance/Hardening
- Phase 6: Dokumentation & Abnahme

Jede Phase braucht konkrete Bullet-Tasks, keine Platzhalter.

## 6) Copilot-Ausführungsregeln

Beim Implementieren aus Roadmap/Future-Enhancement gilt:

1. Keine Stub-Methoden ohne Produktionslogik.
2. Kein rein syntaktischer "TODO-Code" als Endergebnis.
3. Tests müssen reale Funktionalität verifizieren.
4. Akzeptanzkriterien aus Roadmap sind bindend.
5. Bei fehlenden Details zuerst Roadmap/Future-Enhancement präzisieren statt raten.
6. **Metadaten-Compliance**: Jede implementierte Task muss das entsprechende Issue aktualisieren mit:
   - Status-Label auf `status:in_progress` setzen
   - PR mit Issue verknüpfen: `Fixes #XXXX`
   - Milestone korrekt gesetzt

## 7) Beispiel für guten Roadmap-Eintrag

```markdown
- [ ] CUDA geospatial distance and containment kernels (Target: Q3 2026) - Issue: #1234, PR: #5678
  - Inputs: WGS84 points/polygons, batch-size up to 1e6
  - Outputs: distance matrix + containment bitset
  - Constraints: deterministic FP tolerance <= 1e-6
  - Errors: invalid geometry, NaN coordinates, overflow
  - Tests: unit + property-based + GPU/CPU parity
  - Perf: >= 8x speedup vs CPU baseline on RTX-class GPU
  - Status: [x] erledigt | PR: makr-code/ThemisDB#5678 | Milestone: Q3 2026
```

Dieser Detaillierungsgrad ist für produktiven Code erforderlich.

## 8) GitHub Issue/PR Template Compliance

Alle Issues/PRs MÜSSEN folgendes erfüllen (siehe auch `.github/GOVERNANCE.md`):

### Issue Template
```
## Title
[area:core] Feature XYZ - Kurze Beschreibung

## Description
- Kontext: Warum ist das wichtig?
- Ziel: Was soll erreicht werden?
- Akzeptanzkriterien: Was ist "fertig"?

## Labels
- area:* (mandatory)
- priority:* (mandatory)
- type:* (mandatory)
- status:* (mandatory)

## Milestone
- [Quartal/Version wählen]

## Relationships
- Relates to: #123
- Depends on: #456
- Blocks: #789
```

### PR Template
```
## Title
[area:core] PR: Feature XYZ - Kurze Beschreibung

## Description
Fixes #XXXX

- [ ] Tests hinzugefügt/aktualisiert
- [ ] Dokumentation aktualisiert
- [ ] Labels korrekt gesetzt
- [ ] Milestone gesetzt
- [ ] Roadmap-Eintrag aktualisiert (falls zutreffend)
```