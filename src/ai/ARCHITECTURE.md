# Architecture - AI Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The AI module currently centers on `AIPluginGenerator` as a focused service for plugin code generation.
It follows a validation-first and fail-closed pipeline.
Wave C C1/C2 safety-gate and telemetry integrations also extend into production `LLMAQLHandler` execution paths (`executeInfer`, `executeInferStreaming`, `executeRAG`, `executeChat`).

## Main Flow

1. `generatePlugin(prompt)` receives the request.
2. `validatePrompt(prompt)` enforces local constraints.
3. Request payload is serialized to JSON.
4. Endpoint invocation uses configured transport (`endpoint_invoke_fn`) or curl fallback.
5. Response JSON is parsed and mapped into `GeneratedPlugin`.
6. Optional sandbox gating materializes a per-run artifact bundle into `sandbox_dir`, verifies round-trip readability, copies the bundle into `output_dir`, and then invokes any configured callback policy.
7. Mandatory fields and optional policy gates are checked before success is returned.

## Core Contracts

| Contract | Behavior |
|---|---|
| `AIPluginGenerator::Config` | defines endpoint URL, timeout, sandbox/output artifact paths, and optional transport/policy hooks |
| `validatePrompt` | local guardrail against malformed/oversized request text |
| `generatePlugin` | end-to-end orchestration and error normalization |
| `GeneratedPlugin` mapping | converts endpoint payload into typed plugin artifacts |

## Dependency Direction

- ai -> utils (`expected`, error registry)
- ai -> curl/json/spdlog for transport, parsing, and diagnostics
- plugin consumers -> ai public API
- aql runtime (`LLMAQLHandler`) -> optional Wave C C1/C2 callbacks with fail-closed enforcement when enabled

## Failure Semantics

- Empty endpoint, transport errors, and non-2xx HTTP codes fail with structured errors.
- Invalid JSON fails closed with parse-error context.
- Missing mandatory `implementation_code` fails closed.
- Validation errors are returned before any endpoint call.
- Sandbox materialization or callback verification failures reject the generated plugin fail-closed.

## Sourcecode Verification (Module: ai/architecture)

- Verified implementation:
  - src/ai/ai_plugin_generator.cpp
- Verified API surface:
  - include/ai/ai_plugin_generator.h
- Verified architecture claims:
  - validation-before-I/O
  - bounded timeout usage on endpoint calls
  - fail-closed response handling

## Planning Traceability

- Strategic Wave C roadmap tracking: `#5040`
- Upstream dependency planning issues: Wave A `#5038`, Wave B `#5039`
  - Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
  - dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
  - follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
