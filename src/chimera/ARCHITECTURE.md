> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# chimera architecture

## Scope

Dieses Dokument beschreibt den Realstand des Moduls `chimera` in `src/chimera/` und `include/chimera/`.

## Current Runtime Architecture

### Implementierte Komponenten

- `src/chimera/themisdb_adapter.cpp`
  - Referenzadapter `chimera::ThemisDBAdapter`
  - In-Memory-Simulation für Relational/Vector/Graph/Document
  - Optionaler Engine-Dispatch über injizierte `themis::QueryEngine`, `VectorIndexManager`, `GraphIndexManager`
  - Async-Operationen, Streaming-Resultate, Prepared Statements
- `include/chimera/themisdb_adapter.hpp`
  - Public Header für `ThemisDBAdapter`, `ThemisDBResultStream`, `ThemisDBPreparedStatement`
- `include/chimera/database_adapter.hpp`
  - CHIMERA-Basisinterfaces (inkl. Erweiterungen für Streaming/Prepared Statements)

### Datenfluss (vereinfacht)

1. `connect()` validiert Connection-String und setzt Verbindungsstatus.
2. Aufrufe wie `execute_query`, `search_vectors`, `traverse` prüfen Verbindungsstatus.
3. Falls Engine-Pointer gesetzt:
   - Dispatch in ThemisDB-Enginepfade (falls `THEMISDB_ENGINE_AVAILABLE` kompiliert ist).
4. Sonst:
   - In-Memory-Simulationspfade auf `table_store_`, `vector_store_`, `graph_nodes_`, `doc_store_`.
5. Optional:
   - Streaming via `execute_query_stream`.
   - Prepared Statements via `prepare`, `bind`, `execute`, `list_prepared`.

## Verifizierte Grenzen

- Es existiert **nur** der ThemisDB-Referenzadapter im Modulpfad `src/chimera/`.
- Adapter-Factory- und weitere Vendor-Adapter-Dateien sind im Modulpfad aktuell nicht vorhanden.
- Mehrere Engine-Dispatch-Pfade liefern `ErrorCode::NOT_IMPLEMENTED`, wenn `THEMISDB_ENGINE_AVAILABLE` nicht definiert ist.
- Include-Dokumentation (`include/chimera/README.md`) ist vorhanden.

## Testabdeckung (direkt chimera-bezogen)

- `tests/chimera/test_chimera_streaming.cpp`
- `tests/chimera/test_chimera_prepared_statements.cpp`

Diese Tests validieren primär Streaming-/Prepared-Statement-Verhalten im Simulationsmodus.
