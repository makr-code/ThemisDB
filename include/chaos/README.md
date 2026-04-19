> **Build:** `cmake --preset release && cmake --build build/release`

# include chaos module

Public header surface for chaos engineering fault injection.

## Header
- `include/chaos/chaos_framework.h`

## Exposed API
- Fault model types (`FaultType`, `FaultSpec`, `ActiveFault`)
- `FaultInjector`
- `ChaosScheduler`

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

Include the relevant headers from this module:

```cpp
#include "chaos/module_header.h"
```

See [`ARCHITECTURE.md`](ARCHITECTURE.md) and [`ROADMAP.md`](ROADMAP.md) for details.
