---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# LLM Wiki Public Headers

`include/llm_wiki/` contains the public SDK contract and supporting declarations. The primary plugin interface and request/result types are in [`llm_wiki_plugin_interface.h`](llm_wiki_plugin_interface.h); status values are in [`llm_wiki_status.h`](llm_wiki_status.h).

## Supporting headers

| Header | Declared responsibility |
|---|---|
| `workspace_state_manager.h` | Workspace JSON state persistence contract |
| `process_policy_manager.h` | Process policy types and validation interface |
| `rocksdb_wiki_store.h` | Optional RocksDB key/value store interface |

## Contract and implementation

The public header describes available operations and options. Do not assume that every declared option or operation is fully implemented by the current plugin. In particular, the current plugin query does not honor `WikiQueryOptions`, `ingestWikipediaDump()` does not process its input, and the header's thread-safety/lock-free-query claim conflicts with observed locking and unlocked plugin-member access. Until that contract is resolved, serialize plugin calls externally. See [`../../src/llm_wiki/API.md`](../../src/llm_wiki/API.md) for the source comparison and [`../../src/llm_wiki/README.md`](../../src/llm_wiki/README.md) for module context.

## Build and integration

The implementation target is `themis_llm_wiki`, defined in [`../../src/llm_wiki/CMakeLists.txt`](../../src/llm_wiki/CMakeLists.txt). RocksDB linkage is optional and depends on the configured build. Use the repository CMake preset and link against the target exported by that build; the public declarations alone do not guarantee that an optional backend is enabled.

> Exact model backend not verifiable from final document text alone.
