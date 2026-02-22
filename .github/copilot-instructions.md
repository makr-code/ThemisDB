# Copilot Instructions for Roadmap-Driven Implementation

Diese Regeln steuern, wie Copilot in diesem Repository aus `ROADMAP.md` und `future_enhancement.md` produktive Implementierungen erzeugt.

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

Regel: keine vagen Formulierungen wie „improve“, „optimize“ ohne messbares Ziel.

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

## 7) Beispiel für guten Roadmap-Eintrag

```markdown
- [ ] CUDA geospatial distance and containment kernels (Target: Q3 2026)
  - Inputs: WGS84 points/polygons, batch-size up to 1e6
  - Outputs: distance matrix + containment bitset
  - Constraints: deterministic FP tolerance <= 1e-6
  - Errors: invalid geometry, NaN coordinates, overflow
  - Tests: unit + property-based + GPU/CPU parity
  - Perf: >= 8x speedup vs CPU baseline on RTX-class GPU
```

Dieser Detaillierungsgrad ist für produktiven Code erforderlich.

## 8) Research Documentation Policy

```yaml
research_documentation:
  policy: "MANDATORY für neue Algorithmen/Designs"

  when_to_document:
    - "Paper oder wissenschaftliche Quelle als Grundlage"
    - "Best Practice aus anderen Open-Source Projekten übernommen"
    - "Stand der Technik recherchiert (z.B. neue GPU-Techniken)"
    - "Architecture Decision getroffen (z.B. HNSW vs. FAISS)"

  workflow:
    - "1. Quelle recherchieren → in /docs/research/<typ>/ als .md speichern"
    - "2. Spezifische Anpassungen für ThemisDB dokumentieren"
    - "3. Im relevanten Modul-README verlinken (Abschnitt: Wissenschaftliche Grundlagen & Einflüsse)"
    - "4. In /docs/research/implementation_influence/ eintragen"
    - "5. Commit-Message: ref(research): Add [Source] to [Module]"

  pr_checklist:
    - "[ ] Basiert auf wissenschaftlicher Quelle oder Best Practice?"
    - "[ ] Wenn ja: Research-Datei in /docs/research/ angelegt?"
    - "[ ] Modul-README mit Influence-Link aktualisiert?"
    - "[ ] Implementation-Influence-Index aktualisiert?"

  templates:
    paper: "docs/research/papers/_template_paper.md"
    best_practice: "docs/research/best_practices/_template_best_practice.md"
    architecture_decision: "docs/research/architecture_decisions/_template_decision.md"

  guide: "docs/research/RESEARCH_GUIDE.md"
```
