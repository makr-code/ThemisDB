> **Build:** `cmake --preset release && cmake --build build/release`

# include stable_diffusion module

Public headers for Stable Diffusion plugin integration.

## Headers
- `sd_config.h`
- `sd_generator.h`
- `sd_plugin.h`
- `sd_plugin_registrar.h`
- `sd_prompt_sanitizer.h`

## Exposed API
- `SDPlugin` implementing `IImageGenerationBackend`
- Generator abstraction (`ISDGenerator`) with stub/in-memory implementations
- Prompt policy sanitizer and JSON config contract

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

Include the relevant headers from this module:

```cpp
#include "stable_diffusion/module_header.h"
```

See the [module documentation](../../docs/src/stable_diffusion/) for details.
