# Sharding Module Headers

This directory contains header files (.h, .hpp) for the sharding module.

## Purpose

Public interfaces and declarations for sharding functionality.

## Implementation

See `../../src/sharding/` for the implementation code.

## Documentation

See `../../docs/src/sharding/` for detailed module documentation.

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

Include the relevant headers from this module:

```cpp
#include "sharding/module_header.h"
```

See [`ARCHITECTURE.md`](ARCHITECTURE.md) and [`ROADMAP.md`](ROADMAP.md) for details.
