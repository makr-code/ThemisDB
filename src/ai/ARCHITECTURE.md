# Architecture - AI Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The AI module currently centers on `AIPluginGenerator` as a focused service for plugin code generation.
It follows a validation-first and fail-closed pipeline.

## Main Flow

1. `generatePlugin(prompt)` receives the request.
2. `validatePrompt(prompt)` enforces local constraints.
3. Request payload is serialized to JSON.
4. Endpoint invocation uses configured transport (`endpoint_invoke_fn`) or curl fallback.
5. Response JSON is parsed and mapped into `GeneratedPlugin`.
6. Mandatory fields are checked before success is returned.

## Core Contracts

| Contract | Behavior |
|---|---|
| `AIPluginGenerator::Config` | defines endpoint URL, timeout, paths, and optional custom invoker |
| `validatePrompt` | local guardrail against malformed/oversized request text |
| `generatePlugin` | end-to-end orchestration and error normalization |
| `GeneratedPlugin` mapping | converts endpoint payload into typed plugin artifacts |

## Dependency Direction

- ai -> utils (`expected`, error registry)
- ai -> curl/json/spdlog for transport, parsing, and diagnostics
- plugin consumers -> ai public API

## Failure Semantics

- Empty endpoint, transport errors, and non-2xx HTTP codes fail with structured errors.
- Invalid JSON fails closed with parse-error context.
- Missing mandatory `implementation_code` fails closed.
- Validation errors are returned before any endpoint call.

## Sourcecode Verification (Module: ai/architecture)

- Verified implementation:
  - src/ai/ai_plugin_generator.cpp
- Verified API surface:
  - include/ai/ai_plugin_generator.h
- Verified architecture claims:
  - validation-before-I/O
  - bounded timeout usage on endpoint calls
  - fail-closed response handling
  - Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
  - dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
  - follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`