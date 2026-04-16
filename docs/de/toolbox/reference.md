[docs](../../README.md) > [de](../README.md) > [toolbox](./index.md) > [reference](./reference.md)
**Datum:** 2026-04-16
**Status:** review
**Primary (Quelle der Wahrheit):**
- `include/toolbox/ROADMAP.md`
- `include/toolbox/ingestion_toolbox.h`
- `include/toolbox/toolbox_builder.h`
- `include/toolbox/content_toolbox_bridge.h`
- `src/toolbox/ingestion_toolbox.cpp`
- `src/toolbox/toolbox_builder.cpp`
- `src/toolbox/content_toolbox_bridge.cpp`
- `tests/test_toolbox_ingestion.cpp`
- `tests/test_content_toolbox_bridge.cpp`
- `tests/test_rag_ingestion_bridge.cpp`

**Bezug / Reference:**
- Issue: [MODULE] include_toolbox
- Kontext: Reality-Check, Roadmap-Verifikation und Research-Hinweise für die Doku-Migration des Toolbox-Moduls.

---

# Toolbox — Reality-Check & Verifikation (DE)

## Task 1 — Reality-Check gegen Sourcecode

| Bereich | Dokumentierter Stand | Verifizierter Code-Stand | Bewertung |
|---|---|---|---|
| Core Toolbox (`IngestionToolbox`) | Als produktionsreif und DI-fähig dokumentiert | `createDefault()`, DI-Setter, `extractEntities()` und Mutex-Schutz vorhanden (`include/toolbox/ingestion_toolbox.h`, `src/toolbox/ingestion_toolbox.cpp`) | ✅ konsistent |
| Builder (`ToolboxBuilder`) | In ROADMAP teils noch als geplant geführt | Implementiert inkl. Tests (`include/toolbox/toolbox_builder.h`, `src/toolbox/toolbox_builder.cpp`, `tests/test_content_toolbox_bridge.cpp`) | ⚠️ ROADMAP-Status war inkonsistent (in Primary korrigiert) |
| Content-Bridge | Sink-Unterstützung als Teil der Bridge beschrieben | `graph_writer` wird genutzt; `vector_writer` bleibt ohne Datenpfad, da `out.vectors` im aktuellen Flow nicht befüllt wird (`src/toolbox/content_toolbox_bridge.cpp`) | ⚠️ funktionale Lücke dokumentiert |

## Task 2 — ROADMAP/FUTURE_ENHANCEMENTS-Verifikation

### ROADMAP-Phasenabgleich

| Phase | ROADMAP-Status | Code-Evidence | Ergebnis |
|---|---|---|---|
| Phase 1: Core Toolbox | Completed | `include/toolbox/ingestion_toolbox.h`, `src/toolbox/ingestion_toolbox.cpp` | ✅ bestätigt |
| Phase 2: AQL Bridge | Completed | `include/aql/aql_ingestion_bridge.h`, `src/aql/aql_ingestion_bridge.cpp`, `tests/test_toolbox_ingestion.cpp` | ✅ bestätigt |
| Phase 3: Builder & RAG Bridge | Completed | `include/toolbox/toolbox_builder.h`, `src/toolbox/toolbox_builder.cpp`, `include/rag/rag_ingestion_bridge.h`, `src/rag/rag_ingestion_bridge.cpp`, `tests/test_rag_ingestion_bridge.cpp` | ✅ bestätigt |
| Phase 4: Observability | Planned | Keine `IngestionToolboxMetrics`-Implementierung in `include/toolbox/` oder `src/toolbox/` | ❗ offen |

### FUTURE_ENHANCEMENTS-Verifikation

- `include/toolbox/FUTURE_ENHANCEMENTS.md` ist aktuell **nicht vorhanden**.
- `src/toolbox/FUTURE_ENHANCEMENTS.md` ist aktuell **nicht vorhanden**.
- Ergebnis: Future-Enhancement-Anforderungen für `include_toolbox` sind derzeit nur indirekt über `include/toolbox/ROADMAP.md` dokumentiert.

## Task 3 — Research-Hinweise / Constraints

- Reihenfolgeabhängigkeit im Builder: Bei `withWorkflowEngine(...)` wird ein externer Engine-Stand übernommen; die tatsächliche Step-Registrierung hängt daher vom injizierten Engine-Zustand plus `withFormatExtractor*()` ab (`src/toolbox/toolbox_builder.cpp`).
- `extractEntities()` ist bewusst fehlertolerant und liefert bei Workflow-Fehlern ein leeres Ergebnis statt Fehlerpropagierung (`src/toolbox/ingestion_toolbox.cpp`), was in abhängigen Modulen berücksichtigt werden muss.
- Für `ContentToolboxBridge` bleibt Vektor-Sink-Verhalten aktuell effektiv inaktiv, solange kein VectorRecord-Pfad gesetzt wird (`src/toolbox/content_toolbox_bridge.cpp`).
