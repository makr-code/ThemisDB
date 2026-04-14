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
6. Wenn Stubs, Mock-Pfade oder Simulationen im Sourcecode erforderlich sind, müssen sie explizit dokumentiert werden (Zweck, Aktivierungsbedingungen, Unterschiede zur Produktionslogik, geplanter Rückbau).

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

## 8) C++ Best Practices (VS Code/Copilot)

Bei C++-Aufgaben gelten folgende Leitlinien fuer Generierung, Review und Refactoring:

```yaml
cpp_best_practices:
  modern_cpp_features:
    - "Use 'auto' for type inference to improve readability."
    - "Prefer 'nullptr' over NULL or 0."
    - "Use 'constexpr' for compile-time computations."
    - "Use range-based for loops for container iteration."
    - "Prefer smart pointers (std::unique_ptr, std::shared_ptr) over raw pointers."

  resource_management_raii:
    - "Use RAII to bind resource lifetime to object lifetime."
    - "Use std::lock_guard or std::unique_lock for mutex locking."
    - "Avoid manual new/delete; prefer smart pointers."
    - "Ensure resources are released automatically when objects go out of scope."

  avoid_unnecessary_copies:
    - "Pass large objects by const reference (const &)."
    - "Use move semantics (std::move) for efficient transfer of ownership."
    - "Implement copy and move constructors appropriately."

  clear_and_safe_interfaces:
    - "Mark member functions that do not modify state as 'const'."
    - "Document side effects and exceptions clearly."
    - "Avoid global variables and mutable shared state."

  threading_and_synchronization:
    - "Use std::mutex with std::lock_guard for critical sections."
    - "Keep critical sections as short as possible."
    - "Use atomic operations (std::atomic) when feasible."
    - "Avoid deadlocks by consistent lock ordering or using std::lock."

  error_handling:
    - "Use exceptions for errors that cannot be handled locally."
    - "Write exception-safe code."
    - "Avoid silent failures and undefined behavior."

  performance:
    - "Profile code before optimizing."
    - "Avoid premature optimization."
    - "Use cache-friendly data structures and algorithms."
    - "Avoid false sharing in multithreaded code."

  style_and_readability:
    - "Use meaningful and descriptive names."
    - "Keep functions short and focused."
    - "Follow a consistent coding style guide."

  copilot_guidance:
    - "When generating C++ code, use modern language features like 'auto', smart pointers, and 'constexpr'."
    - "Avoid manual memory management; prefer RAII."
    - "Synchronize threads with std::mutex and std::lock_guard; keep critical sections short."
    - "Prevent deadlocks by consistent locking order."
    - "Write clear, const-correct, exception-safe functions."
    - "Optimize only after profiling and consider cache friendliness."
    - "If stubs, mocks, or simulation paths are introduced in source code, document them explicitly (purpose, activation conditions, production delta, and removal plan)."
```

  Empfohlenes Kommentar-Template direkt ueber Stub/Mock/Simulation-Codepfaden:

  ```cpp
  // STUB/SIMULATION NOTE:
  // Purpose: <why this non-production path exists>
  // Activation: <build flag/runtime condition/test-only gate>
  // Production Delta: <how behavior differs from production>
  // Removal Plan: <when/how this path will be removed>
  ```
