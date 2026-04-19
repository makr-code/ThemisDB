> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Content Module Headers

This directory contains header files (.h, .hpp) for the content module.

## Purpose

Public interfaces and declarations for content functionality.

## Implementation

See `../../src/content/` for the implementation code.

## Documentation

See `../../docs/src/content/` for detailed module documentation.

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

Include the relevant headers from this module:

```cpp
#include "content/module_header.h"
```

See [`ARCHITECTURE.md`](ARCHITECTURE.md) and [`ROADMAP.md`](ROADMAP.md) for details.
