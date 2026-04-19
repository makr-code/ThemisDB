> **Build:** `cmake --preset release && cmake --build build/release`

# Toolbox Module Headers

Public header files for the ThemisDB toolbox module.

## Purpose

Provides bridging and builder utilities for content ingestion pipelines and toolbox plugin integration.

## Headers

- `content_toolbox_bridge.h` — Bridge between content processors and toolbox plugins
- `ingestion_toolbox.h` — Ingestion pipeline toolbox interface
- `toolbox_builder.h` — Fluent builder for toolbox pipeline assembly

## Implementation

See `../../src/toolbox/` for the implementation code.

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
