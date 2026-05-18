> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release --target test_ai_plugin_generator_focused`

# AI Module

<!-- Status: current | validated: 2026-05-11 -->
<!-- Links: ROADMAP.md · FUTURE_ENHANCEMENTS.md · include/ai/README.md · src/plugins/ROADMAP.md · src/plugins/FUTURE_ENHANCEMENTS.md · docs/de/plugins/README.md -->

Dokumentation der produktiven Implementierung in `src/ai/`.

## Module Purpose

Das Modul stellt eine minimale, produktive Basis für AI-gestützte Plugin-Generierung bereit (`AIPluginGenerator`), inklusive Prompt-Validierung und strukturiertem Fehlerverhalten für den aktuell noch nicht verdrahteten LLM-Aufrufpfad.

## Subsystem Scope

**In scope:** Konstruktion des Generators, Prompt-Validierung, Fehlerweitergabe in `generatePlugin()`, Debug-Logging über `spdlog`.

**Out of scope:** Echte HTTP/LLM-Integration, JSON-Response-Parsing, Security-Sandbox-Buildpipeline (Phase-2-Arbeiten).

## Relevant Implementation Components

| Datei / Komponente | Rolle |
| --- | --- |
| `ai_plugin_generator.cpp` | Implementiert `AIPluginGenerator::{validatePrompt, generatePlugin}` |
| `AIPluginGenerator::validatePrompt` | Prüft `description` auf Leerwert und 8192-Zeichen-Limit |
| `AIPluginGenerator::generatePlugin` | Führt Validierung aus und liefert derzeit einen strukturierten Phase-1-Fehler zurück |

## Runtime Behavior, Fehlerfälle und Grenzen

- `validatePrompt()` liefert Fehler (`ERR_PLUGIN_LOAD_FAILED`), wenn:
  - `prompt.description` leer ist.
  - `prompt.description` länger als 8192 Zeichen ist.
- `generatePlugin()` ruft zuerst `validatePrompt()` auf und propagiert dessen Fehler unverändert.
- Bei gültigem Prompt wird aktuell **kein** LLM-Endpunkt aufgerufen; stattdessen wird ein strukturierter Fehler mit Endpunkt-Kontext zurückgegeben (`LLM endpoint not yet wired`).
- Limit: Es wird aktuell kein `GeneratedPlugin` zurückgegeben (Phase-1-Implementierung).

## Usage Snippet

```cpp
#include "ai/ai_plugin_generator.h"

using namespace themis::plugins::ai;

AIPluginGenerator::Config cfg;
cfg.llm_endpoint = "http://localhost:8080";

AIPluginGenerator generator(cfg);

PluginGenerationPrompt prompt;
prompt.description = "Generate a blob storage plugin";
prompt.type = themis::plugins::PluginType::BLOB_STORAGE;

auto result = generator.generatePlugin(prompt);
if (!result) {
    // Phase-1 expected path: structured error
    std::cerr << result.error().message() << std::endl;
}
```

## Installation

Das Modul wird über den normalen ThemisDB-Build eingebunden; es ist keine separate Installation nötig.

## Troubleshooting

- **Fehler: `prompt description must not be empty`**
  `PluginGenerationPrompt::description` vor Aufruf von `generatePlugin()` befüllen.
- **Fehler: `prompt description exceeds 8192-character limit`**
  Prompt kürzen oder in mehrere Teilprompts splitten.
- **Fehler: `LLM endpoint not yet wired`**
  Erwartetes Verhalten der aktuellen Phase; siehe Roadmap/Future-Enhancements für den geplanten Integrationspfad.

## Verifikation / Tests

- Fokus-Testziel: `AIPluginGeneratorFocusedTests` (`tests/test_ai_plugin_generator.cpp`, APG-01..06).

## Siehe auch

- Public API: [`../../include/ai/README.md`](../../include/ai/README.md)
- AI-Roadmap: [`ROADMAP.md`](./ROADMAP.md)
- AI-Future-Enhancements: [`FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md)
- Architektur: [`ARCHITECTURE.md`](./ARCHITECTURE.md)
- Audit: [`AUDIT.md`](./AUDIT.md)
- Security: [`SECURITY.md`](./SECURITY.md)
- Changelog: [`CHANGELOG.md`](./CHANGELOG.md)
- Plugin-Roadmap (AIPluginGenerator-Einträge): [`../plugins/ROADMAP.md`](../plugins/ROADMAP.md)
- Plugin-Future-Enhancements: [`../plugins/FUTURE_ENHANCEMENTS.md`](../plugins/FUTURE_ENHANCEMENTS.md)
- Modulübergreifende Roadmap: [`../ROADMAP.md`](../ROADMAP.md)
- Modulübergreifende Future Enhancements: [`../FUTURE_ENHANCEMENTS.md`](../FUTURE_ENHANCEMENTS.md)
- Sekundärdoku (DE): [`../../docs/de/plugins/README.md`](../../docs/de/plugins/README.md)
