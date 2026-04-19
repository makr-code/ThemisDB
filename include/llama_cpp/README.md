> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# include llama_cpp module

Public header for llama.cpp plugin integration.

## Header
- `include/llama_cpp/llama_cpp_plugin.h`

## Exposed API
- `LlamaCppPlugin` implementing `llm::ILLMPlugin`
- Model load/unload, generation, RAG generation, embeddings, LoRA lifecycle, stats

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

Include the relevant headers from this module:

```cpp
#include "llama_cpp/module_header.h"
```

See [`ARCHITECTURE.md`](ARCHITECTURE.md) and [`ROADMAP.md`](ROADMAP.md) for details.
