> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

# AI Module — Public Headers

**Version:** 0.1.0
**Status:** 🟡 Phase-1 implementation
**Module Path:** `include/ai/`

---

## Purpose

This directory defines the public API for AI-assisted plugin generation.
Consumers should include headers from `include/ai/` and avoid direct coupling
to internals in `src/ai/`.

## Key Header

| Header | Role |
|---|---|
| `ai_plugin_generator.h` | Public API for prompt definition, generation request handling, and structured result contracts |

## Public API Overview

### Core Types
- `LLMModel` — model selector (`CODE_LLAMA`, `CODEX`, `STARCODER`, `GITHUB_COPILOT`, `CUSTOM`)
- `SecurityLevel` — security strictness level (`LOW`..`PARANOID`)
- `PluginGenerationPrompt` — caller input payload
- `GeneratedPlugin` — generated code bundle + metadata (reserved for Phase 2 output)

### Entry Point
- `AIPluginGenerator`
  - `Result<void> validatePrompt(const PluginGenerationPrompt&)`
  - `Result<GeneratedPlugin> generatePlugin(const PluginGenerationPrompt&)`

## Configuration Options (`AIPluginGenerator::Config`)

| Option | Default | Description |
|---|---|---|
| `llm_endpoint` | `http://localhost:8080` | Ziel-Endpunkt für zukünftige LLM-Anbindung |
| `sandbox_dir` | `/tmp/themis_plugin_sandbox` | Verzeichnis für zukünftige Sandbox-/Build-Schritte |
| `output_dir` | `./generated_plugins` | Ausgabeziel für generierte Artefakte |

## Runtime Contract and Error Behavior

- Alle Aufrufe verwenden `Result<T>` statt Exceptions im API-Vertrag
- `validatePrompt` prüft Mindestanforderungen des Inputs (leer/zu lang)
- `generatePlugin` validiert zuerst und gibt aktuell für valide Inputs einen strukturierten Phase-1-Fehler zurück
- `GeneratedPlugin` ist für den zukünftigen Live-Generierungspfad vorgesehen

## Usage Snippet

```cpp
#include "ai/ai_plugin_generator.h"

AIPluginGenerator::Config cfg;
cfg.llm_endpoint = "http://localhost:8080";

AIPluginGenerator generator(cfg);

PluginGenerationPrompt prompt;
prompt.description = "Generate a secure plugin skeleton.";
prompt.type = themis::plugins::PluginType::BLOB_STORAGE;

auto check = generator.validatePrompt(prompt);
if (!check) {
    std::cerr << check.error().message() << '\n';
}
```

## Installation

This module is included in ThemisDB builds and exposed through the global include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Troubleshooting

- **Compile error: missing `PluginType` symbols**
  Ensure plugin headers are on include path (`plugins/plugin_interface.h` is required transitively).

- **`generatePlugin(...)` always returns an error**
  Expected in current phase: endpoint invocation is not implemented yet.

- **Unexpected validation failure**
  Verify `description` is set and length is within 8192 characters.

## See Also

- [Implementation Docs (`src/ai/README.md`)](../../src/ai/README.md)
- [Root Roadmap (`ROADMAP.md`)](../../ROADMAP.md)
- [Root Future Enhancements (`FUTURE_ENHANCEMENTS.md`)](../../FUTURE_ENHANCEMENTS.md)
- [Plugins Module Roadmap (`src/plugins/ROADMAP.md`)](../../src/plugins/ROADMAP.md)
- [Plugins Module Future Enhancements (`src/plugins/FUTURE_ENHANCEMENTS.md`)](../../src/plugins/FUTURE_ENHANCEMENTS.md)
- [Sekundärdokumentation (`docs/de/plugins/README.md`)](../../docs/de/plugins/README.md)
