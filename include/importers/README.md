# Importers Module Headers

This directory contains header files (.h, .hpp) for the importers module.

## Purpose

Public interfaces and declarations for importers functionality.

## Implementation

See `../../src/importers/` for the implementation code.

## Documentation

See `../../docs/src/importers/` for detailed module documentation.

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

Include the relevant headers from this module:

```cpp
#include "importers/module_header.h"
```

See [`ARCHITECTURE.md`](ARCHITECTURE.md) and [`ROADMAP.md`](ROADMAP.md) for details.
