[docs](../../README.md) > [de](../README.md) > [toolbox](./index.md) > [reference](./MISSING_IMPLEMENTATIONS.md)
**Datum:** 2026-04-16
**Status:** review
**Primary (Quelle der Wahrheit):**
- `include/toolbox/ROADMAP.md`
- `include/toolbox/toolbox_builder.h`
- `include/toolbox/content_toolbox_bridge.h`
- `src/toolbox/toolbox_builder.cpp`
- `src/toolbox/content_toolbox_bridge.cpp`

**Bezug / Reference:**
- Issue: [MODULE] include_toolbox
- Kontext: Offene Implementierungslücken und Doku-Lücken des Moduls `include_toolbox` für Milestone `v1.8.0`.

---

# Missing Implementations Report — include_toolbox

| Gap | Impact | Evidence | Priorität | Folge-Issue |
|---|---|---|---|---|
| `IngestionToolboxMetrics` aus ROADMAP Phase 4 fehlt | Keine dedizierte Observability für Toolbox-Nutzung/Latenz | `include/toolbox/ROADMAP.md` (Phase 4 offen), kein Metrics-Artefakt in `include/toolbox/` oder `src/toolbox/` | Hoch | Neu anzulegen (Roadmap-Eintrag vorhanden) |
| `ToolboxBuilder::withGraphWriter(...)` wird im Build-Pfad nicht verdrahtet | Erwartete Sink-Anbindung wird nicht durch Builder bereitgestellt | API in `include/toolbox/toolbox_builder.h`; keine Verwendung von `impl_->graph_writer` in `src/toolbox/toolbox_builder.cpp` außer Accessor | Mittel | Neu anzulegen (Implementierungs-Lücke) |
| `ContentToolboxBridge`-Vektor-Sink bleibt effektiv inaktiv | Vektor-Sink schreibt nicht, obwohl API vorhanden ist | `src/toolbox/content_toolbox_bridge.cpp`: `out.vectors` wird nie befüllt; Schreibpfad prüft `!out.vectors.empty()` | Mittel | Neu anzulegen (Implementierungs-Lücke) |
| Primär-Dokumente `README.md`, `ARCHITECTURE.md`, `CHANGELOG.md`, `FUTURE_ENHANCEMENTS.md` fehlen für `include/toolbox`/`src/toolbox` | Begrenzte Nachvollziehbarkeit von Architekturentscheidungen und Änderungsverlauf | Verzeichnisabgleich `include/toolbox/` und `src/toolbox/` enthielt nur `ROADMAP.md` als Primär-Markdown; `FUTURE_ENHANCEMENTS.md` jetzt ergänzt | Niedrig → ✅ `FUTURE_ENHANCEMENTS.md` gelöst; `README.md`, `ARCHITECTURE.md`, `CHANGELOG.md` offen | Neu anzulegen (Doku-Konsolidierung) |

## Priorisierte Folgearbeiten

1. Phase-4-Metrics als produktionsfähige API/Implementierung ergänzen.
2. `ToolboxBuilder`-Graph-Sink-End-to-End verdrahten oder API-Vertrag präzisieren.
3. Vektorpfad in `ContentToolboxBridge` funktionsfähig machen (inkl. Tests).
4. Fehlende Primary-Dokumente für `toolbox`-Modul ergänzen und in `PRIMARY_SOURCES.md` sichtbar machen.
