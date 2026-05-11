> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release --target test_ai_plugin_generator_focused`

# AI Module - Public API

<!-- Status: current | validated: 2026-05-11 -->
<!-- Links: src/ai/README.md · src/ROADMAP.md · src/FUTURE_ENHANCEMENTS.md · src/plugins/ROADMAP.md · src/plugins/FUTURE_ENHANCEMENTS.md · docs/de/plugins/README.md -->

Dokumentation der öffentlichen Header in `include/ai/`.

## Module Purpose

Der Header-Bereich stellt den öffentlichen API-Vertrag für AI-gestützte Plugin-Generierung bereit. Kern ist `AIPluginGenerator` mit Konfiguration, Prompt-Modell und Ergebnisstruktur.

## Public API Entry Points

| Header | Rolle |
| --- | --- |
| `ai_plugin_generator.h` | Definiert `AIPluginGenerator`, `PluginGenerationPrompt`, `GeneratedPlugin`, `LLMModel`, `SecurityLevel` |

## API-Überblick

### `enum class LLMModel`

Modellauswahl für den Prompt (`CODE_LLAMA`, `CODEX`, `STARCODER`, `GITHUB_COPILOT`, `CUSTOM`).

### `enum class SecurityLevel`

Sicherheitsniveau für den gewünschten Generierungspfad (`LOW`, `MEDIUM`, `HIGH`, `PARANOID`).

### `struct PluginGenerationPrompt`

Wichtige Felder:

- `description` (Pflichtfeld, wird validiert)
- `type` (`PluginType`)
- `required_capabilities`, `dependencies`
- `llm_model`, `security_level`
- `generate_tests`, `generate_docs`

### `struct GeneratedPlugin`

Ausgabemodell für generierten Code (`header_code`, `implementation_code`, `test_code`, `cmake_code`) plus `manifest`, `build_dependencies`, Security-Statusfelder.

### `class AIPluginGenerator`

Öffentliche Methoden:

- `Result<void> validatePrompt(const PluginGenerationPrompt&)`
- `Result<GeneratedPlugin> generatePlugin(const PluginGenerationPrompt&)`

#### Konfigurationsoptionen (`AIPluginGenerator::Config`)

| Feld | Default | Bedeutung |
| --- | --- | --- |
| `llm_endpoint` | `http://localhost:8080` | Ziel-Endpunkt für geplante LLM-Integration |
| `sandbox_dir` | `/tmp/themis_plugin_sandbox` | Geplantes Sandbox-Arbeitsverzeichnis |
| `output_dir` | `./generated_plugins` | Geplantes Ausgabeziel für Artefakte |

## Laufzeitverhalten und Fehlerfälle

- `validatePrompt()` prüft aktuell nur `description` (leer / >8192 Zeichen).
- `generatePlugin()` validiert zuerst; bei Validierungsfehlern wird derselbe Fehler zurückgegeben.
- Bei gültigem Prompt liefert `generatePlugin()` derzeit einen strukturierten `ERR_PLUGIN_LOAD_FAILED`-Fehler zurück, da der LLM-Endpunkt noch nicht verdrahtet ist.

## Usage Snippet

```cpp
#include "ai/ai_plugin_generator.h"

using namespace themis::plugins::ai;

AIPluginGenerator gen(AIPluginGenerator::Config{});
PluginGenerationPrompt prompt;
prompt.description = "Generate an exporter plugin";
prompt.type = themis::plugins::PluginType::EXPORTER;

auto check = gen.validatePrompt(prompt);
if (!check) {
    std::cerr << check.error().message() << std::endl;
}
```

## Installation

Header werden über den regulären ThemisDB-Build bereitgestellt; keine zusätzliche Modulinstallation erforderlich.

## Troubleshooting

- **`ERR_PLUGIN_LOAD_FAILED` bei gültigem Prompt:** Aktuell erwartetes Verhalten (Phase-1, Endpunkt nicht verdrahtet).
- **Zu lange Beschreibung:** `description` auf maximal 8192 Zeichen begrenzen.

## Siehe auch

- Implementierung: [`../../src/ai/README.md`](../../src/ai/README.md)
- Modulübergreifende Roadmap: [`../../src/ROADMAP.md`](../../src/ROADMAP.md)
- Modulübergreifende Future Enhancements: [`../../src/FUTURE_ENHANCEMENTS.md`](../../src/FUTURE_ENHANCEMENTS.md)
- Plugin-Roadmap (AIPluginGenerator-Einträge): [`../../src/plugins/ROADMAP.md`](../../src/plugins/ROADMAP.md)
- Plugin-Future-Enhancements: [`../../src/plugins/FUTURE_ENHANCEMENTS.md`](../../src/plugins/FUTURE_ENHANCEMENTS.md)
- Sekundärdoku (DE): [`../../docs/de/plugins/README.md`](../../docs/de/plugins/README.md)
