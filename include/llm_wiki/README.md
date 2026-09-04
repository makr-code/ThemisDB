> **Build:** `cmake --preset release && cmake --build build/release`

# LLM Wiki Module — Public Headers

**Module Path:** `include/llm_wiki/`
**Implementation:** `../../src/llm_wiki/`

## Purpose

Public interfaces and declarations for ThemisDB's LLM-powered knowledge base and wiki integration subsystem, providing unified knowledge retrieval and semantic search capabilities.

## Canonical Module Documentation

`include/llm_wiki/` contains public header contracts. Canonical module behavior, architecture, and operations docs live in `src/llm_wiki/`:

- [`../../src/llm_wiki/README.md`](../../src/llm_wiki/README.md)
- [`../../src/llm_wiki/ARCHITECTURE.md`](../../src/llm_wiki/ARCHITECTURE.md)
- [`../../src/llm_wiki/ROADMAP.md`](../../src/llm_wiki/ROADMAP.md)
- [`../../src/llm_wiki/FUTURE_ENHANCEMENTS.md`](../../src/llm_wiki/FUTURE_ENHANCEMENTS.md)

## Header Files

| Header | Primary Class / Interface |
|--------|--------------------------|
| `llm_wiki_plugin_interface.h` | `ILLMWikiPlugin` contract + request/response types |
| `llm_wiki_status.h` | Lightweight `Status` type used across plugin APIs |
| `workspace_state_manager.h` | `WorkspaceStateManager` state persistence + recovery |
| `process_policy_manager.h` | YAML process policy loading + validation |
| `rocksdb_wiki_store.h` | `RocksDbWikiStore` persistent key-value storage |

## Usage

```cpp
#include "llm_wiki/llm_wiki_plugin_interface.h"

auto wiki_plugin = themis::llm_wiki::createWikiPlugin({
    .index_path = "/path/to/wiki/index",
    .semantic_search_enabled = true
});

auto workspace = wiki_plugin->createWorkspace("project_name");
auto results = workspace->search("query", 10);
```

For full runtime usage examples (workspace management, search, synthesis), see [`../../src/llm_wiki/README.md`](../../src/llm_wiki/README.md).

## Key Configuration Surface

Important configuration entry points are declared in:

- `llm_wiki_plugin_interface.h` (`initialize(config_json)` contract)
- `workspace_state_manager.h` (workspace state persistence contract)
- `rocksdb_wiki_store.h` (RocksDB-backed persistence backend)

## Build

```cmake
cmake --preset release && cmake --build build/release --target themis-llm_wiki
```

## See Also

- [`../../src/llm_wiki/README.md`](../../src/llm_wiki/README.md) — implementation details
- [`../../src/llm/README.md`](../../src/llm/README.md) — LLM module integration
- [`../../src/rag/README.md`](../../src/rag/README.md) — RAG system integration

## Installation

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
