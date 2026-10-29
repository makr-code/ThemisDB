# AI Module

<!-- Status: current | validated: 2026-10-29 -->
<!-- Links: ../../../src/ai/README.md · ../../../include/ai/README.md · ../../../src/ai/ROADMAP.md -->

## Overview

The `ai` module provides the AI-assisted plugin generation workflow used by ThemisDB. It validates prompt inputs, invokes tool or model endpoints, parses results, and maps generated payloads into the plugin artifact model.

## Primary sources

- `src/ai/` — implementation of generation and validation flows
- `include/ai/` — public API contract for AI plugin generation
- `src/ai/ROADMAP.md` — active delivery and release status
- `src/ai/FUTURE_ENHANCEMENTS.md` — planned enhancements

## Key responsibilities

- prompt validation and safety checks
- endpoint invocation with bounded retries and timeout handling
- structured response parsing and error mapping
- sandboxed artifact materialization and audit output handling

## Related documentation

- [src/ai/README.md](../../../src/ai/README.md)
- [src/ai/ARCHITECTURE.md](../../../src/ai/ARCHITECTURE.md)
- [include/ai/README.md](../../../include/ai/README.md)

## Validation

This landing page keeps the German module guide aligned with the implemented AI generator API and runtime behavior tracked in the source module.
