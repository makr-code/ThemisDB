# Ethics AI Tests

Focused unit tests for the Ethics AI plugin.

## Status

Tests are pending implementation. Placeholder CMakeLists.txt is present.

## Source files to migrate from ThemisDB

| ThemisDB source | Destination |
|---|---|
| `tests/ethics_ai/CMakeLists.txt` | `tests/CMakeLists.txt` (replace) |
| (no `test_*.cpp` exist yet) | Implement tests per ROADMAP.md |

## Test scope (planned)

- `test_philosophy_loader.cpp` — YAML profile loading, invalid YAML, missing keys
- `test_ethics_evaluator.cpp` — 5-dimension scoring correctness
- `test_discourse_engine.cpp` — Debate round management, convergence detection
- `test_argument_store.cpp` — BaseEntity CRUD, AQL queries
- `test_rag_context_engine.cpp` — Context retrieval, similarity threshold
