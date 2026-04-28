> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks überführen. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

# chimera roadmap

## Current Status

- `ThemisDBAdapter` ist für den Simulationsmodus implementiert (`src/chimera/themisdb_adapter.cpp`).
- Streaming (`IStreamingAdapter`) und Prepared Statements (`IPreparedStatementAdapter`) sind implementiert und durch dedizierte Tests abgedeckt.
- Erweiterte Multi-Vendor-Adapterlandschaft ist im Modulpfad `src/chimera/` aktuell nicht vorhanden.

## In Progress

- [x] Konsolidierung der capability claims für Simulations- vs. Engine-Modus (Target: v1.8.0)
  - `Capability::CONNECTION_POOLING` aus `has_capability()` und `get_capabilities()` entfernt (falsely advertised → false); Test ergänzt (2026-04-21)
- [x] Präzisierung und Nachrüstung von Include-Dokumentation unter `include/chimera/` (Target: v1.8.0)

## Planned Features

- [ ] Engine-Dispatch-Pfade ohne `NOT_IMPLEMENTED`-Fallback für alle deklarativen Capabilities (Target: Q3 2026)
- [ ] Konsistente Connection-Pooling-Unterstützung (Capability + API + Tests) (Target: Q3 2026)
- [ ] Vendor-Adapter-Registry und dokumentierte Integrationsverträge im Modul selbst (Target: Q4 2026)

## Implementation Phases

### Phase 1 — Design / API-Vertrag

- [x] Adapter-API für Streaming/Prepared Statements im Header-Shim definiert (Target: v1.9.0)
- [ ] Connection-Pooling-Vertrag eindeutig in Header + Implementierung abbilden (Target: v1.8.0)

### Phase 2 — Core-Implementierung

- [x] Simulationsmodus für relationale, dokumenten-, graph- und vektorbezogene Grundpfade (Target: v1.9.0)
- [x] Streaming- und Prepared-Statement-Basisimplementierung (Target: v1.9.0)
- [ ] Vollständige engine-backed Implementierung ohne Build-Flag-Lücken (Target: Q3 2026)

### Phase 3 — Fehlerbehandlung & Edge Cases

- [x] Verbindungsprüfung und strukturierte Fehlercodes in Kernpfaden (Target: v1.9.0)
- [x] Einheitliche Behandlung von Feature-Mismatch zwischen Capabilities und Verhalten (Target: v1.8.0)
  - `CONNECTION_POOLING` liefert nun korrekt `false` bis zur Implementierung (2026-04-21)

### Phase 4 — Tests

- [x] Streaming-Tests vorhanden (`tests/chimera/test_chimera_streaming.cpp`) (Target: v1.9.0)
- [x] Prepared-Statement-Tests vorhanden (`tests/chimera/test_chimera_prepared_statements.cpp`) (Target: v1.9.0)
- [x] Engine-injection-Tests für reale Backend-Pfade ergänzen (Target: Q3 2026)
  - CHI-EI-01..09 in `tests/chimera/test_themisdb_adapter.cpp` (`ThemisDBEngineInjectionTest`)
  - Prüfen: NOT_IMPLEMENTED bei injiziertem Engine-Pointer ohne `THEMISDB_ENGINE_AVAILABLE`
  - Alle 4 NOT_IMPLEMENTED-Pfade (execute_query, search_vectors, shortest_path, traverse) abgedeckt

### Phase 5 — Performance/Hardening

- [ ] Connection-Pooling, Retry-Strategien und Ressourcenhärtung implementieren (Target: Q3 2026)
- [ ] Last- und Speicherprofiling für große Batch-/Stream-Workloads dokumentieren (Target: Q3 2026)

### Phase 6 — Dokumentation & Abnahme

- [x] Modulbezogene Primary/Secondary-Doku-Migration gestartet (`docs/de|en/chimera/*`) (Target: v1.8.0)
- [x] Include-Readme und konsolidierte Modulnavigationspfade fertigstellen (`include/chimera/README.md`) (Target: v1.8.0)

## Production Readiness Checklist

- [x] Simulationsmodus stabil für lokale Tests
- [x] Streaming- und Prepared-Statement-Pfade testbar
- [x] Capability-Matrix deckt reales Verhalten ohne Widersprüche ab
  - `CONNECTION_POOLING` korrekt auf `false` gesetzt bis zur Implementierung (2026-04-21)
- [x] Engine-injection-Pfade explizit getestet: NOT_IMPLEMENTED-Guard (CHI-EI-01..09) bestätigt deterministisches Verhalten bei fehlendem `THEMISDB_ENGINE_AVAILABLE` (2026-04-28)
- [x] Include-API-Doku vollständig und mit Sourcecode konsistent (`include/chimera/README.md`)
- [ ] Sicherheits- und Lasttests für produktive Adapteranbindung dokumentiert

## Known Issues & Limitations

- Nur ThemisDB-Referenzadapter ist im Modulpfad `src/chimera/` vorhanden.
- Teile des engine-backed Dispatches hängen an `THEMISDB_ENGINE_AVAILABLE`.
- `Capability::CONNECTION_POOLING` wird als verfügbar gemeldet, ohne dedizierte Pooling-API-Implementierung im Adapter. (**Fix 2026-04-21**: `has_capability()` gibt jetzt `false`, `get_capabilities()` enthält `CONNECTION_POOLING` nicht mehr.)
- Include-Dokumentation (`include/chimera/README.md`) fehlt derzeit nicht mehr — vorhanden.
- `shortest_path` mit `max_depth != 10` verwendet `dijkstraWithConstraints` (engine-backed); `traverse` mit mehreren `edge_labels` läuft als BFS-pro-Label mit Deduplication (engine-backed).

## Breaking Changes

Aktuell keine bestätigten Breaking Changes geplant.
Falls Connection-Pooling-Verträge oder Capability-Semantik geändert werden, muss dies als Breaking Change dokumentiert werden.
