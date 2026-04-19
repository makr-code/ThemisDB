# Document Module Headers

This directory contains header files (.h, .hpp) for the document module.

## Purpose

Public interfaces and declarations for document functionality.

**Note:** The DocumentManager has been moved to the `projects` module. See `../projects/DocumentManager/` for the new location.

For backward compatibility, a deprecated forwarding header is available at `document_manager_deprecated.h`.

## Implementation

See `../../src/document/` for the implementation code.

## Documentation

See `../../docs/src/document/` for detailed module documentation.

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

Include the relevant headers from this module:

```cpp
#include "document/module_header.h"
```

See [`ARCHITECTURE.md`](ARCHITECTURE.md) and [`ROADMAP.md`](ROADMAP.md) for details.
