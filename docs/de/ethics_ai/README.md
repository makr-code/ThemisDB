# Ethics-AI-Modul

<!-- Status: current | validated: 2026-05-12 -->
<!-- Links: PRIMARY_SOURCES.md · ../../../src/ethics_ai/ · ../../../include/ethics_ai/ -->

**Stand:** 12. Mai 2026
**Version:** aktuell
**Kategorie:** KI-Ethik / Governance
**Status:** 🟡 Beta

---

## Übersicht

Das Ethics-AI-Modul implementiert ethische Entscheidungsfindung auf Basis mehrerer
Philosophieschulen. Es kombiniert Debattenorchestrierung, Argument-Persistenz,
RAG-Kontextaufbau, Synthese und Qualitätsbewertung.

**Primärquellen:**
- [`src/ethics_ai/`](../../../src/ethics_ai/)
- [`include/ethics_ai/`](../../../include/ethics_ai/)

---

## Wichtige Entry-Points (Public API)

| Header | Relevanz |
|--------|----------|
| `include/ethics_ai/ethics_ai_plugin_interface.h` | Zentrale Plugin-API (`initializeDebate`, `makeDecision`, Persistenz, Metriken, Konfiguration) |
| `include/ethics_ai/ethics_ai_types.h` | Gemeinsame Datentypen für Argumente, Entscheidungen, Profile, RAG-Kontext, Status |
| `include/ethics_ai/*.h` (Router/Kompression/Validator) | Erweiterungen für große Mehrschulen-Debatten und Kontextbudget-Strategien |

---

## Hauptkomponenten (`src/ethics_ai`)

| Komponente | Source | Beschreibung |
|------------|--------|--------------|
| EthicsAIPlugin | `ethics_ai_plugin.cpp` | Plugin-Einstiegspunkt, Lifecycle, Verdrahtung der Kernkomponenten |
| EthicalDiscourseEngine | `discourse_engine.cpp` | Debatteninitialisierung, Rundenfortsetzung, Entscheidungssynthese |
| ArgumentStore | `argument_store.cpp` | Persistenz für Argumente, Entscheidungen, Chains, Profile |
| PhilosophyLoader | `philosophy_loader.cpp` | Laden/Validieren von Profilen aus YAML |
| RAGContextEngine | `rag_context_engine.cpp` | Kontextaufbau über retrieval-basierte Abfragen |
| EthicsEvaluator | `ethics_evaluator.cpp` | Mehrdimensionale Qualitätsbewertung und Metrikunterstützung |
| ChainVisualizer | `chain_visualizer.cpp` | DOT/Mermaid-Export von Argumentketten |

---

## Laufzeitverhalten, Fehlerfälle, Grenzen

### Laufzeitverhalten

- Initialisierung erzeugt Loader, Store, RAG-Engine, Discourse-Engine und Evaluator.
- `philosophy_dir` (falls gesetzt) lädt Profile direkt bei `initialize()`.
- API-Aufrufe vor erfolgreicher Initialisierung liefern `Status::Error("Plugin not initialized")`.
- Interne Metriken (Debatten/Entscheidungen/Argumente/Evaluationen) werden fortlaufend aktualisiert.

### Typische Fehlerfälle

- Ungültiger oder fehlender Profilpfad (`philosophy_dir`).
- YAML-Parsing-Fehler in Profilen.
- Unbekannte Philosophieschule bei Debatten-/Abfrageaufrufen.
- Leere/ungültige IDs bei Store-Operationen.
- Nicht gefundene Argumente/Entscheidungen/Ketten.

### Bekannte Grenzen

- Vollständige LLM-basierte Argumentgenerierung ist noch nicht Endzustand.
- Echte Embedding-basierte semantische Suche ist als Ausbaupunkt dokumentiert.
- Ergebnisqualität hängt von Breite und Qualität der geladenen Philosophieprofile ab.

---

## Installation

Das Modul ist Teil von ThemisDB und wird über den normalen Projekt-Build gebaut.
Für die öffentliche API genügt der Include-Pfad auf das Repository-`include/`.

---

## Usage (Beispiel)

```cpp
#include "ethics_ai/ethics_ai_plugin_interface.h"

using themis::plugins::ethics::IEthicsAIPlugin;
using themis::plugins::ethics::EthicalDecision;

IEthicsAIPlugin* plugin = /* via Plugin-Manager */ nullptr;

if (plugin && plugin->initialize(R"({"philosophy_dir":"plugins/ethics_ai/philosophies"})")) {
    auto result = plugin->makeDecision(
        "Soll ein autonomes Fahrzeug im Zweifel defensiv abbremsen?",
        {"utilitarianism", "kantian_ethics", "virtue_ethics"},
        "autonomous_systems",
        true
    );

    if (std::holds_alternative<EthicalDecision>(result)) {
        const auto& decision = std::get<EthicalDecision>(result);
        // decision.decision_text, decision.confidence, decision.consensus_level
    }
}
```

---

## Troubleshooting

- **`Plugin not initialized`**: Initialisierung prüfen, JSON-Konfiguration validieren.
- **Profil nicht gefunden**: Schul-ID gegen geladene Profile abgleichen.
- **YAML-Fehler**: Profil-Dateien auf Syntax/Schema prüfen.
- **Kaum RAG-Kontext**: Sicherstellen, dass der ArgumentStore mit relevanten Daten gefüllt ist.

---

## Relevante Querverweise

- Modul-README (Implementation): [`src/ethics_ai/README.md`](../../../src/ethics_ai/README.md)
- Header-README (Public API): [`include/ethics_ai/README.md`](../../../include/ethics_ai/README.md)
- Roadmap: [`src/ethics_ai/ROADMAP.md`](../../../src/ethics_ai/ROADMAP.md)
- Future Enhancements: [`src/ethics_ai/FUTURE_ENHANCEMENTS.md`](../../../src/ethics_ai/FUTURE_ENHANCEMENTS.md)
- Architektur: [`src/ethics_ai/ARCHITECTURE.md`](../../../src/ethics_ai/ARCHITECTURE.md)
- Security: [`src/ethics_ai/SECURITY.md`](../../../src/ethics_ai/SECURITY.md)
- Primärquellenindex: [`PRIMARY_SOURCES.md`](./PRIMARY_SOURCES.md)
