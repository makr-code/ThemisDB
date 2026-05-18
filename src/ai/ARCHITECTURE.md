> **Status:** 2026-05-13 — Architecture reflects actual source and headers.

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../include/ai/README.md -->

# Architecture — AI Module

## Overview

The `ai` module provides a minimal, production-ready foundation for AI-assisted
plugin generation in ThemisDB. It exposes a single public class, `AIPluginGenerator`,
and delegates LLM communication to a configurable endpoint (Phase 2 work).

## Module Position in ThemisDB

```
┌────────────────────────────────────────────────────────────┐
│ ThemisDB Plugin System (src/plugins/)                       │
│                                                            │
│  Plugin Lifecycle Manager                                  │
│   └── AIPluginGenerator  ◄── this module                  │
│         ├── validatePrompt()   input validation            │
│         └── generatePlugin()  LLM endpoint call (Phase 2) │
└────────────────────────────────────────────────────────────┘
              │
              ▼ (Phase 2)
┌────────────────────────────────────────────────────────────┐
│ External LLM Service                                        │
│  HTTP POST → config_.llm_endpoint                          │
│  Response: JSON-encoded GeneratedPlugin artefacts          │
└────────────────────────────────────────────────────────────┘
```

## Class Hierarchy

```
themis::plugins::ai
└── AIPluginGenerator             (include/ai/ai_plugin_generator.h)
    ├── Config
    │   ├── llm_endpoint          string — LLM service URL
    │   ├── sandbox_dir           string — output sandbox path
    │   └── output_dir            string — generated-code output path
    ├── validatePrompt(prompt)    → Result<void>
    └── generatePlugin(prompt)   → Result<GeneratedPlugin>
```

## Data Flow — generatePlugin()

```
generatePlugin(PluginGenerationPrompt)
        │
        ▼
validatePrompt()
  ├── description empty? → ERR_PLUGIN_LOAD_FAILED
  └── description > 8192 chars? → ERR_PLUGIN_LOAD_FAILED
        │ (valid)
        ▼
[Phase 1] Return structured error: "LLM endpoint not yet wired"
        │
        ▼ (Phase 2, Target v1.6.0)
HTTP POST → config_.llm_endpoint
        │ JSON response
        ▼
Parse → GeneratedPlugin { header_code, implementation_code, manifest }
        │
        ▼
Security sandbox pipeline (Phase 2)
        │
        ▼
Return Result<GeneratedPlugin>
```

## Key Design Decisions

1. **Fail-safe Phase-1 behaviour** — `generatePlugin()` returns a structured error
   for all valid prompts. This allows callers to be tested end-to-end without a
   live LLM service, while the API contract (`Result<GeneratedPlugin>`) is stable
   for Phase-2 wiring.

2. **Validation-before-I/O** — `validatePrompt()` is always called before any
   network activity. This ensures that malformed prompts are rejected locally
   without consuming LLM quota or causing timeouts.

3. **Config-driven endpoint** — the LLM endpoint is injected at construction time
   via `Config::llm_endpoint`, enabling test doubles and multi-tenant deployments
   without recompilation.

4. **Dependency isolation** — no HTTP/networking libraries are linked in Phase 1.
   The implementation depends only on `spdlog`, `utils/error_registry`, and
   `utils/expected`.

## Dependency Direction

```
ai/ → utils/         (error_registry, expected — permitted)
ai/ → spdlog         (logging — permitted)
ai/ → [external LLM] (Phase 2 only — HTTP client TBD)
plugins/ → ai/       (AIPluginGenerator used by plugin system — permitted)
```

## Interfaces

- **Public API:** `include/ai/ai_plugin_generator.h` — `AIPluginGenerator`, `Config`,
  `PluginGenerationPrompt`, `GeneratedPlugin`
- **Error codes:** `utils/error_registry` — `ERR_PLUGIN_LOAD_FAILED`
- **Result type:** `utils/expected` — `Result<T>` / `tl::expected<T, Error>`

## See Also

- Implementation overview: [`README.md`](./README.md)
- Public API: [`../../include/ai/README.md`](../../include/ai/README.md)
- Roadmap: [`ROADMAP.md`](./ROADMAP.md)
- Future Enhancements: [`FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md)
- Security: [`SECURITY.md`](./SECURITY.md)
