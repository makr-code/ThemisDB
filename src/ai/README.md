# ThemisDB AI Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The AI module provides plugin code generation via `AIPluginGenerator`.
It validates prompts, calls a configured endpoint, parses JSON responses, and maps results to `GeneratedPlugin`.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| ai_plugin_generator.cpp | runtime flow for validation, endpoint invocation, response parsing |
| include/ai/ai_plugin_generator.h | public API and configuration contract |
| AIPluginGenerator::validatePrompt | enforces required prompt constraints |
| AIPluginGenerator::generatePlugin | orchestrates request and response mapping |

## Scope

In scope:
- prompt validation for plugin generation requests
- endpoint invocation with bounded timeouts
- structured error handling for transport, HTTP, and JSON failures
- response mapping to `GeneratedPlugin` plus manifest defaults

Out of scope:
- quality of endpoint model output beyond basic structural checks
- deployment-specific external sandbox engines beyond the built-in artifact materialization and callback gate
- higher-level plugin lifecycle orchestration outside the generator contract

## Runtime Behavior and Limits

- `validatePrompt` rejects empty descriptions and descriptions longer than 8192 characters.
- `generatePlugin` always validates input before I/O.
- Endpoint invocation uses either `Config::endpoint_invoke_fn` or built-in curl path.
- Non-2xx responses and invalid JSON return structured `ERR_PLUGIN_LOAD_FAILED` errors.
- Responses must include non-empty `implementation_code`; otherwise generation fails.
- Debug logging truncates prompt content to a bounded prefix.
- Optional Wave C runtime hooks can be enabled in `AIPluginGenerator::Config`:
  - C1 safety gate (`enable_c1_cai_safety_gate`) enforces a minimum CAI safety score via callback.
  - Sandbox gate (`enable_sandbox_gate`) materializes generated source bundles into `sandbox_dir`, verifies read/write integrity, persists an audit copy into `output_dir`, and optionally invokes `sandbox_verify_fn`.
  - C2 telemetry forwarding (`enable_c2_federated_telemetry`) forwards local generation metrics via callback.
- The same Wave C C1/C2 hook model is also available in production `LLMAQLHandler` runtime paths (`executeInfer`, `executeInferStreaming`, `executeRAG`, `executeChat`) with fail-closed behavior when enabled callbacks are missing or fail.

## Sourcecode Verification (Module: ai/readme)

- Verified files:
  - src/ai/ai_plugin_generator.cpp
  - include/ai/ai_plugin_generator.h
- Verified behavior surfaces:
  - validation-first execution path
  - endpoint call path with timeout and HTTP code checks
  - fail-closed JSON parsing and mandatory implementation payload check
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - strategic Wave C planning is tracked in issue `#5040` with dependency links to Wave A `#5038` and Wave B `#5039`
  - Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
  - dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
  - follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
  - historical completion remains in CHANGELOG.md