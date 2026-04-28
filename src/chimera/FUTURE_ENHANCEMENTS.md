> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

# Future Enhancements

## chimera

### Scope
- Vereinheitlichung der CHIMERA-Adaptersemantik zwischen Simulationsmodus und engine-backed Betrieb.
- Schließen dokumentierter Lücken aus `src/chimera/ROADMAP.md` für v1.8.0+.

### Design Constraints
- Keine ABI-instabilen Brüche in `include/chimera/themisdb_adapter.hpp` ohne Migrationshinweis.
- Feature-Claims in `has_capability/get_capabilities` müssen dem tatsächlichen Verhalten entsprechen.
- Engine-spezifische Pfade müssen bei fehlender Backend-Verfügbarkeit deterministisch fehlerschlagen.

### Required Interfaces
- Bestehende Interfaces: `IDatabaseAdapter`, `IAsyncDatabaseAdapter`, `IStreamingAdapter`, `IPreparedStatementAdapter`.
- Zu präzisieren: Connection-Pooling-Vertrag (Capabilities, API, Fehlerfälle, Telemetrie).

### Implementation Notes
- Build-Flag-abhängige `NOT_IMPLEMENTED`-Pfade in engine-backed Routen schrittweise ersetzen.
  - Status 2026-04-28: Alle 4 Pfade mit STUB/SIMULATION NOTEs dokumentiert;
    CHI-EI-01..09 bestätigen deterministisches Verhalten bei fehlendem `THEMISDB_ENGINE_AVAILABLE`.
  - Nächster Schritt: cmake/ChimeraAdapters.cmake so erweitern, dass Engine-Injection
    automatisch `THEMISDB_ENGINE_AVAILABLE` definiert (Target: Q3 2026).
- Simulationspfad als deterministische Test-Basis beibehalten, aber klar gegen produktive Pfade abgrenzen.
- Optionaler Engine-Dispatch (`query_engine_`, `vector_index_`, `graph_index_`) soll konsistente Fehlercodes und Semantik liefern.

### Test Strategy
- Bestehende Tests (`tests/chimera/test_chimera_streaming.cpp`, `tests/chimera/test_chimera_prepared_statements.cpp`) als Regression-Basis.
- Engine-Injection-Tests CHI-EI-01..09 in `tests/chimera/test_themisdb_adapter.cpp` (`ThemisDBEngineInjectionTest`) eingeführt (2026-04-28).
  - Prüfen: `ErrorCode::NOT_IMPLEMENTED` bei allen 4 engine-dispatched Methoden wenn `THEMISDB_ENGINE_AVAILABLE` fehlt.
  - CMake-Ziel: `ThemisDBAdapterFocusedTests` (aktiv mit `-DTHEMIS_BUILD_CHIMERA=ON`).
- Für neue Pooling-/Retry-Features: Unit-Tests + Fehlerpfadtests (Timeout, Cancel, Invalid Config).

### Performance Targets
- Streaming: keine zusätzliche lineare Kopierlast pro Batch gegenüber aktuellem Simulationspfad.
- Prepared Statements: Wiederverwendung ohne regressiven Overhead bei wiederholtem `execute()`.
- Künftige Pooling-Features: messbarer Verbindungsaufbau-Overhead < 20% gegenüber wiederverwendeten Sessions.

### Security / Reliability
- Connection-String-Validierung und Credential-Masking beibehalten/ausbauen.
- Keine stillen Fallbacks bei nicht unterstützten Engine-Features.
- Für neue Features explizite Failure-Modes und Recovery-Strategien dokumentieren.
