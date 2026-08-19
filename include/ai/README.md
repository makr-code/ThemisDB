> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release --target test_ai_plugin_generator_focused`

# AI Module - Public API

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: src/ai/README.md · src/ai/ROADMAP.md · src/ai/FUTURE_ENHANCEMENTS.md · src/plugins/ROADMAP.md · src/plugins/FUTURE_ENHANCEMENTS.md · docs/de/plugins/README.md -->

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
| `llm_endpoint` | `http://localhost:8080` | Ziel-Endpunkt für die Runtime-Generierung; wird optional durch `endpoint_invoke_fn` oder `setLlmHttpPostFn()` übersteuert |
| `allowed_llm_endpoints` | leer | Optionale Allow-List für `llm_endpoint`; bei gesetzter Liste werden nur explizit erlaubte Endpunkte akzeptiert |
| `sandbox_dir` | `/tmp/themis_plugin_sandbox` | Arbeitsverzeichnis für Sandbox-Artefakte; pro Lauf wird ein Bundle mit Quellcode und Manifest materialisiert |
| `output_dir` | `./generated_plugins` | Audit-/Diagnose-Ausgabeziel; erhält eine persistierte Kopie jedes Sandbox-Bundles |
| `max_request_body_bytes` | `262144` | Hartes Limit für serialisierte Request-Größe |
| `max_response_body_bytes` | `8388608` | Hartes Limit für Endpoint-Response-Größe |

## Laufzeitverhalten und Fehlerfälle

- `validatePrompt()` prüft `description` (leer / >8192 Zeichen) sowie strukturierte Felder (`required_capabilities`, `dependencies`) auf Limits, Token-Format und Duplikate.
- `generatePlugin()` validiert zuerst; bei Validierungsfehlern wird derselbe Fehler zurückgegeben.
- `generatePlugin()` erzwingt zusätzlich Request-/Response-Größenlimits und optionales Endpoint-Allow-Listing fail-closed.
- Bei aktiviertem `enable_sandbox_gate` materialisiert `generatePlugin()` Header-, Implementierungs-, Test-, CMake- und Manifest-Artefakte in `sandbox_dir`, verifiziert den Schreib-/Lese-Roundtrip fail-closed und kopiert das Bundle nach `output_dir`.
- `sandbox_verify_fn` ist optional und wird erst nach erfolgreicher Artefakt-Materialisierung ausgeführt; ein Callback-Fehler lehnt das generierte Plugin fail-closed ab.

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

- **`ERR_PLUGIN_LOAD_FAILED` bei gültigem Prompt:** Prüfe Endpoint-Erreichbarkeit, Allow-List, Response-Format sowie aktivierte Sandbox-/C1-/C2-Gates.
- **Zu lange Beschreibung:** `description` auf maximal 8192 Zeichen begrenzen.

## Siehe auch

- Implementierung: [`../../src/ai/README.md`](../../src/ai/README.md)
- AI-Roadmap: [`../../src/ai/ROADMAP.md`](../../src/ai/ROADMAP.md)
- AI-Future-Enhancements: [`../../src/ai/FUTURE_ENHANCEMENTS.md`](../../src/ai/FUTURE_ENHANCEMENTS.md)
- Plugin-Roadmap (AIPluginGenerator-Einträge): [`../../src/plugins/ROADMAP.md`](../../src/plugins/ROADMAP.md)
- Plugin-Future-Enhancements: [`../../src/plugins/FUTURE_ENHANCEMENTS.md`](../../src/plugins/FUTURE_ENHANCEMENTS.md)
- Modulübergreifende Roadmap: [`../../src/ROADMAP.md`](../../src/ROADMAP.md)
- Modulübergreifende Future Enhancements: [`../../src/FUTURE_ENHANCEMENTS.md`](../../src/FUTURE_ENHANCEMENTS.md)
- Sekundärdoku (DE): [`../../docs/de/plugins/README.md`](../../docs/de/plugins/README.md)
