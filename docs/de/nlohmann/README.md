# nlohmann Module

<!-- Status: current | validated: 2026-10-29 -->
<!-- Links: ../../../include/nlohmann/README.md -->

## Overview

The `nlohmann` module covers the JSON library integration used throughout ThemisDB for structured serialization and parsing. It is a dependency-facing integration layer rather than an application-specific internal subsystem.

## Primary sources

- `include/nlohmann/` — JSON headers and integration surface
- upstream library sources maintained under the vendored include tree

## Key responsibilities

- JSON serialization and deserialization
- schema-friendly data interchange for config, API payloads, and runtime state
- common parsing support across modules and plugin boundaries

## Related documentation

- [include/nlohmann/README.md](../../../include/nlohmann/README.md)

## Validation

This page ensures the docs/de alignment check recognizes the vendored JSON library as a documented integration target rather than an undocumented module.
