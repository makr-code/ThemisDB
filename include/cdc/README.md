# Cdc Module Headers
<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: ../../src/cdc/README.md · ../../src/cdc/ARCHITECTURE.md · ../../src/cdc/ROADMAP.md -->

This directory contains header files (.h, .hpp) for the cdc module.

## Purpose

Public interfaces and declarations for cdc functionality.

## Implementation

See `../../src/cdc/` for the implementation code.

## Documentation

See `../../src/cdc/README.md` for a module overview and `../../src/cdc/ARCHITECTURE.md` for
the detailed architecture guide.

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

Include the relevant headers from this module:

```cpp
#include "cdc/module_header.h"
```

See [`ARCHITECTURE.md`](ARCHITECTURE.md) and [`ROADMAP.md`](ROADMAP.md) for details.
