# Public Header Architecture (`include/`)

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Kontext

`include/` definiert die öffentlichen C++-Verträge von ThemisDB.
Die Implementierungen liegen in `src/`; diese Ebene beschreibt bewusst nur API-/ABI-relevante Schnittstellen.

## Verantwortlichkeiten

- stabile Header-Oberflächen für Consumer (`include/<modul>/...`)
- eindeutige Trennung zwischen öffentlichem Vertrag (`include/`) und Implementierungsdetails (`src/`)
- modulweise Navigation über Unterordner (`ai`, `api`, `query`, `storage`, `llm`, `graph`, `rag`, ...)

## Architektur-Surfaces

| Surface | Ort |
|---|---|
| Öffentliche Modulverträge | `include/<module>/` |
| Feature-nahe Unterflächen | z. B. `include/llm/attention/`, `include/server/rpc/`, `include/query/functions/` |
| Header-Bezug in Build/Test | `tests/CMakeLists.txt` (`include_directories(... include ...)`) |
| Packaging-Ziel | Root-`CMakeLists.txt` (`include/` als SDK-Header-Ziel im Runtime-Layout) |

## Beziehung zur strategischen Zielarchitektur

Das strategische Ziel aus `../FUTURE_PLAN.md` (ANN Frontdoor → Tensor Mid-Layer → Graph Truth Layer → LLM/LoRA Final Layer) wird hier als **Vertragsgrenze** abgebildet:

- ANN-/Search-nahe Verträge: `include/index/`, `include/search/`
- Tensor-/Training-/Inference-nahe Verträge: `include/tensor/`, `include/training/`, `include/llm/`
- Graph-/Query-/Truth-nahe Verträge: `include/graph/`, `include/query/`
- Final-Layer-/Orchestrierungsverträge: `include/llm/`, `include/rag/`, `include/ai/`

Diese Zuordnung dokumentiert Schichten und Verantwortungen, ohne eine bereits vollständige Endzustands-Implementierung zu behaupten.

## Nicht-Ziele

- keine Spezifikation privater Implementierungsdetails aus `src/`
- keine Duplizierung von Modul-Roadmaps
- keine Annahme, dass alle strategischen Layer bereits vollständig verdrahtet sind

## Sourcecode Verification (Scope: include/<root>)

- Verifiziert:
  - `include/README.md`
  - `tests/CMakeLists.txt` (Header-Einbindung)
  - `CMakeLists.txt` (Kommentar zu `include/` als SDK-Header-Ziel)
  - Verzeichnisstruktur unter `include/` (modulweise Header-Verträge)
