> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

# AI Module

**Version:** 0.1.0
**Status:** 🟡 Phase-1 implementation
**Module Path:** `src/ai/`
**Namespace:** `themis::plugins::ai`

---

## Module Purpose

The AI module contains the implementation for AI-assisted plugin generation.
Current scope is the production baseline for prompt validation and deterministic
error handling around generation requests.

## Subsystem Scope

**In scope:**
- Validation of `PluginGenerationPrompt` inputs
- Structured `Result<T>`-based error reporting
- `AIPluginGenerator` orchestration entry-point for future LLM-backed generation

**Out of scope (current phase):**
- Real network invocation of an LLM endpoint
- AST/sandbox/security pipeline execution for generated code
- Plugin build/sign/deploy workflow orchestration

## Relevant Components

| File | Role |
|---|---|
| `ai_plugin_generator.cpp` | Implements `AIPluginGenerator::validatePrompt` and `AIPluginGenerator::generatePlugin` |

## Runtime Behavior

### `validatePrompt(...)`
- Rejects empty descriptions
- Rejects descriptions longer than `8192` characters
- Returns `Result<void>` with structured errors

### `generatePlugin(...)`
- Calls `validatePrompt(...)` first
- Returns validation errors unchanged
- In Phase 1 always returns a structured "LLM endpoint not yet wired" error for valid prompts

## Error Cases and Limits

- Empty `description` → validation error
- Description length `> 8192` → validation error
- Any valid request currently returns a Phase-1 not-wired error (no generated plugin output yet)
- Endpoint value in config is included in the diagnostic message for easier debugging

## Usage Snippet

```cpp
#include "ai/ai_plugin_generator.h"

using namespace themis::plugins::ai;

AIPluginGenerator::Config cfg;
cfg.llm_endpoint = "http://localhost:8080";

AIPluginGenerator generator(cfg);

PluginGenerationPrompt prompt;
prompt.description = "Generate a blob storage plugin with audit logging.";
prompt.type = themis::plugins::PluginType::BLOB_STORAGE;

auto result = generator.generatePlugin(prompt);
if (!result) {
    // Phase 1 expected: LLM endpoint not yet wired
    std::cerr << result.error().message() << '\n';
}
```

## Installation

This module is built as part of ThemisDB. Use the root build flow:

```bash
cmake --preset linux-release
cmake --build --preset linux-release
```

## Troubleshooting

- **`prompt description must not be empty`**
  Ensure `PluginGenerationPrompt::description` is non-empty.

- **`prompt description exceeds 8192-character limit`**
  Shorten or split the request text before calling `generatePlugin`.

- **`LLM endpoint not yet wired (Phase 2...)`**
  This is expected in current delivery state; no outbound generation call is executed yet.

## See Also

- [Public Header Docs (`include/ai/README.md`)](../../include/ai/README.md)
- [Root Roadmap (`ROADMAP.md`)](../../ROADMAP.md)
- [Root Future Enhancements (`FUTURE_ENHANCEMENTS.md`)](../../FUTURE_ENHANCEMENTS.md)
- [Plugins Module Roadmap (`src/plugins/ROADMAP.md`)](../plugins/ROADMAP.md)
- [Plugins Module Future Enhancements (`src/plugins/FUTURE_ENHANCEMENTS.md`)](../plugins/FUTURE_ENHANCEMENTS.md)
- [Sekundärdokumentation (`docs/de/plugins/README.md`)](../../docs/de/plugins/README.md)
