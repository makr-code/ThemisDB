> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Cache Module Headers

This directory contains header files (.h, .hpp) for the cache module.

## Purpose

Public interfaces and declarations for cache functionality.

## Implementation

See `../../src/cache/` for the implementation code.

## Documentation

See `../../src/cache/README.md` for detailed module documentation.
For secondary (German-language) docs see `../../docs/de/src/cache/`.

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

Include the relevant headers from this module:

```cpp
#include "cache/module_header.h"
```

See [`ARCHITECTURE.md`](ARCHITECTURE.md) and [`ROADMAP.md`](ROADMAP.md) for details.
